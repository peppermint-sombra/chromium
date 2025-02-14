// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/platform/impl/spdy_flags_impl.h"

namespace net {

// Deliver OnPaddingLength separately from OnPadding.
bool h2_on_stream_pad_length = true;

}  // namespace net
