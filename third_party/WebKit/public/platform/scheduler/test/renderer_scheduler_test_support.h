// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_PUBLIC_PLATFORM_SCHEDULER_TEST_RENDERER_SCHEDULER_TEST_SUPPORT_H_
#define THIRD_PARTY_WEBKIT_PUBLIC_PLATFORM_SCHEDULER_TEST_RENDERER_SCHEDULER_TEST_SUPPORT_H_

#include "base/bind.h"
#include "base/memory/ptr_util.h"

namespace base {
class SequencedTaskRunner;
class SingleThreadTaskRunner;
}  // namespace base

namespace blink {

namespace scheduler {

class RendererScheduler;

std::unique_ptr<RendererScheduler> CreateRendererSchedulerForTests();

void RunIdleTasksForTesting(RendererScheduler* scheduler,
                            const base::Closure& callback);

// Returns a SequencedTaskRunner. This implementation is same as
// SequencedTaskRunnerHandle::Get(), but this is intended to be used for
// testing. See crbug.com/794123.
scoped_refptr<base::SequencedTaskRunner> GetSequencedTaskRunnerForTesting();

// Returns the SingleThreadTaskRunner for the current thread for testing. This
// implementation is same as ThreadTaskRunnerHandle::Get(), but this is intended
// to be used for testing. See crbug.com/794123.
scoped_refptr<base::SingleThreadTaskRunner>
GetSingleThreadTaskRunnerForTesting();

// Creates a task runner for testing.
scoped_refptr<base::SingleThreadTaskRunner> CreateWebTaskRunnerForTesting();

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_PUBLIC_PLATFORM_SCHEDULER_TEST_RENDERER_SCHEDULER_TEST_SUPPORT_H_
