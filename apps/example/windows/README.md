# Example app — Windows

The Windows host for the example app is generated on demand by
[`react-native-test-app`](https://github.com/microsoft/react-native-test-app),
the same harness used for the iOS, macOS, and Android hosts.

The committed contents of this directory are intentionally minimal:

- `.gitignore` — excludes the `*.sln`, `*.vcxproj`, `Generated Files/`,
  `packages/`, and other build outputs that `init-test-app` produces.
- `README.md` — this file.

## First-time setup on a Windows machine

```pwsh
# From the repo root
yarn

# Generate the test-app Windows host into apps/example/windows/
cd apps\example
npx --yes react-native-test-app init --platform windows --destination .

# Restore NuGet packages and build
cd windows
nuget restore example.sln
msbuild example.sln /p:Configuration=Debug /p:Platform=x64
```

After that, `npx react-native run-windows --arch x64` (run from
`apps/example/`) builds the example app and launches it. The Skia native
module under `packages/skia/windows/ReactNativeSkia/` is autolinked via
`react-native.config.js` (already configured in this repo).

## Skia binaries

The example app expects prebuilt Skia static libraries at
`packages/skia/libs/windows/{x64,arm64}/`. Build them locally with:

```pwsh
cd packages\skia
yarn build-skia windows
```

A long-term plan is to publish a sibling `react-native-skia-windows` npm
package (parallel to `react-native-skia-apple-macos`) that ships these
prebuilts, mirroring the iOS / macOS / Android distribution model.
