#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Wrapper script to help run clang tools across Chromium code.

How to use run_tool.py:
If you want to run a clang tool across all Chromium code:
run_tool.py <tool> <path/to/compiledb>

If you want to include all files mentioned in the compilation database
(this will also include generated files, unlike the previous command):
run_tool.py <tool> <path/to/compiledb> --all

If you want to run the clang tool across only chrome/browser and
content/browser:
run_tool.py <tool> <path/to/compiledb> chrome/browser content/browser

Please see docs/clang_tool_refactoring.md for more information, which documents
the entire automated refactoring flow in Chromium.

Why use run_tool.py (instead of running a clang tool directly):
The clang tool implementation doesn't take advantage of multiple cores, and if
it fails mysteriously in the middle, all the generated replacements will be
lost. Additionally, if the work is simply sharded across multiple cores by
running multiple RefactoringTools, problems arise when they attempt to rewrite a
file at the same time.

run_tool.py will
1) run multiple instances of clang tool in parallel
2) gather stdout from clang tool invocations
3) "atomically" forward #2 to stdout

Output of run_tool.py can be piped into extract_edits.py and then into
apply_edits.py. These tools will extract individual edits and apply them to the
source files. These tools assume the clang tool emits the edits in the
following format:
    ...
    ==== BEGIN EDITS ====
    r:::<file path>:::<offset>:::<length>:::<replacement text>
    r:::<file path>:::<offset>:::<length>:::<replacement text>
    ...etc...
    ==== END EDITS ====
    ...

