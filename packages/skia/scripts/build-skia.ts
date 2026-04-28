import { exit } from "process";
import fs from "fs";
import path from "path";

import type {
  ApplePlatformName,
  Platform,
  PlatformName,
} from "./skia-configuration";
import {
  commonArgs,
  configurations,
  copyHeaders,
  GRAPHITE,
  isApplePlatform,
  isWindowsPlatform,
  MACCATALYST,
  OutFolder,
  PackageRoot,
  ProjectRoot,
  SkiaSrc,
} from "./skia-configuration";
import { $, mapKeys, runAsync } from "./utils";

const getOutDir = (platform: PlatformName, targetName: string) => {
  return `${OutFolder}/${platform}/${targetName}`;
};

const configurePlatform = async (
  platformName: PlatformName,
  configuration: Platform,
  targetName: string
) => {
  process.chdir(SkiaSrc);
  console.log(
    `Configuring platform "${platformName}" for target "${targetName}"`
  );
  console.log("Current directory", process.cwd());

  if (configuration) {
    const target = configuration.targets[targetName];
    if (!target) {
      console.log(
        `Target ${targetName} not found for platform ${platformName}`
      );
      exit(1);
    }

    const commandline = `PATH=../depot_tools/:$PATH gn gen ${getOutDir(
      platformName,
      targetName
    )}`;

    const args = configuration.args.reduce(
      (a, cur) => (a += `${cur[0]}=${cur[1]} \n`),
      ""
    );

    const common = commonArgs.reduce(
      (a, cur) => (a += `${cur[0]}=${cur[1]} \n`),
      ""
    );

    const targetArgs =
      target.args?.reduce((a, cur) => (a += `${cur[0]}=${cur[1]} \n`), "") ||
      "";

    const options =
      configuration.options?.reduce(
        (a, cur) => (a += `--${cur[0]}=${cur[1]} `),
        ""
      ) || "";

    const targetOptions =
      target.options?.reduce((a, cur) => (a += `--${cur[0]}=${cur[1]} `), "") ||
      "";

    // Windows runners ship `python.exe` (no `python3` alias). gn embeds the
    // value of --script-executable into the generated build.ninja `python_path`
    // variable, so when ninja later spawns `cmd.exe /c python_path ...` for
    // Skia's Python build helpers (e.g. gn/cp.py for icudtl.dat), an
    // executable that doesn't exist on PATH causes a hard failure mid-build.
    const scriptExecutable = platformName === "windows" ? "python" : "python3";
    const command = `${commandline} ${options} ${targetOptions} --script-executable=${scriptExecutable} --args='target_os="${target.platform}" target_cpu="${target.cpu}" ${common}${args}${targetArgs}'`;
    await runAsync(command, "⚙️");
    return true;
  } else {
    console.log(
      `Could not find platform "${platformName}" for target "${targetName}" `
    );
    return false;
  }
};

export const buildPlatform = async (
  platform: PlatformName,
  targetName: string
) => {
  process.chdir(SkiaSrc);
  console.log(`Building platform "${platform}" for target "${targetName}"`);
  // We need to include the path to our custom python2 -> python3 mapping script
  // to make sure we can run all scripts that uses #!/usr/bin/env python as shebang
  // https://groups.google.com/g/skia-discuss/c/BYyB-TwA8ow
  //
  // On Windows we restrict ninja to just the top-level user-facing libs. The
  // default `ninja all` rebuilds every BUILD.gn rule including orphan
  // third-parties (dng_sdk, etc.) whose Windows compatibility is broken in
  // current Skia even when their consumers are disabled via skia_use_*=false.
  //
  // We name the GN targets directly (no ".lib" suffix) and only at the
  // top-level — Skia's nested `skia_component` outputs under modules/* are
  // not registered as top-level ninja aliases (e.g. `skunicode_libgrapheme`),
  // but they are pulled in transitively as `public_deps` of `skparagraph`,
  // so the corresponding .lib files end up on disk for copyLib + the
  // .vcxproj link inputs even though ninja isn't asked for them by name.
  // macOS/Linux keep the existing default-target behavior.
  const windowsNinjaTargets = [
    "skia",
    "skshaper",
    "svg",
    "skottie",
    "sksg",
    "skparagraph",
  ];
  const ninjaTargets =
    platform === "windows" ? ` ${windowsNinjaTargets.join(" ")}` : "";
  const command = `PATH=${process.cwd()}/../bin:$PATH ninja -C ${getOutDir(
    platform,
    targetName
  )}${ninjaTargets}`;
  const platformEmoji: Record<string, string> = {
    android: "🤖",
    windows: "🪟",
  };
  const emoji = platformEmoji[platform] ?? "🍏";
  await runAsync(command, `${emoji} ${targetName}`);
};

