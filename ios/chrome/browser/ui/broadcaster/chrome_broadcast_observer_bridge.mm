// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcast_observer_bridge.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

ChromeBroadcastObserverInterface::~ChromeBroadcastObserverInterface() = default;

@implementation ChromeBroadcastOberverBridge
@synthesize observer = _observer;

- (instancetype)initWithObserver:(ChromeBroadcastObserverInterface*)observer {
  if (self = [super init]) {
    _observer = observer;
    DCHECK(_observer);
  }
  return self;
}

- (void)broadcastContentScrollOffset:(CGFloat)offset {
  self.observer->OnContentScrollOffsetBroadcasted(offset);
}

- (void)broadcastScrollViewIsScrolling:(BOOL)scrolling {
  self.observer->OnScrollViewIsScrollingBroadcasted(scrolling);
}

- (void)broadcastScrollViewIsDragging:(BOOL)dragging {
  self.observer->OnScrollViewIsDraggingBroadcasted(dragging);
}

- (void)broadcastToolbarHeight:(CGFloat)height {
  self.observer->OnToolbarHeightBroadcasted(height);
}

@end
