// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/native_view_accessibility_win.h"

#include <oleacc.h>

#include <memory>
#include <set>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "third_party/iaccessible2/ia2_api_all.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_text_utils.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/layout.h"
#include "ui/base/win/accessibility_misc_utils.h"
#include "ui/base/win/atl_module.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"
#include "ui/wm/core/window_util.h"

namespace views {

namespace {

// Return the parent of |window|, first checking to see if it has a
// transient parent. This allows us to walk up the aura::Window
// hierarchy when it spans multiple window tree hosts, each with
// their own HWND.
aura::Window* GetWindowParentIncludingTransient(aura::Window* window) {
  aura::Window* transient_parent = wm::GetTransientParent(window);
  if (transient_parent)
    return transient_parent;

  return window->parent();
}

}  // namespace

// static
std::unique_ptr<ViewAccessibility> ViewAccessibility::Create(View* view) {
  return std::make_unique<NativeViewAccessibilityWin>(view);
}

NativeViewAccessibilityWin::NativeViewAccessibilityWin(View* view)
    : NativeViewAccessibilityBase(view) {}

NativeViewAccessibilityWin::~NativeViewAccessibilityWin() {}

gfx::NativeViewAccessible NativeViewAccessibilityWin::GetParent() {
  // If the View has a parent View, return that View's IAccessible.
  if (view()->parent())
    return view()->parent()->GetNativeViewAccessible();

  // Otherwise we must be the RootView, get the corresponding Widget
  // and Window.
  Widget* widget = view()->GetWidget();
  if (!widget)
    return nullptr;

  aura::Window* window = widget->GetNativeWindow();
  if (!window)
    return nullptr;

  // Look for an ancestor window with a Widget, and if found, return
  // the NativeViewAccessible for its RootView.
  aura::Window* ancestor_window = GetWindowParentIncludingTransient(window);
  while (ancestor_window) {
    Widget* ancestor_widget = Widget::GetWidgetForNativeView(ancestor_window);
    if (ancestor_widget && ancestor_widget->GetRootView())
      return ancestor_widget->GetRootView()->GetNativeViewAccessible();
    ancestor_window = GetWindowParentIncludingTransient(ancestor_window);
  }

  // If that fails, return the NativeViewAccessible for our owning HWND.
  HWND hwnd = HWNDForView(view());
  if (!hwnd)
    return nullptr;

  IAccessible* parent;
  if (SUCCEEDED(
          ::AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, IID_IAccessible,
                                       reinterpret_cast<void**>(&parent))))
    return parent;

  return nullptr;
}

gfx::AcceleratedWidget
NativeViewAccessibilityWin::GetTargetForNativeAccessibilityEvent() {
  return HWNDForView(view());
}

gfx::RectF NativeViewAccessibilityWin::GetBoundsInScreen() const {
  gfx::RectF bounds = gfx::RectF(view()->GetBoundsInScreen());
  gfx::NativeView native_view = view()->GetWidget()->GetNativeView();
  float device_scale = ui::GetScaleFactorForNativeView(native_view);
  bounds.Scale(device_scale);
  return bounds;
}

}  // namespace views
