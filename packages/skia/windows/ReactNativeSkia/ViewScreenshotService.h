#pragma once

#include <memory>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include "include/core/SkImage.h"

#pragma clang diagnostic pop

namespace RNSkia {

/**
 * Capture an SkImage of a Fabric view's visual tree.
 *
 * Phase 1 returns nullptr; Phase 2 will use the
 * Microsoft.UI.Xaml.Media.Imaging.RenderTargetBitmap analog
 * (or DXGI/Direct3D 12 surface readback) to produce a raster snapshot.
 */
class ViewScreenshotService {
public:
  static sk_sp<SkImage> ScreenshotForViewTag(size_t viewTag);
};

} // namespace RNSkia
