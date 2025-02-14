// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_APP_LIST_METRICS_H_
#define UI_APP_LIST_APP_LIST_METRICS_H_

#include "ui/app_list/app_list_export.h"

namespace app_list {

class AppListModel;
class SearchModel;
class SearchResult;

APP_LIST_EXPORT void RecordSearchResultOpenSource(
    const SearchResult* result,
    const AppListModel* model,
    const SearchModel* search_model);

}  // namespace app_list

#endif  // UI_APP_LIST_APP_LIST_METRICS_H_
