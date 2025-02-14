// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SerializedBlobStructTraits_h
#define SerializedBlobStructTraits_h

#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "platform/PlatformExport.h"
#include "platform/blob/BlobData.h"
#include "third_party/WebKit/common/blob/serialized_blob.mojom-blink.h"

namespace mojo {

template <>
struct PLATFORM_EXPORT
    StructTraits<blink::mojom::blink::SerializedBlob::DataView,
                 scoped_refptr<blink::BlobDataHandle>> {
  static WTF::String uuid(const scoped_refptr<blink::BlobDataHandle>& input) {
    return input->Uuid();
  }

  static WTF::String content_type(
      const scoped_refptr<blink::BlobDataHandle>& input) {
    return input->GetType();
  }

  static uint64_t size(const scoped_refptr<blink::BlobDataHandle>& input) {
    return input->size();
  }

  static blink::mojom::blink::BlobPtr blob(
      const scoped_refptr<blink::BlobDataHandle>& input) {
    return input->CloneBlobPtr();
  }

  static bool Read(blink::mojom::blink::SerializedBlob::DataView,
                   scoped_refptr<blink::BlobDataHandle>* out);
};

}  // namespace mojo

#endif  // SerializedBlobStructTraits_h
