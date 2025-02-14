// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "components/password_manager/core/browser/ui/export_progress_status.h"

namespace password_manager {

class CredentialProviderInterface;

// Controls the exporting of passwords. PasswordManagerExporter will perform
// the export asynchrnously as soon as all the required info is available
// (password list and destination), unless canceled.
class PasswordManagerExporter {
 public:
  using ProgressCallback =
      base::RepeatingCallback<void(password_manager::ExportProgressStatus,
                                   const std::string&)>;

  explicit PasswordManagerExporter(
      password_manager::CredentialProviderInterface*
          credential_provider_interface,
      ProgressCallback on_progress);
  virtual ~PasswordManagerExporter();

  // Pre-load the passwords from the password store.
  virtual void PreparePasswordsForExport();

  // Set the destination, where the passwords will be written when they are
  // ready.
  virtual void SetDestination(const base::FilePath& destination);

  // Best-effort canceling of any on-going task related to exporting.
  virtual void Cancel();

  // Returns the most recent ExportProgressStatus value, as would have been
  // seen on the callback provided to the constructor.
  virtual ExportProgressStatus GetProgressStatus();

  // Replace the function which writes to the filesystem with a custom action.
  // The return value is -1 on error, otherwise the number of bytes written.
  void SetWriteForTesting(int (*write_function)(const base::FilePath& filename,
                                                const char* data,
                                                int size));

 private:
  // Caches the serialised password list. It proceeds to export, if all the
  // parts are ready. |count| is the number of passwords which were serialised.
  // |serialised| is the serialised list of passwords.
  void SetSerialisedPasswordList(size_t count, const std::string& serialised);

  // Returns true if all the necessary data is available.
  bool IsReadyForExport();

  // Performs the export. It should not be called before the data is available.
  // At the end, it clears cached fields.
  void Export();

  // Callback after the passwords have been serialised. It reports the result to
  // the UI and to metrics. |destination| is the folder we wrote to. |count| is
  // the number of passwords exported. |success| is whether they were actually
  // written.
  void OnPasswordsExported(const base::FilePath& destination,
                           int count,
                           bool success);

  // Wrapper for the |on_progress_| callback, which caches |status|, so that
  // it can be provided by GetProgressStatus.
  void OnProgress(ExportProgressStatus status, const std::string& folder);

  // The source of the password list which will be exported.
  CredentialProviderInterface* const credential_provider_interface_;

  // Callback to the UI.
  ProgressCallback on_progress_;

  // The most recent progress status update, as was seen on |on_progress_|.
  ExportProgressStatus last_progress_status_;

  // The password list that was read from the store and serialised.
  std::string serialised_password_list_;
  // The number of passwords that were serialised. Useful for metrics.
  size_t password_count_;

  // The destination which was provided and where the password list will be
  // sent. It will be cleared once exporting is complete.
  base::FilePath destination_;

  // The moment in time that we started reading and serialising the password
  // list. Useful for metrics.
  base::Time export_preparation_started_;

  // The function which does the actual writing. It should point to
  // base::WriteFile, unless it's changed for testing purposes.
  int (*write_function_)(const base::FilePath& filename,
                         const char* data,
                         int size);

  // |task_runner_| is used for time-consuming tasks during exporting. The tasks
  // will dereference a WeakPtr to |*this|, which means they all need to run on
  // the same sequence.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<PasswordManagerExporter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerExporter);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_EXPORT_PASSWORD_MANAGER_EXPORTER_H_
