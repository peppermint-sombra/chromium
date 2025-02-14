// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_UI_GAIA_DIALOG_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_UI_GAIA_DIALOG_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace ui {
class Accelerator;
}

namespace views {
class WebDialogView;
class Widget;
}  // namespace views

namespace chromeos {

class LoginDisplayHostViews;
class OobeUI;

// This class manages the behavior of the gaia signin dialog.
// And its lifecyle is managed by the widget created in Show().
//   WebDialogView<----delegate_----GaiaDialogDelegate
//         |
//         |
//         V
//   clientView---->Widget's view hierachy
class GaiaDialogDelegate : public ui::WebDialogDelegate {
 public:
  explicit GaiaDialogDelegate(base::WeakPtr<LoginDisplayHostViews> controller);
  ~GaiaDialogDelegate() override;

  void Show();
  void Close();
  void SetSize(int width, int height);
  OobeUI* GetOobeUI() const;

 private:
  // ui::WebDialogDelegate:
  ui::ModalType GetDialogModalType() const override;
  base::string16 GetDialogTitle() const override;
  GURL GetDialogContentURL() const override;
  void GetWebUIMessageHandlers(
      std::vector<content::WebUIMessageHandler*>* handlers) const override;
  void GetDialogSize(gfx::Size* size) const override;
  bool CanResizeDialog() const override;
  std::string GetDialogArgs() const override;
  // NOTE: This function deletes this object at the end.
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(content::WebContents* source,
                       bool* out_close_dialog) override;
  bool ShouldShowDialogTitle() const override;
  bool HandleContextMenu(const content::ContextMenuParams& params) override;
  std::vector<ui::Accelerator> GetAccelerators() override;

  base::WeakPtr<LoginDisplayHostViews> controller_;

  // This is owned by the underlying native widget.
  // Before its deletion, onDialogClosed will get called and delete this object.
  views::Widget* dialog_widget_ = nullptr;
  views::WebDialogView* dialog_view_ = nullptr;
  gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(GaiaDialogDelegate);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_UI_GAIA_DIALOG_DELEGATE_H_
