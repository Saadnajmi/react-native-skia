#pragma once

#include <memory>

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <winrt/Microsoft.ReactNative.h>

#include "RNSkManager.h"
#include "RNSkWindowsPlatformContext.h"

namespace RNSkia {

/**
 * Holds the RNSkManager + RNSkWindowsPlatformContext for the lifetime of a
 * React instance. SkiaPictureView instances obtain the platform context from
 * here when they need a WindowContext.
 *
 * One instance per React instance, owned by the RNSkiaModule. SkiaManager
 * itself is process-wide accessible via the static getInstance(); the static
 * pointer is reset whenever the JS runtime is torn down.
 */
class SkiaManager {
public:
  static SkiaManager *getInstance();

  SkiaManager(winrt::Microsoft::ReactNative::ReactContext const &reactContext,
              facebook::jsi::Runtime *runtime,
              std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
              float pixelDensity);

  ~SkiaManager();

  std::shared_ptr<RNSkPlatformContext> getPlatformContext() const {
    return _platformContext;
  }

  RNSkManager *getManager() const { return _manager.get(); }

private:
  std::shared_ptr<RNSkPlatformContext> _platformContext;
  std::unique_ptr<RNSkManager> _manager;
};

} // namespace RNSkia
