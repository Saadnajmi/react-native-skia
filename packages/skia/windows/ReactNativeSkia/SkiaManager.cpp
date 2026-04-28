#include "pch.h"

#include "SkiaManager.h"

namespace RNSkia {

namespace {
SkiaManager *g_instance = nullptr;
} // namespace

SkiaManager *SkiaManager::getInstance() { return g_instance; }

SkiaManager::SkiaManager(
    winrt::Microsoft::ReactNative::ReactContext const &reactContext,
    facebook::jsi::Runtime *runtime,
    std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
    float pixelDensity) {
  _platformContext = std::make_shared<RNSkWindowsPlatformContext>(
      reactContext, jsCallInvoker, pixelDensity);
  _manager =
      std::make_unique<RNSkManager>(runtime, jsCallInvoker, _platformContext);
  g_instance = this;
}

SkiaManager::~SkiaManager() {
  if (g_instance == this) {
    g_instance = nullptr;
  }
}

} // namespace RNSkia
