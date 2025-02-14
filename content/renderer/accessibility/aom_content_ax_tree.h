// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_
#define CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_

#include <stdint.h>

#include "third_party/WebKit/public/platform/WebComputedAXTree.h"

#include "base/macros.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "ui/accessibility/ax_tree.h"

namespace content {

class AomContentAxTree : public blink::WebComputedAXTree {
 public:
  explicit AomContentAxTree(RenderFrameImpl* render_frame);

  // blink::WebComputedAXTree implementation.
  bool ComputeAccessibilityTree() override;
  bool GetRoleForAXNode(int32_t ax_id, blink::WebString* out_param) override;
  bool GetStringAttributeForAXNode(int32_t,
                                   blink::WebAOMStringAttribute,
                                   blink::WebString* out_param) override;
  bool GetIntAttributeForAXNode(int32_t ax_id,
                                blink::WebAOMIntAttribute,
                                int32_t* out_param) override;

  bool GetParentIdForAXNode(int32_t ax_id, int32_t* out_param) override;
  bool GetFirstChildIdForAXNode(int32_t ax_id, int32_t* out_param) override;
  bool GetLastChildIdForAXNode(int32_t ax_id, int32_t* out_param) override;
  bool GetPreviousSiblingIdForAXNode(int32_t ax_id,
                                     int32_t* out_param) override;
  bool GetNextSiblingIdForAXNode(int32_t ax_id, int32_t* out_param) override;

 private:
  ui::AXTree tree_;
  RenderFrameImpl* render_frame_;
  DISALLOW_COPY_AND_ASSIGN(AomContentAxTree);
};

}  // namespace content

#endif  // CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_
