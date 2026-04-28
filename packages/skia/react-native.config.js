// Tells the React Native CLI / RNW autolinker where to find the Windows
// native module so a consuming app picks up SkiaPictureView and the
// RNSkiaModule TurboModule without manual project references. Other
// platforms (iOS, macOS, Android) keep using their default discovery
// paths — the CLI only consults this file for keys it doesn't already
// have heuristics for.
module.exports = {
  dependency: {
    platforms: {
      windows: {
        sourceDir: "windows",
        solutionFile: "ReactNativeSkia.sln",
        projects: [
          {
            projectFile: "ReactNativeSkia\\ReactNativeSkia.vcxproj",
            directDependency: true,
          },
        ],
      },
    },
  },
};
