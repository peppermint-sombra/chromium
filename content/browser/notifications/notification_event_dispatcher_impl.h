// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_EVENT_DISPATCHER_IMPL_H_
#define CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_EVENT_DISPATCHER_IMPL_H_

#include <map>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/common/content_export.h"
#include "content/public/browser/notification_database_data.h"
#include "content/public/browser/notification_event_dispatcher.h"
#include "third_party/WebKit/public/platform/modules/notifications/notification_service.mojom.h"

namespace content {

class CONTENT_EXPORT NotificationEventDispatcherImpl
    : public NotificationEventDispatcher {
 public:
  // Returns the instance of the NotificationEventDispatcherImpl. Must be called
  // on the UI thread.
  static NotificationEventDispatcherImpl* GetInstance();

  // NotificationEventDispatcher implementation.
  void DispatchNotificationClickEvent(
      BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& origin,
      const base::Optional<int>& action_index,
      const base::Optional<base::string16>& reply,
      NotificationDispatchCompleteCallback dispatch_complete_callback) override;
  void DispatchNotificationCloseEvent(
      BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& origin,
      bool by_user,
      NotificationDispatchCompleteCallback dispatch_complete_callback) override;
  void DispatchNonPersistentShowEvent(
      const std::string& notification_id) override;
  void DispatchNonPersistentClickEvent(
      const std::string& notification_id) override;
  void DispatchNonPersistentCloseEvent(
      const std::string& notification_id) override;

  // Called when a renderer that had shown a non persistent notification
  // dissappears.
  void RendererGone(int renderer_id);

  // Register the fact that a non persistent notification has been displayed.
  void RegisterNonPersistentNotification(const std::string& notification_id,
                                         int renderer_id,
                                         int request_id);

  // Registers |listener| to receive the show, click and close events of the
  // non-persistent notification identified by |notification_id|.
  void RegisterNonPersistentNotificationListener(
      const std::string& notification_id,
      blink::mojom::NonPersistentNotificationListenerPtrInfo listener_ptr_info);

 private:
  friend class NotificationEventDispatcherImplTest;
  friend struct base::DefaultSingletonTraits<NotificationEventDispatcherImpl>;

  NotificationEventDispatcherImpl();
  ~NotificationEventDispatcherImpl() override;

  // Removes all references to the listener registered to receive events
  // from the non-persistent notification identified by |notification_id|.
  // Should be called when the connection to this listener goes away.
  void HandleConnectionErrorForNonPersistentNotificationListener(
      const std::string& notification_id);

  // Notification Id -> renderer Id.
  std::map<std::string, int> renderer_ids_;

  // Notification Id -> request Id.
  std::map<std::string, int> request_ids_;

  // Notification Id -> listener.
  std::map<std::string, blink::mojom::NonPersistentNotificationListenerPtr>
      non_persistent_notification_listeners_;

  DISALLOW_COPY_AND_ASSIGN(NotificationEventDispatcherImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_EVENT_DISPATCHER_IMPL_H_
