#pragma once

#include "pch.h"

#include <winrt/Microsoft.ReactNative.h>

namespace winrt::ReactNativeSkia::implementation {

struct ReactPackageProvider
    : winrt::implements<ReactPackageProvider,
                        winrt::Microsoft::ReactNative::IReactPackageProvider> {
  ReactPackageProvider() = default;

  void CreatePackage(
      winrt::Microsoft::ReactNative::IReactPackageBuilder const &packageBuilder)
      noexcept;
};

} // namespace winrt::ReactNativeSkia::implementation