extract_edits.py extracts only lines between BEGIN/END EDITS markers
apply_edits.py reads edit lines from stdin and applies the edits
"""

import argparse
import functools
import multiprocessing
import os
import os.path
import re
import subprocess
import sys

script_dir = os.path.dirname(os.path.realpath(__file__))
tool_dir = os.path.abspath(os.path.join(script_dir, '../pylib'))
sys.path.insert(0, tool_dir)

from clang import compile_db


def _PruneGitFiles(git_files, paths):
  """Prunes the list of files from git to include only those that are either in
  |paths| or start with one item in |paths|.

  Args:
    git_files: List of all repository files.
    paths: Prefix filter for the returned paths. May contain multiple entries,
        and the contents should be absolute paths.

  Returns:
    Pruned list of files.
  """
  if not git_files:
    return []
  pruned_list = []
  git_index = 0
  for path in sorted(paths):
    least = git_index
    most = len(git_files) - 1
    while least <= most:
      middle = (least + most ) / 2
      if git_files[middle] == path:
        least = middle
        break
      elif git_files[middle] > path:
        most = middle - 1
      else:
        least = middle + 1
    while git_files[least].startswith(path):
      pruned_list.append(git_files[least])
      least += 1
    git_index = least

  return pruned_list


def _GetFilesFromGit(paths=None):
  """Gets the list of files in the git repository if |paths| includes prefix
  path filters or is empty. All complete filenames in |paths| are also included
  in the output.

  Args:
    paths: Prefix filter for the returned paths. May contain multiple entries.
  """
  partial_paths = []
  files = []
  for p in paths:
    real_path = os.path.realpath(p)
    if os.path.isfile(real_path):
      files.append(real_path)
    else:
      partial_paths.append(real_path)
  if partial_paths or not files:
    args = []
    if sys.platform == 'win32':
      args.append('git.bat')
    else:
      args.append('git')
    args.append('ls-files')
    command = subprocess.Popen(args, stdout=subprocess.PIPE)
    output, _ = command.communicate()
    git_files = [os.path.realpath(p) for p in output.splitlines()]
    if partial_paths:
      git_files = _PruneGitFiles(git_files, partial_paths)
    files.extend(git_files)
  return files


def _GetFilesFromCompileDB(build_directory):
  """ Gets the list of files mentioned in the compilation database.

  Args:
    build_directory: Directory that contains the compile database.
  """
  return [os.path.join(entry['directory'], entry['file'])
          for entry in compile_db.Read(build_directory)]


def _ExecuteTool(toolname, tool_args, build_directory, filename):
  """Executes the clang tool.

  This is defined outside the class so it can be pickled for the multiprocessing
  module.

  Args:
    toolname: Name of the clang tool to execute.
    tool_args: Arguments to be passed to the clang tool. Can be None.
    build_directory: Directory that contains the compile database.
    filename: The file to run the clang tool over.

  Returns:
    A dictionary that must contain the key "status" and a boolean value
    associated with it.

    If status is True, then the generated output is stored with the key
    "stdout_text" in the dictionary.

    Otherwise, the filename and the output from stderr are associated with the
    keys "filename" and "stderr_text" respectively.
  """
  args = [toolname, '-p', build_directory, filename]
  if (tool_args):
    args.extend(tool_args)
  command = subprocess.Popen(
      args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  stdout_text, stderr_text = command.communicate()
  stderr_text = re.sub(
      r"^warning: .*'linker' input unused \[-Wunused-command-line-argument\]\n",
      "", stderr_text, flags=re.MULTILINE)
  if command.returncode != 0:
    return {'status': False, 'filename': filename, 'stderr_text': stderr_text}
  else:
    return {'status': True, 'filename': filename, 'stdout_text': stdout_text,
            'stderr_text': stderr_text}


class _CompilerDispatcher(object):
  """Multiprocessing controller for running clang tools in parallel."""

  def __init__(self, toolname, tool_args, build_directory, filenames):
    """Initializer method.

    Args:
      toolname: Path to the tool to execute.
      tool_args: Arguments to be passed to the tool. Can be None.
      build_directory: Directory that contains the compile database.
      filenames: The files to run the tool over.
    """
    self.__toolname = toolname
    self.__tool_args = tool_args
    self.__build_directory = build_directory
    self.__filenames = filenames
    self.__success_count = 0
    self.__failed_count = 0

  @property
  def failed_count(self):
    return self.__failed_count

  def Run(self):
    """Does the grunt work."""
    pool = multiprocessing.Pool()
    result_iterator = pool.imap_unordered(
        functools.partial(_ExecuteTool, self.__toolname, self.__tool_args,
                          self.__build_directory),
                          self.__filenames)
    for result in result_iterator:
      self.__ProcessResult(result)
    sys.stderr.write('\n')

  def __ProcessResult(self, result):
    """Handles result processing.

    Args:
      result: The result dictionary returned by _ExecuteTool.
    """
    if result['status']:
      self.__success_count += 1
      sys.stdout.write(result['stdout_text'])
      sys.stderr.write(result['stderr_text'])
    else:
      self.__failed_count += 1
      sys.stderr.write('\nFailed to process %s\n' % result['filename'])
      sys.stderr.write(result['stderr_text'])
      sys.stderr.write('\n')
    done_count = self.__success_count + self.__failed_count
    percentage = (float(done_count) / len(self.__filenames)) * 100
    sys.stderr.write(
        'Processed %d files with %s tool (%d failures) [%.2f%%]\r' %
        (done_count, self.__toolname, self.__failed_count, percentage))


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--options-file',
      help='optional file to read options from')
  args, argv = parser.parse_known_args()
  if args.options_file:
    argv = open(args.options_file).read().split()

  parser.add_argument('--tool', required=True, help='clang tool to run')
  parser.add_argument('--all', action='store_true')
  parser.add_argument(
      '--generate-compdb',
      action='store_true',
      help='regenerate the compile database before running the tool')
  parser.add_argument(
      '--shard',
      metavar='<n>-of-<count>')
  parser.add_argument(
      '-p',
      required=True,
      help='path to the directory that contains the compile database')
  parser.add_argument(
      'path_filter',
      nargs='*',
      help='optional paths to filter what files the tool is run on')
  parser.add_argument(
      '--tool-arg', nargs='?', action='append',
      help='optional arguments passed to the tool')
  parser.add_argument(
      '--tool-path', nargs='?',
      help='optional path to the tool directory')
  args = parser.parse_args(argv)

  if args.tool_path:
    tool_path = os.path.abspath(args.tool_path)
  else:
    tool_path = os.path.abspath(os.path.join(
          os.path.dirname(__file__),
          '../../../third_party/llvm-build/Release+Asserts/bin'))

  if args.generate_compdb:
    with open(os.path.join(args.p, 'compile_commands.json'), 'w') as f:
      f.write(compile_db.GenerateWithNinja(args.p))

  if args.all:
    source_filenames = set(_GetFilesFromCompileDB(args.p))
  else:
    git_filenames = set(_GetFilesFromGit(args.path_filter))
    # Filter out files that aren't C/C++/Obj-C/Obj-C++.
    extensions = frozenset(('.c', '.cc', '.cpp', '.m', '.mm'))
    source_filenames = [f
                        for f in git_filenames
                        if os.path.splitext(f)[1] in extensions]

  if args.shard:
    total_length = len(source_filenames)
    match = re.match(r'(\d+)-of-(\d+)$', args.shard)
    # Input is 1-based, but modular arithmetic is 0-based.
    shard_number = int(match.group(1)) - 1
    shard_count = int(match.group(2))
    source_filenames = [
        f[1] for f in enumerate(sorted(source_filenames))
        if f[0] % shard_count == shard_number
    ]
    print 'Shard %d-of-%d will process %d entries out of %d' % (
        shard_number, shard_count, len(source_filenames), total_length)

  dispatcher = _CompilerDispatcher(os.path.join(tool_path, args.tool),
                                   args.tool_arg,
                                   args.p,
                                   source_filenames)
  dispatcher.Run()
  return -dispatcher.failed_count


if __name__ == '__main__':
  sys.exit(main())