export const copyLib = (
  os: PlatformName,
  cpu: string,
  platform: string,
  outputNames: string[]
) => {
  const dstPath = `${PackageRoot}/libs/${os}/${platform}/`;
  $(`mkdir -p ${dstPath}`);

  outputNames
    .map((name) => `${OutFolder}/${os}/${cpu}/${name}`)
    .forEach((lib) => {
      const libPath = lib;
      console.log(`Copying ${libPath} to ${dstPath}`);
      console.log(`cp ${libPath} ${dstPath}`);
      $(`cp ${libPath} ${dstPath}`);
    });
};

/**
 * Builds an XCFramework for a specific Apple platform.
 * Each platform produces its own XCFramework:
 * - apple-ios: arm64-iphoneos + lipo'd iphonesimulator (arm64 + x64)
 * - apple-tvos: arm64-tvos + lipo'd tvsimulator (arm64 + x64)
 * - apple-macos: lipo'd macosx (arm64 + x64)
 * - apple-maccatalyst: lipo'd maccatalyst (arm64 + x64)
 */
const buildXCFramework = (platformName: ApplePlatformName) => {
  const config = configurations[platformName];

  // Skip if no targets configured (e.g., tvos when GRAPHITE)
  if (Object.keys(config.targets).length === 0) {
    console.log(`⏭️  Skipping ${platformName} - no targets configured`);
    return;
  }

  const { outputNames } = config;
  if (outputNames.length === 0) {
    console.log(`⏭️  Skipping ${platformName} - no outputs configured`);
    return;
  }

  process.chdir(SkiaSrc);
  const prefix = `${OutFolder}/${platformName}`;

  // Get the short platform name (ios, tvos, macos)
  const shortPlatform = platformName.replace("apple-", "");

  // Create output directory
  const outputDir = `${PackageRoot}/libs/${shortPlatform}`;
  $(`mkdir -p ${outputDir}`);

  outputNames.forEach((name) => {
    console.log(`🍏 Building XCFramework for ${name} (${platformName})`);

    let xcframeworkCmd = "xcodebuild -create-xcframework ";

    if (shortPlatform === "ios") {
      // iOS: device + lipo'd simulator (arm64 + x64)
      $(`mkdir -p ${prefix}/iphonesimulator`);
      $(`rm -rf ${prefix}/iphonesimulator/${name}`);
      $(
        `lipo -create ${prefix}/x64-iphonesimulator/${name} ${prefix}/arm64-iphonesimulator/${name} -output ${prefix}/iphonesimulator/${name}`
      );
      xcframeworkCmd += `-library ${prefix}/arm64-iphoneos/${name} `;
      xcframeworkCmd += `-library ${prefix}/iphonesimulator/${name} `;
    } else if (shortPlatform === "tvos") {
      // tvOS: device + lipo'd simulator (arm64 + x64)
      $(`mkdir -p ${prefix}/tvsimulator`);
      $(`rm -rf ${prefix}/tvsimulator/${name}`);
      $(
        `lipo -create ${prefix}/x64-tvsimulator/${name} ${prefix}/arm64-tvsimulator/${name} -output ${prefix}/tvsimulator/${name}`
      );
      xcframeworkCmd += `-library ${prefix}/arm64-tvos/${name} `;
      xcframeworkCmd += `-library ${prefix}/tvsimulator/${name} `;
    } else if (shortPlatform === "macos") {
      // macOS: lipo arm64 + x64
      $(`mkdir -p ${prefix}/macosx`);
      $(`rm -rf ${prefix}/macosx/${name}`);
      $(
        `lipo -create ${prefix}/x64-macosx/${name} ${prefix}/arm64-macosx/${name} -output ${prefix}/macosx/${name}`
      );
      xcframeworkCmd += `-library ${prefix}/macosx/${name} `;
    } else if (shortPlatform === "maccatalyst") {
      // Mac Catalyst: lipo arm64 + x64
      $(`mkdir -p ${prefix}/maccatalyst`);
      $(`rm -rf ${prefix}/maccatalyst/${name}`);
      $(
        `lipo -create ${prefix}/x64-maccatalyst/${name} ${prefix}/arm64-maccatalyst/${name} -output ${prefix}/maccatalyst/${name}`
      );
      xcframeworkCmd += `-library ${prefix}/maccatalyst/${name} `;
    }

    const [lib] = name.split(".");
    const dstPath = `${outputDir}/${lib}.xcframework`;
    xcframeworkCmd += `-output ${dstPath}`;

    $(xcframeworkCmd);
  });

  console.log(`✅ XCFramework built for ${platformName}`);
};

