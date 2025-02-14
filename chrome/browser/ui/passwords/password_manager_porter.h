// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_
#define CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_

#include <memory>
#include <string>

#include "components/password_manager/core/browser/import/password_importer.h"
#include "components/password_manager/core/browser/ui/export_flow.h"
#include "components/password_manager/core/browser/ui/export_progress_status.h"
#include "components/password_manager/core/browser/ui/import_flow.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace content {
class WebContents;
}

namespace password_manager {
class CredentialProviderInterface;
class PasswordManagerExporter;
}

class Profile;

// Handles the exporting of passwords to a file, and the importing of such a
// file to the Password Manager.
class PasswordManagerPorter : public ui::SelectFileDialog::Listener,
                              public password_manager::ExportFlow,
                              public password_manager::ImportFlow {
 public:
  using ProgressCallback =
      base::RepeatingCallback<void(password_manager::ExportProgressStatus,
                                   const std::string&)>;

  explicit PasswordManagerPorter(
      std::unique_ptr<password_manager::PasswordManagerExporter> exporter);
  ~PasswordManagerPorter() override;

  // Create an instance of PasswordManagerPorter.
  // |credential_provider_interface| provides the credentials which can be
  // exported. |on_export_progress_callback| will be called with updates to
  // the progress of exporting.
  static std::unique_ptr<PasswordManagerPorter>
  CreatePasswordManagerPorterWithCredentialProvider(
      password_manager::CredentialProviderInterface*
          credential_provider_interface,
      ProgressCallback on_export_progress_callback);

  void set_web_contents(content::WebContents* web_contents) {
    web_contents_ = web_contents;
  }

  // password_manager::ExportFlow
  bool Store() override;
  password_manager::ExportProgressStatus GetExportProgressStatus() override;

  // password_manager::ImportFlow
  void Load() override;

 private:
  enum Type {
    PASSWORD_IMPORT,
    PASSWORD_EXPORT,
  };

  // Display the file-picker dialogue for either importing or exporting
  // passwords.
  void PresentFileSelector(content::WebContents* web_contents, Type type);

  // Callback from the file selector dialogue when a file has been picked (for
  // either import or export).
  // ui::SelectFileDialog::Listener:
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;

  virtual void ImportPasswordsFromPath(const base::FilePath& path);

  virtual void ExportPasswordsToPath(const base::FilePath& path);

  std::unique_ptr<password_manager::PasswordManagerExporter> exporter_;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  Profile* profile_ = nullptr;

  // Caching the current WebContents for when PresentFileSelector is called.
  content::WebContents* web_contents_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerPorter);
};

#endif  // CHROME_BROWSER_UI_PASSWORDS_PASSWORD_MANAGER_PORTER_H_
