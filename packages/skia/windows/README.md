# React Native Skia for Windows

This directory contains the React Native Windows port of `@shopify/react-native-skia`.
It mirrors the role of `apple/` (iOS, macOS, tvOS) and `android/` (Android) and
provides:

- A Visual Studio C++/WinRT project (`ReactNativeSkia/ReactNativeSkia.vcxproj`)
  that integrates with React Native Windows via `Microsoft.ReactNative`.
- `RNSkWindowsPlatformContext`, the Windows implementation of
  `RNSkPlatformContext` declared in `cpp/rnskia/RNSkPlatformContext.h`.
- A `D3D12WindowContext` (Ganesh) that hosts Skia rendering on a
  `SwapChainPanel` via Direct3D 12.
- The `SkiaPictureView` Fabric component registered with
  `Microsoft.ReactNative`.

## Renderer

Two backends are supported, switched via the `SK_GRAPHITE` compile-time flag:

- **Ganesh + Direct3D 12** (default): `D3D12WindowContext` drives Skia's native
  D3D12 backend.
- **Graphite + Dawn**: reuses the cross-platform `cpp/rnskia/RNDawnContext.h`
  pipeline already used on Android. On Windows Dawn defaults to D3D12.

ANGLE / OpenGL is intentionally not used.

## Targets

- New Architecture (Fabric) only.
- React Native Windows 0.81 or later.
- Windows 10 19041 (May 2020) or later.
- x64 and arm64.

## Building Skia for Windows

```sh
cd packages/skia
yarn build-skia windows
```

Outputs land in `packages/skia/libs/windows/{x64,arm64}/`. Phase 1 consumes
these binaries from a local checkout. A sibling `react-native-skia-windows`
npm package (parallel to `react-native-skia-apple-macos`) is the long-term
plan for distribution.
