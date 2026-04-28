#pragma once

#include <memory>

#include <NativeModules.h>
#include <winrt/Microsoft.ReactNative.h>

#include "SkiaManager.h"

namespace RNSkia {

/**
 * TurboModule entry point exposed to RN as `RNSkiaModule`. The module's
 * `install()` method is invoked from JS during library initialization;
 * it constructs the SkiaManager which in turn binds the JSI Skia API onto
 * the JS runtime.
 */
REACT_MODULE(RNSkiaModule, L"RNSkiaModule")
struct RNSkiaModule {
  REACT_INIT(Initialize)
  void Initialize(
      winrt::Microsoft::ReactNative::ReactContext const &reactContext) noexcept;

  REACT_SYNC_METHOD(install)
  bool install() noexcept;

private:
  winrt::Microsoft::ReactNative::ReactContext _reactContext{nullptr};
  std::unique_ptr<SkiaManager> _skiaManager;
};

} // namespace RNSkia