(async () => {
  // Parse command line arguments
  const args = process.argv.slice(2);
  const defaultTargets = Object.keys(configurations);
  const targetSpecs = args.length > 0 ? args : defaultTargets;

  // Parse target specifications (platform or platform-target format)
  const buildTargets: { platform: PlatformName; targets?: string[] }[] = [];
  const validPlatforms = Object.keys(configurations);

  for (const spec of targetSpecs) {
    // First check if the entire spec is a valid platform name (e.g., "android", "apple-ios")
    if (validPlatforms.includes(spec)) {
      buildTargets.push({ platform: spec as PlatformName });
    } else if (spec.includes("-")) {
      // Handle platform-target format (e.g., android-arm, android-arm64)
      const [platform, target] = spec.split("-", 2);
      if (!validPlatforms.includes(platform)) {
        console.error(`❌ Invalid platform or target: ${spec}`);
        console.error(`Valid platforms are: ${validPlatforms.join(", ")}`);
        exit(1);
      }

      const platformConfig = configurations[platform as PlatformName];
      if (!(target in platformConfig.targets)) {
        console.error(
          `❌ Invalid target '${target}' for platform '${platform}'`
        );
        console.error(
          `Valid targets for ${platform}: ${Object.keys(
            platformConfig.targets
          ).join(", ")}`
        );
        exit(1);
      }

      // Find existing platform entry or create new one
      let existingPlatform = buildTargets.find(
        (bt) => bt.platform === platform
      );
      if (!existingPlatform) {
        existingPlatform = { platform: platform as PlatformName, targets: [] };
        buildTargets.push(existingPlatform);
      }
      if (!existingPlatform.targets) {
        existingPlatform.targets = [];
      }
      existingPlatform.targets.push(target);
    } else {
      // Invalid spec
      console.error(`❌ Invalid platform: ${spec}`);
      console.error(`Valid platforms are: ${validPlatforms.join(", ")}`);
      exit(1);
    }
  }

  console.log(`🎯 Building targets: ${targetSpecs.join(", ")}`);

  if (GRAPHITE) {
    console.log("🪨 Skia Graphite");
    console.log(
      "⚠️  Apple TV (tvOS) and MacCatalyst builds are skipped when GRAPHITE is enabled"
    );
  } else {
    console.log("🐘 Skia Ganesh");
  }

  if (MACCATALYST) {
    console.log("✅ macCatalyst builds are enabled");
  } else {
    console.log(
      "⚠️  macCatalyst builds are disabled (set SK_MACCATALYST=1 to enable)"
    );
  }

  // Check Android environment variables if android is in target platforms
  const hasAndroid = buildTargets.some((bt) => bt.platform === "android");
  if (hasAndroid) {
    ["ANDROID_NDK", "ANDROID_HOME"].forEach((name) => {
      // Test for existence of Android SDK
      if (!process.env[name]) {
        console.log(`${name} not set.`);
        exit(1);
      } else {
        console.log(`✅ ${name}`);
      }
    });
  }

  // Run glient sync
  console.log("Running gclient sync...");
  // Start by running sync
  process.chdir(SkiaSrc);
  $("PATH=../depot_tools/:$PATH python3 tools/git-sync-deps");
  console.log("gclient sync done");

  // Skia's bundled dng_sdk pthread polyfill
  // (third_party/externals/dng_sdk/source/dng_pthread.cpp) uses std::auto_ptr,
  // which was removed in C++17 and therefore fails to compile under MSVC /
  // clang-cl. Setting `enabled = !is_win` on the third_party() target makes
  // the template emit an empty static_library on Windows; the (empty) target
  // still exists for any GN dep that points at it, but no Windows-specific
  // sources get compiled. macOS/Linux/Android continue to build dng_sdk
  // normally. Raw image decoding (which would consume dng_sdk) is out of
  // scope for the Windows port; revisit if/when dng_sdk publishes a fix.
  const hasWindows = buildTargets.some(
    (bt) => bt.platform === "windows"
  );
  if (hasWindows) {
    // Skip the dng_sdk static_library on Windows — its bundled
    // dng_pthread.cpp uses std::auto_ptr (removed in C++17) and won't
    // compile under MSVC/clang-cl.
    const dngSdkBuildGn = `${SkiaSrc}/third_party/dng_sdk/BUILD.gn`;
    const dngContent = fs.readFileSync(dngSdkBuildGn, "utf-8");
    if (
      !dngContent.includes("enabled = !is_win") &&
      dngContent.includes('third_party("dng_sdk") {')
    ) {
      const patched = dngContent.replace(
        'third_party("dng_sdk") {',
        'third_party("dng_sdk") {\n  enabled = !is_win'
      );
      fs.writeFileSync(dngSdkBuildGn, patched);
      console.log("Patched third_party/dng_sdk/BUILD.gn (enabled = !is_win)");
    }

    // Even with skia_use_dng_sdk=false the optional("raw") consumer in
    // Skia's main BUILD.gn somehow keeps SkRawCodec.cpp in the compile
    // graph (the empty-stub branch of the optional() template should drop
    // sources, but doesn't here). Force-disable raw on Windows by ANDing
    // !is_win into its `enabled = ...` gate.
    const skiaBuildGn = `${SkiaSrc}/BUILD.gn`;
    const skiaContent = fs.readFileSync(skiaBuildGn, "utf-8");
    const rawEnabledLine =
      "enabled = skia_use_dng_sdk && skia_use_libjpeg_turbo_decode && skia_use_piex";
    const rawEnabledLineWin =
      "enabled = !is_win && skia_use_dng_sdk && skia_use_libjpeg_turbo_decode && skia_use_piex";
    if (
      skiaContent.includes(rawEnabledLine) &&
      !skiaContent.includes(rawEnabledLineWin)
    ) {
      fs.writeFileSync(
        skiaBuildGn,
        skiaContent.replace(rawEnabledLine, rawEnabledLineWin)
      );
      console.log("Patched BUILD.gn optional(\"raw\") to disable on Windows");
    }
  }

  if (GRAPHITE) {
    console.log("Applying Graphite patches...");
    $(`git reset --hard HEAD`);

    // Apply arm64e simulator patch
    const arm64ePatchFile = path.join(__dirname, "dawn-arm64e-simulator.patch");
    $(`cd ${SkiaSrc} && git apply ${arm64ePatchFile}`);

    // Remove arm64e arch flags (not available on simulator)
    {
      const filePath = `${SkiaSrc}/gn/skia/BUILD.gn`;
      const search = [
        `      ]`,
        `      if (!ios_use_simulator) {`,
        `        _arch_flags += [`,
        `          "-arch",`,
        `          "arm64e",`,
        `        ]`,
        `      }`,
        `    } else if (current_cpu == "x86") {`,
      ].join("\n");
      const replace = [`      ]`, `    } else if (current_cpu == "x86") {`].join("\n");
      const content = fs.readFileSync(filePath, "utf-8");
      if (!content.includes(search)) {
        throw new Error(`Patch target not found in ${filePath}`);
      }
      fs.writeFileSync(filePath, content.replace(search, replace));
    }
    // C++20 is required for Dawn (uses concepts/requires)
    {
      const filePath = `${SkiaSrc}/gn/skia/BUILD.gn`;
      const content = fs.readFileSync(filePath, "utf-8");
      fs.writeFileSync(
        filePath,
        content.replace(
          `cflags_cc += [ "-std=c++17" ]`,
          `cflags_cc += [ "-std=c++20" ]`
        )
      );
    }
    // Fix Dawn ShaderModuleMTL.mm uint32 typo (should be uint32_t)
    {
      const filePath = `${SkiaSrc}/third_party/externals/dawn/src/dawn/native/metal/ShaderModuleMTL.mm`;
      const content = fs.readFileSync(filePath, "utf-8");
      fs.writeFileSync(
        filePath,
        content.replace(/uint32\(bindingInfo\.binding\)/g, "uint32_t(bindingInfo.binding)")
      );
    }
    // Add iOS support to Dawn cmake_utils.py
    {
      const filePath = `${SkiaSrc}/third_party/dawn/cmake_utils.py`;
      let content = fs.readFileSync(filePath, "utf-8");
      // Add --ios_use_simulator argument
      content = content.replace(
        `parser.add_argument(
      "--enable_rtti", action=argparse.BooleanOptionalAction, help="Enable RTTI.")`,
        `parser.add_argument(
      "--enable_rtti", action=argparse.BooleanOptionalAction, help="Enable RTTI.")
  parser.add_argument(
      "--ios_use_simulator", action=argparse.BooleanOptionalAction, help="Build for iOS simulator.")`
      );
      // Add iOS OS/CPU mapping
      content = content.replace(
        `if os == "mac":
    target_cpu_map = {
      "arm64": "arm64",
      "x64": "x86_64",
    }
    return "Darwin", target_cpu_map[cpu]

  if os == "win":`,
        `if os == "mac":
    target_cpu_map = {
      "arm64": "arm64",
      "x64": "x86_64",
    }
    return "Darwin", target_cpu_map[cpu]

  if os == "ios":
    target_cpu_map = {
      "arm64": "arm64",
      "x64": "x86_64",
    }
    return "iOS", target_cpu_map[cpu]

  if os == "win":`
      );
      fs.writeFileSync(filePath, content);
    }
    // Add iOS support to Dawn build_dawn.py
    {
      const filePath = `${SkiaSrc}/third_party/dawn/build_dawn.py`;
      let content = fs.readFileSync(filePath, "utf-8");
      content = content.replace(
        `if target_os == "Darwin" or target_os == "iOS":
    configure_cmd.append(f"-DCMAKE_OSX_ARCHITECTURES={target_cpu}")

  env = os.environ.copy()`,
        `if target_os == "Darwin" or target_os == "iOS":
    configure_cmd.append(f"-DCMAKE_OSX_ARCHITECTURES={target_cpu}")

  if target_os == "iOS":
    configure_cmd.append("-DTINT_BUILD_CMD_TOOLS=OFF")
    if args.ios_use_simulator:
      configure_cmd.append("-DCMAKE_OSX_SYSROOT=iphonesimulator")

  env = os.environ.copy()`
      );
      fs.writeFileSync(filePath, content);
    }
    // Fix Dawn BUILD.gn for iOS (Cocoa.framework is macOS only)
    {
      const filePath = `${SkiaSrc}/third_party/dawn/BUILD.gn`;
      let content = fs.readFileSync(filePath, "utf-8");
      content = content.replace(
        `"Metal.framework",
      "QuartzCore.framework",
      "Cocoa.framework",
    ]
  }
}`,
        `"Metal.framework",
      "QuartzCore.framework",
    ]
    if (is_mac) {
      frameworks += [ "Cocoa.framework" ]
    }
  }
}`
      );
      // Add ios_use_simulator argument passing
      content = content.replace(
        `if (is_android) {
    args += [
      "--android_ndk_path=" + ndk,
      "--android_platform=android-" + ndk_api,
    ]
  }

  if (dawn_enable_d3d11) {`,
        `if (is_android) {
    args += [
      "--android_ndk_path=" + ndk,
      "--android_platform=android-" + ndk_api,
    ]
  }
  if (is_ios && ios_use_simulator) {
    args += [ "--ios_use_simulator" ]
  }

  if (dawn_enable_d3d11) {`
      );
      fs.writeFileSync(filePath, content);
    }
    console.log("Patches applied successfully");
  }
  $(`rm -rf ${PackageRoot}/libs`);

  if (GRAPHITE) {
    $(`mkdir -p ${PackageRoot}/libs`);
    fs.writeFileSync(`${PackageRoot}/libs/.graphite`, "");
  }

  // Build specified platforms and targets
  for (const buildTarget of buildTargets) {
    const { platform, targets } = buildTarget;
    const configuration = configurations[platform];
    console.log(`\n🔨 Building platform: ${platform}`);

    const targetsToProcess =
      targets || (mapKeys(configuration.targets) as string[]);

    for (const target of targetsToProcess) {
      await configurePlatform(platform, configuration, target);
      await buildPlatform(platform, target);
      process.chdir(ProjectRoot);
      if (platform === "android" || isWindowsPlatform(platform)) {
        copyLib(
          platform,
          target,
          // eslint-disable-next-line @typescript-eslint/ban-ts-comment
          // @ts-ignore
          configuration.targets[target].output,
          configuration.outputNames
        );
      }
    }
  }

  // Build XCFrameworks for each Apple platform that was built
  const applePlatforms = buildTargets
    .filter((bt) => isApplePlatform(bt.platform))
    .map((bt) => bt.platform as ApplePlatformName);

  for (const applePlatform of applePlatforms) {
    buildXCFramework(applePlatform);
  }

  copyHeaders();
})();
