// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDER_WIDGET_SCREEN_METRICS_EMULATOR_H_
#define CONTENT_RENDERER_RENDER_WIDGET_SCREEN_METRICS_EMULATOR_H_

#include <memory>

#include "content/common/content_export.h"
#include "content/public/common/screen_info.h"
#include "third_party/blink/public/web/web_device_emulation_params.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace content {
class RenderWidgetScreenMetricsEmulatorDelegate;
struct ContextMenuParams;

// RenderWidgetScreenMetricsEmulator class manages screen emulation inside a
// RenderWidget. This includes resizing, placing view on the screen at desired
// position, changing device scale factor, and scaling down the whole
// widget if required to fit into the browser window.
class CONTENT_EXPORT RenderWidgetScreenMetricsEmulator {
 public:
  RenderWidgetScreenMetricsEmulator(
      RenderWidgetScreenMetricsEmulatorDelegate* delegate,
      const ScreenInfo& screen_info,
      const gfx::Size& widget_size,
      const gfx::Size& visible_viewport_size,
      const gfx::Rect& view_screen_rect,
      const gfx::Rect& window_screen_rect);
  ~RenderWidgetScreenMetricsEmulator();

  const gfx::Size& original_size() const { return original_widget_size_; }
  const ScreenInfo& original_screen_info() const {
    return original_screen_info_;
  }
  const gfx::Rect& original_screen_rect() const {
    return original_view_screen_rect_;
  }

  float scale() const { return scale_; }
  const gfx::Rect& applied_widget_rect() const { return applied_widget_rect_; }

  // Disables emulation and applies non-emulated values to the RenderWidget.
  // Call this before destroying the RenderWidgetScreenMetricsEmulator.
  void DisableAndApply();

  // Sets new parameters and applies them to the RenderWidget.
  void ChangeEmulationParams(const blink::WebDeviceEmulationParams& params);

  void OnSynchronizeVisualProperties(const ScreenInfo& screen_info,
                                     const gfx::Size& widget_size,
                                     const gfx::Size& visible_viewport_size);
  void OnUpdateScreenRects(const gfx::Rect& view_screen_rect,
                           const gfx::Rect& window_screen_rect);

  // Modify ContextMenuParams to account for emulation.
  void OnShowContextMenu(ContextMenuParams* params);

 private:
  RenderWidgetScreenMetricsEmulatorDelegate* const delegate_;

  // Applies emulated values to the RenderWidget.
  void Apply();

  // Parameters as passed by RenderWidget::EnableScreenMetricsEmulation.
  blink::WebDeviceEmulationParams emulation_params_;

  // The computed scale and offset used to fit widget into browser window.
  float scale_ = 1;

  // Widget rect as passed to webwidget.
  gfx::Rect applied_widget_rect_;

  // Original values to restore back after emulation ends.
  ScreenInfo original_screen_info_;
  gfx::Size original_widget_size_;
  gfx::Size original_visible_viewport_size_;
  gfx::Rect original_view_screen_rect_;
  gfx::Rect original_window_screen_rect_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetScreenMetricsEmulator);
};

}  // namespace content

#endif  // CONTENT_RENDERER_RENDER_WIDGET_SCREEN_METRICS_EMULATOR_H_
