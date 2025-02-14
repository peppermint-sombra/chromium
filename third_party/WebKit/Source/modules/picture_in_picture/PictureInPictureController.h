// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PictureInPictureController_h
#define PictureInPictureController_h

#include "core/frame/LocalFrame.h"
#include "modules/ModulesExport.h"

namespace blink {

class HTMLVideoElement;

// The PictureInPictureController is keeping the state and implementing the
// logic around the Picture-in-Picture feature. It is meant to be used as well
// by the Picture-in-Picture Web API and internally (eg. media controls). All
// consumers inside Blink should use this class to access Picture-in-Picture. A
// PictureInPictureController instance is associated to a Document. It is
// supplement and therefore can be lazy-initiated. Callers should consider
// whether they want to instantiate an object when they make a call.
class MODULES_EXPORT PictureInPictureController
    : public GarbageCollectedFinalized<PictureInPictureController>,
      public Supplement<Document> {
  USING_GARBAGE_COLLECTED_MIXIN(PictureInPictureController);
  WTF_MAKE_NONCOPYABLE(PictureInPictureController);

 public:
  virtual ~PictureInPictureController();

  static PictureInPictureController& Ensure(Document&);

  static const char* SupplementName();

  // Returns whether system allows Picture-in-Picture feature or not for
  // the associated document.
  bool PictureInPictureEnabled() const;

  void SetPictureInPictureEnabledForTesting(bool);

  // List of Picture-in-Picture support statuses. If status is kEnabled,
  // Picture-in-Picture is enabled for a document or element, otherwise it is
  // not supported.
  enum class Status {
    kEnabled,
    kFrameDetached,
    kDisabledBySystem,
    kDisabledByFeaturePolicy,
    kDisabledByAttribute,
  };

  // Returns whether the document associated with the controller is allowed to
  // request Picture-in-Picture.
  Status IsDocumentAllowed() const;

  // Returns whether a given video element in a document associated with the
  // controller is allowed to request Picture-in-Picture.
  Status IsElementAllowed(HTMLVideoElement&) const;

  // Meant to be called by HTMLVideoElementPictureInPicture and DOM objects
  // but not internally.
  void SetPictureInPictureElement(HTMLVideoElement&);

  // Meant to be called by DocumentPictureInPicture,
  // HTMLVideoElementPictureInPicture, and DOM objects but not internally.
  void UnsetPictureInPictureElement();

  // Returns element currently in Picture-in-Picture if any. Null otherwise.
  HTMLVideoElement* PictureInPictureElement() const;

  void Trace(blink::Visitor*) override;

 private:
  explicit PictureInPictureController(Document&);

  // Whether system allows Picture-in-Picture feature for the associated
  // document.
  bool picture_in_picture_enabled_ = true;

  // The Picture-in-Picture element for the associated document.
  Member<HTMLVideoElement> picture_in_picture_element_;
};

}  // namespace blink

#endif  // PictureInPictureController_h
