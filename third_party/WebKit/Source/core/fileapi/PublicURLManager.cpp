/*
 * Copyright (C) 2012 Motorola Mobility Inc.
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/fileapi/PublicURLManager.h"

#include "core/fileapi/URLRegistry.h"
#include "platform/blob/BlobData.h"
#include "platform/blob/BlobURL.h"
#include "platform/runtime_enabled_features.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/weborigin/URLSecurityOriginMap.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/ThreadSpecific.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/StringHash.h"

namespace blink {

namespace {

// When a blob URL is created in a unique origin the origin is serialized into
// the URL as "null". Since that makes it impossible to parse the origin back
// out and compare it against a context's origin (to check if a context is
// allowed to dereference the URL) we store a map of blob URL to SecurityOrigin
// instance for blob URLs with unique origins.

class BlobOriginMap : public URLSecurityOriginMap {
 public:
  BlobOriginMap();
  SecurityOrigin* GetOrigin(const KURL&) override;
};

typedef HashMap<String, scoped_refptr<SecurityOrigin>> BlobURLOriginMap;
static ThreadSpecific<BlobURLOriginMap>& OriginMap() {
  // We want to create the BlobOriginMap exactly once because it is shared by
  // all the threads.
  DEFINE_THREAD_SAFE_STATIC_LOCAL(BlobOriginMap, cache, ());
  (void)cache;  // BlobOriginMap's constructor does the interesting work.

  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<BlobURLOriginMap>, map, ());
  return map;
}

static void SaveToOriginMap(SecurityOrigin* origin, const KURL& url) {
  // If the blob URL contains null origin, as in the context with unique
  // security origin or file URL, save the mapping between url and origin so
  // that the origin can be retrieved when doing security origin check.
  //
  // See the definition of the origin of a Blob URL in the File API spec.
  if (origin && BlobURL::GetOrigin(url) == "null")
    OriginMap()->insert(url.GetString(), origin);
}

static void RemoveFromOriginMap(const KURL& url) {
  if (BlobURL::GetOrigin(url) == "null")
    OriginMap()->erase(url.GetString());
}

BlobOriginMap::BlobOriginMap() {
  SecurityOrigin::SetMap(this);
}

SecurityOrigin* BlobOriginMap::GetOrigin(const KURL& url) {
  if (url.ProtocolIs("blob"))
    return OriginMap()->at(url.GetString());
  return nullptr;
}

}  // namespace

PublicURLManager* PublicURLManager::Create(ExecutionContext* context) {
  return new PublicURLManager(context);
}

PublicURLManager::PublicURLManager(ExecutionContext* context)
    : ContextLifecycleObserver(context), is_stopped_(false) {}

String PublicURLManager::RegisterURL(URLRegistrable* registrable) {
  if (is_stopped_)
    return String();

  SecurityOrigin* origin = GetExecutionContext()->GetMutableSecurityOrigin();
  const KURL& url = BlobURL::CreatePublicURL(origin);
  DCHECK(!url.IsEmpty());
  const String& url_string = url.GetString();

  mojom::blink::BlobPtr blob;
  if (RuntimeEnabledFeatures::MojoBlobURLsEnabled())
    blob = registrable->AsMojoBlob();
  if (blob) {
    if (!url_store_) {
      BlobDataHandle::GetBlobRegistry()->URLStoreForOrigin(
          origin, MakeRequest(&url_store_));
    }
    url_store_->Register(std::move(blob), url);
    mojo_urls_.insert(url_string);
  } else {
    URLRegistry* registry = &registrable->Registry();
    registry->RegisterURL(origin, url, registrable);
    url_to_registry_.insert(url_string, registry);
  }
  SaveToOriginMap(origin, url);

  return url_string;
}

void PublicURLManager::Revoke(const KURL& url) {
  if (is_stopped_)
    return;
  // Don't bother trying to revoke URLs that can't have been registered anyway.
  if (!url.ProtocolIs("blob") || url.HasFragmentIdentifier())
    return;
  // Don't support revoking cross-origin blob URLs.
  if (!SecurityOrigin::Create(url)->IsSameSchemeHostPort(
          GetExecutionContext()->GetSecurityOrigin()))
    return;

  if (RuntimeEnabledFeatures::MojoBlobURLsEnabled()) {
    if (!url_store_) {
      BlobDataHandle::GetBlobRegistry()->URLStoreForOrigin(
          GetExecutionContext()->GetSecurityOrigin(), MakeRequest(&url_store_));
    }
    url_store_->Revoke(url);
    mojo_urls_.erase(url.GetString());
  }
  RemoveFromOriginMap(url);
  auto it = url_to_registry_.find(url.GetString());
  if (it == url_to_registry_.end())
    return;
  it->value->UnregisterURL(url);
  url_to_registry_.erase(it);
}

void PublicURLManager::ContextDestroyed(ExecutionContext*) {
  if (is_stopped_)
    return;

  is_stopped_ = true;
  for (auto& url_registry : url_to_registry_) {
    url_registry.value->UnregisterURL(KURL(url_registry.key));
    RemoveFromOriginMap(KURL(url_registry.key));
  }
  for (const auto& url : mojo_urls_)
    RemoveFromOriginMap(KURL(url));

  url_to_registry_.clear();
  mojo_urls_.clear();

  url_store_.reset();
}

void PublicURLManager::Trace(blink::Visitor* visitor) {
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
