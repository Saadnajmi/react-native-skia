#include "pch.h"

#include "RNSkD3D12CanvasProvider.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include "include/core/SkSurface.h"

#pragma clang diagnostic pop

namespace RNSkia {

RNSkD3D12CanvasProvider::RNSkD3D12CanvasProvider(
    std::function<void()> requestRedraw,
    std::shared_ptr<RNSkPlatformContext> platformContext)
    : _requestRedraw(std::move(requestRedraw)),
      _platformContext(std::move(platformContext)) {}

RNSkD3D12CanvasProvider::~RNSkD3D12CanvasProvider() = default;

bool RNSkD3D12CanvasProvider::renderToCanvas(
    const std::function<void(SkCanvas *)> &cb) {
  std::lock_guard<std::mutex> lock(_mutex);

  const int width = _width.load();
  const int height = _height.load();
  if (width <= 0 || height <= 0 || _swapChainPanel == nullptr) {
    return false;
  }

  if (_windowContext == nullptr) {
    _windowContext = _platformContext->makeContextFromNativeSurface(
        _swapChainPanel, width, height);
    if (_windowContext == nullptr) {
      return false;
    }
  } else if (_windowContext->getWidth() != width ||
             _windowContext->getHeight() != height) {
    _windowContext->resize(width, height);
  }

  auto surface = _windowContext->getSurface();
  if (!surface) {
    return false;
  }
  auto *canvas = surface->getCanvas();
  cb(canvas);
  _windowContext->present();
  return true;
}

void RNSkD3D12CanvasProvider::setSize(int width, int height) {
  _width = width;
  _height = height;
  if (_requestRedraw) {
    _requestRedraw();
  }
}

void RNSkD3D12CanvasProvider::setSwapChainPanel(
    void *swapChainPanelInspectable) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_swapChainPanel == swapChainPanelInspectable) {
    return;
  }
  _swapChainPanel = swapChainPanelInspectable;
  _windowContext.reset();
  if (_requestRedraw) {
    _requestRedraw();
  }
}

void RNSkD3D12CanvasProvider::surfaceLost() {
  std::lock_guard<std::mutex> lock(_mutex);
  _windowContext.reset();
}

} // namespace RNSkia
