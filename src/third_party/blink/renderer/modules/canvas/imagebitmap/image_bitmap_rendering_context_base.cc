// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/canvas/imagebitmap/image_bitmap_rendering_context_base.h"

#include "third_party/blink/renderer/bindings/modules/v8/html_canvas_element_or_offscreen_canvas.h"
#include "third_party/blink/renderer/bindings/modules/v8/rendering_context.h"
#include "third_party/blink/renderer/core/imagebitmap/image_bitmap.h"
#include "third_party/blink/renderer/platform/graphics/gpu/image_layer_bridge.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace blink {

ImageBitmapRenderingContextBase::ImageBitmapRenderingContextBase(
    CanvasRenderingContextHost* host,
    const CanvasContextCreationAttributesCore& attrs)
    : CanvasRenderingContext(host, attrs),
      image_layer_bridge_(MakeGarbageCollected<ImageLayerBridge>(
          attrs.alpha ? kNonOpaque : kOpaque)) {}

ImageBitmapRenderingContextBase::~ImageBitmapRenderingContextBase() = default;

void ImageBitmapRenderingContextBase::getHTMLOrOffscreenCanvas(
    HTMLCanvasElementOrOffscreenCanvas& result) const {
  if (Host()->IsOffscreenCanvas()) {
    result.SetOffscreenCanvas(static_cast<OffscreenCanvas*>(Host()));
  } else {
    result.SetHTMLCanvasElement(static_cast<HTMLCanvasElement*>(Host()));
  }
}

void ImageBitmapRenderingContextBase::Stop() {
  image_layer_bridge_->Dispose();
}

void ImageBitmapRenderingContextBase::SetImage(ImageBitmap* image_bitmap) {
  DCHECK(!image_bitmap || !image_bitmap->IsNeutered());

  image_layer_bridge_->SetImage(image_bitmap ? image_bitmap->BitmapImage()
                                             : nullptr);

  DidDraw();

  if (image_bitmap)
    image_bitmap->close();
}

scoped_refptr<StaticBitmapImage> ImageBitmapRenderingContextBase::GetImage(
    AccelerationHint) const {
  return image_layer_bridge_->GetImage();
}

scoped_refptr<StaticBitmapImage>
ImageBitmapRenderingContextBase::GetImageAndResetInternal() {
  if (!image_layer_bridge_->GetImage())
    return nullptr;
  scoped_refptr<StaticBitmapImage> copy_image = image_layer_bridge_->GetImage();

  SkBitmap black_bitmap;
  black_bitmap.allocN32Pixels(copy_image->width(), copy_image->height());
  black_bitmap.eraseARGB(0, 0, 0, 0);
  image_layer_bridge_->SetImage(
      StaticBitmapImage::Create(SkImage::MakeFromBitmap(black_bitmap)));

  return copy_image;
}

void ImageBitmapRenderingContextBase::SetUV(const FloatPoint& left_top,
                                            const FloatPoint& right_bottom) {
  image_layer_bridge_->SetUV(left_top, right_bottom);
}

cc::Layer* ImageBitmapRenderingContextBase::CcLayer() const {
  return image_layer_bridge_->CcLayer();
}

bool ImageBitmapRenderingContextBase::IsPaintable() const {
  return !!image_layer_bridge_->GetImage();
}

void ImageBitmapRenderingContextBase::Trace(blink::Visitor* visitor) {
  visitor->Trace(image_layer_bridge_);
  CanvasRenderingContext::Trace(visitor);
}

bool ImageBitmapRenderingContextBase::IsAccelerated() const {
  return image_layer_bridge_->IsAccelerated();
}

bool ImageBitmapRenderingContextBase::CanCreateCanvas2dResourceProvider()
    const {
  DCHECK(Host());
  DCHECK(Host()->IsOffscreenCanvas());
  return !!static_cast<OffscreenCanvas*>(Host())->GetOrCreateResourceProvider();
}

void ImageBitmapRenderingContextBase::PushFrame() {
  DCHECK(Host());
  DCHECK(Host()->IsOffscreenCanvas());
  if (!CanCreateCanvas2dResourceProvider())
    return;

  scoped_refptr<StaticBitmapImage> image = image_layer_bridge_->GetImage();
  cc::PaintFlags paint_flags;
  paint_flags.setBlendMode(SkBlendMode::kSrc);
  Host()->ResourceProvider()->Canvas()->drawImage(
      image->PaintImageForCurrentFrame(), 0, 0, &paint_flags);
  scoped_refptr<CanvasResource> resource =
      Host()->ResourceProvider()->ProduceCanvasResource();
  Host()->PushFrame(
      std::move(resource),
      SkIRect::MakeWH(image_layer_bridge_->GetImage()->Size().Width(),
                      image_layer_bridge_->GetImage()->Size().Height()));
}

bool ImageBitmapRenderingContextBase::IsOriginTopLeft() const {
  if (Host()->IsOffscreenCanvas())
    return false;
  return IsAccelerated();
}

}  // namespace blink
