// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/chrome_screenshot_grabber.h"
#include "chrome/browser/ui/ash/chrome_screenshot_grabber_test_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/session_manager/core/session_manager.h"
#include "components/signin/core/account_id/account_id.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_monitor.h"
#include "ui/base/clipboard/clipboard_observer.h"

class ChromeScreenshotGrabberBrowserTest
    : public InProcessBrowserTest,
      public ChromeScreenshotGrabberTestObserver,
      public ui::ClipboardObserver {
 public:
  ChromeScreenshotGrabberBrowserTest() = default;
  ~ChromeScreenshotGrabberBrowserTest() override = default;

  void SetUpOnMainThread() override {
    display_service_ = std::make_unique<NotificationDisplayServiceTester>(
        browser()->profile());
    display_service_->SetNotificationAddedClosure(base::BindRepeating(
        &ChromeScreenshotGrabberBrowserTest::OnNotificationAdded,
        base::Unretained(this)));
  }

  void SetTestObserver(ChromeScreenshotGrabber* chrome_screenshot_grabber,
                       ChromeScreenshotGrabberTestObserver* observer) {
    chrome_screenshot_grabber->test_observer_ = observer;
  }

  // Overridden from ui::ScreenshotGrabberObserver
  void OnScreenshotCompleted(
      ui::ScreenshotResult screenshot_result,
      const base::FilePath& screenshot_path) override {
    screenshot_result_ = screenshot_result;
    screenshot_path_ = screenshot_path;
  }

  void OnNotificationAdded() {
    notification_added_ = true;
    message_loop_runner_->Quit();
  }

  // Overridden from ui::ClipboardObserver
  void OnClipboardDataChanged() override {
    clipboard_changed_ = true;
    message_loop_runner_->Quit();
  }

  void RunLoop() {
    message_loop_runner_ = new content::MessageLoopRunner;
    message_loop_runner_->Run();
  }

  bool IsImageClipboardAvailable() {
    return ui::Clipboard::GetForCurrentThread()->IsFormatAvailable(
        ui::Clipboard::GetBitmapFormatType(), ui::CLIPBOARD_TYPE_COPY_PASTE);
  }

  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  std::unique_ptr<NotificationDisplayServiceTester> display_service_;
  ui::ScreenshotResult screenshot_result_;
  base::FilePath screenshot_path_;
  bool notification_added_ = false;
  bool clipboard_changed_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeScreenshotGrabberBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ChromeScreenshotGrabberBrowserTest, TakeScreenshot) {
  ChromeScreenshotGrabber* chrome_screenshot_grabber =
      ChromeScreenshotGrabber::Get();
  SetTestObserver(chrome_screenshot_grabber, this);
  base::ScopedTempDir directory;
  ASSERT_TRUE(directory.CreateUniqueTempDir());
  EXPECT_TRUE(chrome_screenshot_grabber->CanTakeScreenshot());

  chrome_screenshot_grabber->HandleTakeWindowScreenshot(
      ash::Shell::GetPrimaryRootWindow());

  EXPECT_FALSE(
      chrome_screenshot_grabber->screenshot_grabber()->CanTakeScreenshot());

  RunLoop();
  SetTestObserver(chrome_screenshot_grabber, nullptr);

  EXPECT_TRUE(notification_added_);
  EXPECT_TRUE(display_service_->GetNotification(std::string("screenshot")));

  EXPECT_EQ(ui::ScreenshotResult::SUCCESS, screenshot_result_);
  EXPECT_TRUE(base::PathExists(screenshot_path_));

  EXPECT_FALSE(IsImageClipboardAvailable());
  ui::ClipboardMonitor::GetInstance()->AddObserver(this);

  // Copy to clipboard button.
  display_service_->SimulateClick(NotificationHandler::Type::TRANSIENT,
                                  std::string("screenshot"), 0, base::nullopt);

  RunLoop();
  ui::ClipboardMonitor::GetInstance()->RemoveObserver(this);

  EXPECT_TRUE(clipboard_changed_);
  EXPECT_TRUE(IsImageClipboardAvailable());
}
