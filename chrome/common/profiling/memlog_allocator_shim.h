// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PROFILING_MEMLOG_ALLOCATOR_SHIM_H_
#define CHROME_COMMON_PROFILING_MEMLOG_ALLOCATOR_SHIM_H_

#include "chrome/common/profiling/memlog_sender_pipe.h"
#include "chrome/common/profiling/memlog_stream.h"
#include "chrome/common/profiling/profiling_client.mojom.h"

namespace profiling {

// Initializes the TLS slot globally. This will be called early in Chrome's
// lifecycle to prevent re-entrancy from occurring while trying to set up the
// TLS slot, which is the entity that's supposed to prevent re-entrancy.
void InitTLSSlot();

// Begin profiling all allocations in the process. Send the results to
// |sender_pipe|. |stack_mode| describes the type of stack to record for each
// allocation. |include_thread_names| describes whether to insert thread names
// into NATIVE stacks.
void InitAllocatorShim(MemlogSenderPipe* sender_pipe,
                       mojom::StackMode stack_mode);

// Stop profiling allocations by dropping shim callbacks. There is no way to
// consistently, synchronously stop the allocator shim without negatively
// impacting fast-path performance. This method eventually "turns off" the
// allocator shim by turning future calls to AllocatorShimLogAlloc and
// AllocatorShimLogFree into no-ops, modulo caching [g_send_buffers is not
// volatile, intentionally]. This method is well-defined, but isn't guaranteed
// to stop all messages to sender_pipe, since another thread might already be in
// the process of forming a message.
void StopAllocatorShimDangerous();

// Logs an allocation. The context is a null-terminated string of
// allocator-specific context information. It can be null if there is no
// context.
void AllocatorShimLogAlloc(AllocatorType type,
                           void* address,
                           size_t sz,
                           const char* context);

void AllocatorShimLogFree(void* address);

// Ensures all send buffers are flushed. The given barrier ID is sent to the
// logging process so it knows when this operation is complete.
void AllocatorShimFlushPipe(uint32_t barrier_id);

// Sets the functions that can be called to hook GC heap allocations. These
// must be set externally since GC heap only exists in renderer processes. If
// set, these functions functions will be called to enable logging of the GC
// heap.
using SetGCAllocHookFunction = void (*)(void (*)(uint8_t*,
                                                 size_t,
                                                 const char*));
using SetGCFreeHookFunction = void (*)(void (*)(uint8_t*));
void SetGCHeapAllocationHookFunctions(SetGCAllocHookFunction hook_alloc,
                                      SetGCFreeHookFunction hook_free);

// Exists for testing only. |callback| is called on |task_runner| after the
// allocator shim is initialized.
void SetOnInitAllocatorShimCallbackForTesting(
    base::OnceClosure callback,
    scoped_refptr<base::TaskRunner> task_runner);

void DisableAllocationTrackingForCurrentThreadForTesting();
void EnableAllocationTrackingForCurrentThreadForTesting();

}  // namespace profiling

#endif  // CHROME_COMMON_PROFILING_MEMLOG_ALLOCATOR_SHIM_H_
