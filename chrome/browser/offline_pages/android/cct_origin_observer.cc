// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/android/cct_origin_observer.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "jni/CctOfflinePageModelObserver_jni.h"

using base::android::ConvertUTF8ToJavaString;

namespace offline_pages {

namespace {
int kCctOriginObserverUserDataKey;
}

// static
void CctOriginObserver::AttachToOfflinePageModel(OfflinePageModel* model) {
  if (!IsOfflinePagesCTV2Enabled())
    return;
  auto origin_observer = base::MakeUnique<CctOriginObserver>();
  model->AddObserver(origin_observer.get());
  model->SetUserData(&kCctOriginObserverUserDataKey,
                     std::move(origin_observer));
}

CctOriginObserver::CctOriginObserver() = default;
CctOriginObserver::~CctOriginObserver() = default;

void CctOriginObserver::OfflinePageModelLoaded(OfflinePageModel* model) {}

void CctOriginObserver::OfflinePageAdded(OfflinePageModel* model,
                                         const OfflinePageItem& added_page) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_CctOfflinePageModelObserver_onPageChanged(
      env, ConvertUTF8ToJavaString(env, added_page.request_origin));
}

void CctOriginObserver::OfflinePageDeleted(
    const OfflinePageModel::DeletedPageInfo& page_info) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_CctOfflinePageModelObserver_onPageChanged(
      env, ConvertUTF8ToJavaString(env, page_info.request_origin));
}

}  // namespace offline_pages
