// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_CACHE_URL_LOADER_H_
#define SERVICES_NETWORK_CACHE_URL_LOADER_H_

#include "services/network/public/interfaces/url_loader.mojom.h"

namespace net {
class URLRequestContext;
}

namespace network {

// Creates a URLLoader that responds to developer requests to view the cache.
void StartCacheURLLoader(const GURL& url,
                         net::URLRequestContext* request_context,
                         mojom::URLLoaderClientPtr client);

}  // namespace network

#endif  // SERVICES_NETWORK_CACHE_URL_LOADER_H_
