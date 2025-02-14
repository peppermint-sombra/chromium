// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_CHROME_SCREENSHOT_GRABBER_H_
#define CHROME_BROWSER_UI_ASH_CHROME_SCREENSHOT_GRABBER_H_

#include <memory>

#include "ash/screenshot_delegate.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "build/build_config.h"
#include "components/drive/chromeos/file_system_interface.h"
#include "components/drive/drive.pb.h"
#include "ui/gfx/image/image.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/snapshot/screenshot_grabber.h"

class Profile;

namespace ash {
class ChromeScreenshotGrabberTest;
}  // namespace ash

namespace policy {
class PolicyTest;
}  // namespace policy

class ChromeScreenshotGrabberBrowserTest;
class ChromeScreenshotGrabberTestObserver;

// Result of asynchronous file operations.
enum class ScreenshotFileResult {
  SUCCESS,
  CHECK_DIR_FAILED,
  CREATE_DIR_FAILED,
  CREATE_FAILED
};

class ChromeScreenshotGrabber : public ash::ScreenshotDelegate {
 public:
  // Callback called with the |result| of trying to create a local writable
  // |path| for the possibly remote path.
  using FileCallback = base::Callback<void(ScreenshotFileResult result,
                                           const base::FilePath& path)>;

  ChromeScreenshotGrabber();
  ~ChromeScreenshotGrabber() override;

  static ChromeScreenshotGrabber* Get();

  ui::ScreenshotGrabber* screenshot_grabber() {
    return screenshot_grabber_.get();
  }

  // ash::ScreenshotDelegate:
  void HandleTakeScreenshotForAllRootWindows() override;
  void HandleTakePartialScreenshot(aura::Window* window,
                                   const gfx::Rect& rect) override;
  void HandleTakeWindowScreenshot(aura::Window* window) override;
  bool CanTakeScreenshot() override;

  // TODO(erg): This is the endpoint for a new interface which will be added in
  // a future patch. It is intended to be both mojo proxyable, and usable as a
  // callback from ScreenshotGrabber.
  void OnTookScreenshot(const base::Time& screenshot_time,
                        const base::Optional<int>& display_num,
                        ui::ScreenshotResult result,
                        scoped_refptr<base::RefCountedMemory> png_data);

 private:
  friend class ash::ChromeScreenshotGrabberTest;
  friend class ChromeScreenshotGrabberBrowserTest;
  friend class policy::PolicyTest;

  // Prepares a writable file for |path|. If |path| is a non-local path (i.e.
  // Google drive) and it is supported this will create a local cached copy of
  // the remote file and call the callback with the local path.
  void PrepareFileAndRunOnBlockingPool(const base::FilePath& path,
                                       const FileCallback& callback);

  // Called once all file writing is completed, or on error.
  void OnScreenshotCompleted(ui::ScreenshotResult result,
                             const base::FilePath& screenshot_path);

  // Callback method of FileSystemInterface::GetFile().
  // Runs ReadScreenshotFileForPreviewLocal if successful. Otherwise, runs
  // OnReadScreenshotFileForPreviewCompleted to show notification without
  // screenshot preview.
  // One way to try screenshot saved to Drive is to change the download
  // directory from Settings > Downloads > Location.
  //
  // |screenshot_path| is screenshot path on Drive.
  // Parameters after |screenshot_path| is set by GetFile().
  // |screenshot_cache_path| is a local cache of remote |screenshot_path|.
  void ReadScreenshotFileForPreviewDrive(
      const base::FilePath& screenshot_path,
      drive::FileError error,
      const base::FilePath& screenshot_cache_path,
      std::unique_ptr<drive::ResourceEntry> entry);

  // Reads a screenshot file in |screenshot_cache_path|, and runs
  // DecodeScreenshotFileForPreview.
  //
  // |screenshot_path| and |screenshot_cache_path| are different when the
  // screenshot is saved to Drive. Otherwise, they should be same.
  // |screenshot_path| can be a Drive path while |screenshot_cache_path| is
  // always a local path.
  void ReadScreenshotFileForPreviewLocal(
      const base::FilePath& screenshot_path,
      const base::FilePath& screenshot_cache_path);

  // Decodes |image_data| and run OnScreenshotFileForPreviewDecoded.
  // Although |image_data| is originally generated by the screenshot grabber,
  // it is saved to local or remote storage, so we have to decode it in
  // a sandboxed process.
  //
  // |screenshot_path| is a path that is opened by file manager when the
  // notification is clicked.
  void DecodeScreenshotFileForPreview(const base::FilePath& screenshot_path,
                                      std::string image_data);

  // Callback method of DecodeScreenshotFileForPreview.
  // Converts SkBitmap to gfx::Image and calls
  // OnReadScreenshotFileForPreviewCompleted.
  //
  // |screenshot_path| is a path that is opened by file manager when the
  // notification is clicked.
  void OnScreenshotFileForPreviewDecoded(const base::FilePath& screenshot_path,
                                         const SkBitmap& decoded_image);

  // Shows "Screenshot taken" notification.
  //
  // |screenshot_path| is a path that is opened by file manager when the
  // notification is clicked.
  // |image| is a preview image attached to the notification. It can be empty.
  void OnReadScreenshotFileForPreviewCompleted(
      ui::ScreenshotResult result,
      const base::FilePath& screenshot_path,
      gfx::Image image);

  Profile* GetProfile();

  std::unique_ptr<ui::ScreenshotGrabber> screenshot_grabber_;

  // Forwards OnScreenshotCompleted() events to a test.
  ChromeScreenshotGrabberTestObserver* test_observer_ = nullptr;

  base::WeakPtrFactory<ChromeScreenshotGrabber> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeScreenshotGrabber);
};

#endif  // CHROME_BROWSER_UI_ASH_CHROME_SCREENSHOT_GRABBER_H_
