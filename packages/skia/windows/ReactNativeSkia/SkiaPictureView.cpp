#include "pch.h"

#include "SkiaPictureView.h"

#include <charconv>

#include "RNSkPictureView.h"
#include "SkiaManager.h"

namespace RNSkia {

namespace {

size_t ParseNativeId(std::string const &nativeId) {
  size_t value = 0;
  std::from_chars(nativeId.data(), nativeId.data() + nativeId.size(), value);
  return value;
}

} // namespace

SkiaPictureView::SkiaPictureView()
    : _panel(winrt::Windows::UI::Xaml::Controls::SwapChainPanel{}) {
  auto manager = SkiaManager::getInstance();
  if (manager == nullptr) {
    return;
  }
  auto platformContext = manager->getPlatformContext();
  std::weak_ptr<SkiaPictureView> weak;
  _canvasProvider = std::make_shared<RNSkD3D12CanvasProvider>(
      [self = this]() { self->RequestRedraw(); }, platformContext);
  _renderer = std::make_shared<RNSkPictureRenderer>(
      [self = this]() { self->RequestRedraw(); }, platformContext);

  _panel.SizeChanged(
      [this](auto const &, winrt::Windows::UI::Xaml::SizeChangedEventArgs const &args) {
        OnSizeChanged(args.NewSize().Width, args.NewSize().Height);
      });
  _panel.Unloaded([this](auto const &, auto const &) { OnUnloaded(); });
}

SkiaPictureView::~SkiaPictureView() { Unregister(); }

void SkiaPictureView::RequestRedraw() {
  if (_renderer && _canvasProvider) {
    _renderer->renderImmediate(_canvasProvider);
  }
}

void SkiaPictureView::SetNativeId(std::string const &nativeId) {
  std::lock_guard<std::mutex> lock(_mutex);
  Unregister();
  _nativeId = nativeId;
  _nativeIdNumeric = ParseNativeId(nativeId);
  Register();
}

void SkiaPictureView::SetDebug(bool /*debug*/) {
  // Debug overlay (FPS, RAF stats) is rendered on top of the Skia canvas;
  // wired in Phase 2.
}

void SkiaPictureView::SetOpaque(bool /*opaque*/) {
  // Toggles DXGI_ALPHA_MODE_PREMULTIPLIED vs DXGI_ALPHA_MODE_IGNORE on the
  // swap chain. Wired in Phase 2.
}

void SkiaPictureView::OnSizeChanged(double width, double height) {
  if (!_canvasProvider) {
    return;
  }
  _canvasProvider->setSize(static_cast<int>(width), static_cast<int>(height));
  // The panel itself is the IInspectable that ISwapChainPanelNative consumes.
  _canvasProvider->setSwapChainPanel(winrt::get_abi(_panel));
}

void SkiaPictureView::OnUnloaded() {
  if (_canvasProvider) {
    _canvasProvider->surfaceLost();
  }
  Unregister();
}

void SkiaPictureView::Register() {
  if (_registered || _nativeIdNumeric == 0) {
    return;
  }
  auto manager = SkiaManager::getInstance();
  if (manager == nullptr) {
    return;
  }
  // The cross-platform RNSkPictureView constructor takes the renderer +
  // canvas provider; we mirror the iOS/Android wiring here.
  auto skView = std::make_shared<RNSkPictureView>(manager->getPlatformContext(),
                                                  _renderer, _canvasProvider);
  manager->getManager()->registerSkiaView(_nativeIdNumeric, skView);
  _registered = true;
}

void SkiaPictureView::Unregister() {
  if (!_registered) {
    return;
  }
  if (auto manager = SkiaManager::getInstance()) {
    manager->getManager()->unregisterSkiaView(_nativeIdNumeric);
  }
  _registered = false;
}

} // namespace RNSkia
