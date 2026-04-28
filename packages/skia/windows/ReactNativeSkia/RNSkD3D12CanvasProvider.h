#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include "include/core/SkCanvas.h"

#pragma clang diagnostic pop

#include "RNSkPlatformContext.h"
#include "RNSkView.h"
#include "RNWindowContext.h"

namespace RNSkia {

/**
 * Windows canvas provider that delegates surface acquisition to a
 * D3D12 (Ganesh) or Dawn (Graphite) WindowContext built lazily when the
 * SwapChainPanel reports a non-zero size.
 *
 * The instance is owned by a SkiaPictureView. Calls to renderToCanvas come
 * from the RNSkView render loop on the UI thread, which matches the thread
 * that drives D3D12 present.
 */
class RNSkD3D12CanvasProvider : public RNSkCanvasProvider {
public:
  RNSkD3D12CanvasProvider(std::function<void()> requestRedraw,
                          std::shared_ptr<RNSkPlatformContext> platformContext);
  ~RNSkD3D12CanvasProvider() override;

  int getWidth() override { return _width; }
  int getHeight() override { return _height; }
  bool renderToCanvas(const std::function<void(SkCanvas *)> &cb) override;

  void setSize(int width, int height);
  void setSwapChainPanel(void *swapChainPanelInspectable);
  void surfaceLost();

private:
  std::function<void()> _requestRedraw;
  std::shared_ptr<RNSkPlatformContext> _platformContext;

  std::mutex _mutex;
  std::shared_ptr<WindowContext> _windowContext;
  void *_swapChainPanel = nullptr;
  std::atomic<int> _width{0};
  std::atomic<int> _height{0};
};

} // namespace RNSkia
