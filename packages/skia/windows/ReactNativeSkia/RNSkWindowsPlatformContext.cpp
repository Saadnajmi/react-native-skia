#include "pch.h"

#include "RNSkWindowsPlatformContext.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include "include/core/SkColorSpace.h"
#include "include/core/SkData.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/ports/SkFontMgr_directwrite.h"

#pragma clang diagnostic pop

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Web.Http.h>

#include "D3D12WindowContext.h"
#include "RNDawnContext.h"

namespace RNSkia {

RNSkWindowsPlatformContext::RNSkWindowsPlatformContext(
    winrt::Microsoft::ReactNative::ReactContext const &reactContext,
    std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker,
    float pixelDensity)
    : RNSkPlatformContext(jsCallInvoker, pixelDensity),
      _reactContext(reactContext) {
  try {
    _uiDispatcher = winrt::Windows::ApplicationModel::Core::CoreApplication::
                        MainView()
                            .CoreWindow()
                            .Dispatcher();
  } catch (...) {
    // No UI dispatcher available (e.g. test/headless host). runOnMainThread
    // will fall back to invoking inline.
    _uiDispatcher = nullptr;
  }
}

void RNSkWindowsPlatformContext::runOnMainThread(std::function<void()> func) {
  if (_uiDispatcher == nullptr) {
    func();
    return;
  }
  _uiDispatcher.RunAsync(
      winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
      [func = std::move(func)]() { func(); });
}

sk_sp<SkImage> RNSkWindowsPlatformContext::takeScreenshotFromViewTag(
    size_t /*tag*/) {
  // TODO(windows): wire ViewScreenshotService once Fabric view registry
  // exposes a stable lookup path. Returning nullptr matches the documented
  // contract for "screenshot unavailable".
  return nullptr;
}

void RNSkWindowsPlatformContext::performStreamOperation(
    const std::string &sourceUri,
    const std::function<void(std::unique_ptr<SkStreamAsset>)> &op) {
  // Phase 1: file:// and absolute paths only. Network and ms-appx:// resources
  // arrive in a follow-up.
  std::string path = sourceUri;
  constexpr std::string_view fileScheme = "file://";
  if (path.compare(0, fileScheme.size(), fileScheme) == 0) {
    path = path.substr(fileScheme.size());
  }

  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream.is_open()) {
    raiseError(std::runtime_error("Could not open stream from " + sourceUri));
    return;
  }
  const auto size = static_cast<size_t>(stream.tellg());
  stream.seekg(0, std::ios::beg);
  auto data = SkData::MakeUninitialized(size);
  stream.read(reinterpret_cast<char *>(data->writable_data()),
              static_cast<std::streamsize>(size));
  op(SkMemoryStream::Make(std::move(data)));
}

void RNSkWindowsPlatformContext::raiseError(const std::exception &err) {
  // RNW surfaces unhandled exceptions through the redbox; we forward via
  // CallInvoker so the JS layer can also observe the message.
  const std::string message = err.what();
  runOnJavascriptThread([message]() { throw std::runtime_error(message); });
}

sk_sp<SkSurface> RNSkWindowsPlatformContext::makeOffscreenSurface(
    int width, int height, bool useP3ColorSpace) {
#if defined(SK_GRAPHITE)
  return DawnContext::getInstance().MakeOffscreen(width, height,
                                                  useP3ColorSpace);
#else
  return D3D12WindowContext::getInstance().MakeOffscreen(width, height,
                                                         useP3ColorSpace);
#endif
}

std::shared_ptr<WindowContext> RNSkWindowsPlatformContext::makeContextFromNativeSurface(
    void *surface, int width, int height) {
#if defined(SK_GRAPHITE)
  return DawnContext::getInstance().MakeWindow(surface, width, height);
#else
  return D3D12WindowContext::getInstance().MakeWindow(surface, width, height);
#endif
}

sk_sp<SkImage>
RNSkWindowsPlatformContext::makeImageFromNativeBuffer(void * /*buffer*/) {
  // Phase 2: wire DXGI shared handle (the Windows analog of AHardwareBuffer
  // and CVPixelBuffer) here.
  return nullptr;
}

#if !defined(SK_GRAPHITE)
sk_sp<SkImage> RNSkWindowsPlatformContext::makeImageFromNativeTexture(
    const TextureInfo & /*textureInfo*/, int /*width*/, int /*height*/,
    bool /*mipMapped*/) {
  // D3D12 texture interop: TextureInfo doesn't yet carry a D3D12 resource
  // pointer; that field is added in Phase 2 alongside this implementation.
  return nullptr;
}

const TextureInfo
RNSkWindowsPlatformContext::getTexture(sk_sp<SkSurface> /*surface*/) {
  return TextureInfo{};
}

const TextureInfo
RNSkWindowsPlatformContext::getTexture(sk_sp<SkImage> /*image*/) {
  return TextureInfo{};
}

GrDirectContext *RNSkWindowsPlatformContext::getDirectContext() {
  return D3D12WindowContext::getInstance().getDirectContext();
}
#endif

void RNSkWindowsPlatformContext::releaseNativeBuffer(uint64_t /*pointer*/) {
  // Paired with makeNativeBuffer; both arrive in Phase 2.
}

uint64_t RNSkWindowsPlatformContext::makeNativeBuffer(sk_sp<SkImage> /*image*/) {
  return 0;
}

std::shared_ptr<RNSkVideo>
RNSkWindowsPlatformContext::createVideo(const std::string & /*url*/) {
  // Phase 3: implement via Windows Media Foundation
  // (IMFSourceReader -> IMFSample -> SkImage).
  return nullptr;
}

sk_sp<SkFontMgr> RNSkWindowsPlatformContext::createFontMgr() {
  return SkFontMgr_New_DirectWrite();
}

std::vector<std::string> RNSkWindowsPlatformContext::getSystemFontFamilies() {
  return {};
}

std::string
RNSkWindowsPlatformContext::resolveFontFamily(const std::string &familyName) {
  if (familyName == "System" || familyName == "system-ui") {
    return "Segoe UI Variable";
  }
  return familyName;
}

} // namespace RNSkia
