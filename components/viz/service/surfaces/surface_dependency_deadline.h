// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_DEPENDENCY_DEADLINE_H_
#define COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_DEPENDENCY_DEADLINE_H_

#include "components/viz/common/frame_sinks/begin_frame_source.h"

#include "components/viz/service/surfaces/surface_deadline_client.h"

namespace base {
class TickClock;
}  // namespace base

namespace viz {

class SurfaceDependencyDeadline : public BeginFrameObserver {
 public:
  SurfaceDependencyDeadline(SurfaceDeadlineClient* client,
                            BeginFrameSource* begin_frame_source,
                            base::TickClock* tick_clock);
  ~SurfaceDependencyDeadline() override;

  // Sets up a deadline in wall time where
  // deadline = frame_time + number_of_frames_to_deadline * frame_interval.
  // It's possible for the deadline to already be in the past. In that case,
  // this method will return false. Otherwise, it will return true.
  bool Set(base::TimeTicks frame_time,
           uint32_t number_of_frames_to_deadline,
           base::TimeDelta frame_interval);

  // If a deadline had been set, then cancel the deadline and return the
  // the duration of the event tracked by this object. If there was no
  // deadline set, then return base::nullopt.
  base::Optional<base::TimeDelta> Cancel();

  bool has_deadline() const { return deadline_.has_value(); }

  // Takes on the same BeginFrameSource and deadline as |other|. Returns
  // false if they're already the same, and true otherwise.
  bool InheritFrom(const SurfaceDependencyDeadline& other);

  bool operator==(const SurfaceDependencyDeadline& other);
  bool operator!=(const SurfaceDependencyDeadline& other) {
    return !(*this == other);
  }

  // BeginFrameObserver implementation.
  void OnBeginFrame(const BeginFrameArgs& args) override;
  const BeginFrameArgs& LastUsedBeginFrameArgs() const override;
  void OnBeginFrameSourcePausedChanged(bool paused) override;
  bool WantsAnimateOnlyBeginFrames() const override;

 private:
  base::Optional<base::TimeDelta> CancelInternal(bool deadline);

  SurfaceDeadlineClient* const client_;
  BeginFrameSource* begin_frame_source_ = nullptr;
  base::TickClock* tick_clock_;
  base::TimeTicks start_time_;
  base::Optional<base::TimeTicks> deadline_;

  BeginFrameArgs last_begin_frame_args_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceDependencyDeadline);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_DEPENDENCY_DEADLINE_H_
