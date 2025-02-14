// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PictureInPictureInterstitial_h
#define PictureInPictureInterstitial_h

#include "core/html/HTMLDivElement.h"
#include "platform/Timer.h"

namespace blink {

class HTMLImageElement;
class HTMLVideoElement;
// class PictureInPictureMessageElement;

// Picture in Picture UI. DOM structure looks like:
//
// PictureInPictureInterstitial
//     (-internal-picture-in-picture-interstitial)
// +-HTMLImageElement
// |    (-internal-picture-in-picture-background-image)
// \-PictureInPictureMessageElement
// |    (-internal-picture-in-picture-message)
class PictureInPictureInterstitial final : public HTMLDivElement {
 public:
  explicit PictureInPictureInterstitial(HTMLVideoElement&);

  void Show();
  void Hide();

  void OnPosterImageChanged();
  bool IsVisible() const { return should_be_visible_; }

  HTMLVideoElement& GetVideoElement() const { return *video_element_; }

  // Element:
  void Trace(blink::Visitor*) override;

 private:
  // Node override.
  bool IsPictureInPictureInterstitial() const override { return true; }

  void ToggleInterstitialTimerFired(TimerBase*);

  // Indicates whether the interstitial should be visible. It is updated
  // when Show()/Hide() is called.
  bool should_be_visible_ = false;

  TaskRunnerTimer<PictureInPictureInterstitial> interstitial_timer_;
  Member<HTMLVideoElement> video_element_;
  Member<HTMLImageElement> background_image_;
  Member<HTMLDivElement> message_element_;
};

}  // namespace blink

#endif  // PictureInPictureInterstitial_h
