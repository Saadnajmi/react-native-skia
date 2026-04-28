#pragma once

#include <winrt/Microsoft.ReactNative.h>

namespace RNSkia::SkiaPictureViewComponent {

/**
 * Registers the `SkiaPictureView` Fabric component with the package builder.
 *
 * The exact registration API used here (AddViewManagerProvider /
 * IReactPackageBuilderFabric / IReactViewComponentBuilder) is RNW-version
 * dependent. The implementation in SkiaPictureViewComponent.cpp targets
 * RNW 0.81+ (Fabric, new architecture only). When RNW publishes a stable
 * Fabric component API the implementation will simplify accordingly.
 */
void RegisterViewComponent(
    winrt::Microsoft::ReactNative::IReactPackageBuilder const &packageBuilder);

} // namespace RNSkia::SkiaPictureViewComponent
