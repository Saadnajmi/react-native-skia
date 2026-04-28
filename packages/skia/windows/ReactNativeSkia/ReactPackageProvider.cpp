#include "pch.h"

#include "ReactPackageProvider.h"
#include "RNSkiaModule.h"
#include "SkiaPictureViewComponent.h"

#include <NativeModules.h>

namespace winrt::ReactNativeSkia::implementation {

void ReactPackageProvider::CreatePackage(
    winrt::Microsoft::ReactNative::IReactPackageBuilder const &packageBuilder)
    noexcept {
  AddAttributedModules(packageBuilder, true);
  RNSkia::SkiaPictureViewComponent::RegisterViewComponent(packageBuilder);
}

} // namespace winrt::ReactNativeSkia::implementation
