#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <ReactCommon/CallInvoker.h>
#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Windows.UI.Core.h>

#include "RNSkPlatformContext.h"

namespace RNSkia {

/**
 * Windows implementation of RNSkPlatformContext.
 *
 * Threading: tasks scheduled via runOnMainThread() are dispatched onto the
 * UI thread's CoreDispatcher captured at construction. The JS thread is
 * accessed via the CallInvoker provided by Microsoft.ReactNative.
 *
 * GPU: in the default Ganesh build (SK_DIRECT3D), Skia surfaces are produced
 * by D3D12WindowContext. In the Graphite build (SK_GRAPHITE) they're produced
 * by DawnContext (cpp/rnskia/RNDawnContext.h, shared with Android).
 */
class RNSkWindowsPlatformContext : public RNSkPlatformContext {
public:
  RNSkWindowsPlatformContext(
      winrt::Microsoft::ReactNative::ReactContext const &reactContext,
      std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
      float pixelDensity);

  ~RNSkWindowsPlatformContext() override = default;

  void runOnMainThread(std::function<void()> func) override;

  sk_sp<SkImage> takeScreenshotFromViewTag(size_t tag) override;

  void performStreamOperation(
      const std::string &sourceUri,
      const std::function<void(std::unique_ptr<SkStreamAsset>)> &op) override;

  void raiseError(const std::exception &err) override;

  sk_sp<SkSurface> makeOffscreenSurface(int width, int height,
                                        bool useP3ColorSpace = false) override;

  std::shared_ptr<WindowContext>
  makeContextFromNativeSurface(void *surface, int width, int height) override;

  sk_sp<SkImage> makeImageFromNativeBuffer(void *buffer) override;

#if !defined(SK_GRAPHITE)
  sk_sp<SkImage> makeImageFromNativeTexture(const TextureInfo &textureInfo,
                                            int width, int height,
                                            bool mipMapped) override;

  const TextureInfo getTexture(sk_sp<SkSurface> surface) override;

  const TextureInfo getTexture(sk_sp<SkImage> image) override;

  GrDirectContext *getDirectContext() override;
#endif

  void releaseNativeBuffer(uint64_t pointer) override;

  uint64_t makeNativeBuffer(sk_sp<SkImage> image) override;

  std::shared_ptr<RNSkVideo> createVideo(const std::string &url) override;

  sk_sp<SkFontMgr> createFontMgr() override;

  std::vector<std::string> getSystemFontFamilies() override;

  std::string resolveFontFamily(const std::string &familyName) override;

private:
  winrt::Microsoft::ReactNative::ReactContext _reactContext;
  winrt::Windows::UI::Core::CoreDispatcher _uiDispatcher{nullptr};
};

} // namespace RNSkia
