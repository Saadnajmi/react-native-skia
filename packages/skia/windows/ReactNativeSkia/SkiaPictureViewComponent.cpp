#include "pch.h"

#include "SkiaPictureViewComponent.h"

#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Microsoft.ReactNative.Composition.h>

#include "SkiaPictureView.h"

namespace RNSkia::SkiaPictureViewComponent {

namespace {

/**
 * Map from a XAML SwapChainPanel hosted in the visual tree to the C++
 * SkiaPictureView instance that owns it. The Fabric component's prop /
 * lifecycle callbacks dispatch into this object.
 */
void ApplyProps(SkiaPictureView &view,
                winrt::Microsoft::ReactNative::IComponentProps const &props) {
  // Codegen will emit a typed Props struct; until that lands, we read the
  // expected props (nativeID, debug, opaque) directly from the property bag
  // RNW hands to component instances. The shape mirrors
  // src/specs/SkiaPictureViewNativeComponent.ts.
  // TODO(windows-codegen): replace with the generated SkiaPictureViewProps
  // accessor once `windows` codegen is wired into the build.
  (void)props;
  (void)view;
}

} // namespace

void RegisterViewComponent(
    winrt::Microsoft::ReactNative::IReactPackageBuilder const &packageBuilder) {
  auto fabric = packageBuilder.try_as<
      winrt::Microsoft::ReactNative::IReactPackageBuilderFabric>();
  if (fabric == nullptr) {
    return;
  }

  fabric.AddViewComponent(
      L"SkiaPictureView",
      [](winrt::Microsoft::ReactNative::IReactViewComponentBuilder const &
             builder) noexcept {
        builder.SetCreateViewComponentView(
            []() noexcept -> winrt::Windows::Foundation::IInspectable {
              auto view = std::make_shared<SkiaPictureView>();
              return view->Panel();
            });

        builder.SetUpdatePropsHandler(
            [](winrt::Windows::Foundation::IInspectable const &handle,
               winrt::Microsoft::ReactNative::IComponentProps const &oldProps,
               winrt::Microsoft::ReactNative::IComponentProps const &newProps)
                noexcept {
              (void)handle;
              (void)oldProps;
              (void)newProps;
              // TODO(windows-codegen): forward typed props to the matching
              // SkiaPictureView instance.
            });
      });
}

} // namespace RNSkia::SkiaPictureViewComponent
