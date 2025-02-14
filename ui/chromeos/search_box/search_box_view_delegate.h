// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHOMEOS_SEARCH_BOX_SEARCH_BOX_VIEW_DELEGATE_H_
#define UI_CHOMEOS_SEARCH_BOX_SEARCH_BOX_VIEW_DELEGATE_H_

#include "ui/chromeos/search_box/search_box_export.h"

namespace search_box {

class SearchBoxViewBase;

class SEARCH_BOX_EXPORT SearchBoxViewDelegate {
 public:
  // Invoked when query text has changed by the user.
  virtual void QueryChanged(SearchBoxViewBase* sender) = 0;

  // Invoked when the back button has been pressed.
  virtual void BackButtonPressed() = 0;

 protected:
  virtual ~SearchBoxViewDelegate() {}
};

}  // namespace search_box

#endif  // UI_CHOMEOS_SEARCH_BOX_SEARCH_BOX_VIEW_DELEGATE_H_
