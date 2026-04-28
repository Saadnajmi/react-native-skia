#include "pch.h"

#include "RNSkiaModule.h"

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>

#include <winrt/Windows.Graphics.Display.h>

namespace RNSkia {

void RNSkiaModule::Initialize(
    winrt::Microsoft::ReactNative::ReactContext const &reactContext) noexcept {
  _reactContext = reactContext;
}

bool RNSkiaModule::install() noexcept {
  try {
    auto jsDispatcher = _reactContext.JSDispatcher();
    auto callInvoker = winrt::Microsoft::ReactNative::ReactNonAbiValue<
        std::shared_ptr<facebook::react::CallInvoker>>::GetPtr(
        _reactContext.Properties().Get(
            winrt::Microsoft::ReactNative::ReactPropertyName(
                L"ReactNative.JSCallInvoker")));

    auto runtimeHandle =
        winrt::Microsoft::ReactNative::ReactPropertyName(L"ReactNative.RuntimeHandle");
    auto runtime = static_cast<facebook::jsi::Runtime *>(
        winrt::Microsoft::ReactNative::ReactNonAbiValue<void *>::GetPtr(
            _reactContext.Properties().Get(runtimeHandle)));

    if (runtime == nullptr || callInvoker == nullptr) {
      return false;
    }

    float pixelDensity = 1.0f;
    try {
      auto info =
          winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
      pixelDensity = static_cast<float>(info.RawPixelsPerViewPixel());
    } catch (...) {
      pixelDensity = 1.0f;
    }

    _skiaManager = std::make_unique<SkiaManager>(_reactContext, runtime,
                                                 *callInvoker, pixelDensity);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace RNSkia
