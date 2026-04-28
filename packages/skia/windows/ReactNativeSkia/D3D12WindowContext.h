#pragma once

#include <memory>
#include <mutex>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"

#pragma clang diagnostic pop

#include "RNWindowContext.h"

namespace RNSkia {

/**
 * Process-wide D3D12 device + GrDirectContext, mirroring the role
 * OpenGLContext plays on Android. Window-bound resources (swap chains,
 * render-target textures) are managed by D3D12WindowSurface.
 *
 * Threading: device creation is one-shot and threadsafe. Submit + present
 * happen on the UI thread (the Skia render thread). The same singleton may
 * be queried for an offscreen surface from background threads provided
 * Skia's standard GrDirectContext threading rules are honoured.
 */
class D3D12WindowSurface;

class D3D12WindowContext {
public:
  static D3D12WindowContext &getInstance();

  D3D12WindowContext(const D3D12WindowContext &) = delete;
  D3D12WindowContext &operator=(const D3D12WindowContext &) = delete;

  GrDirectContext *getDirectContext();

  sk_sp<SkSurface> MakeOffscreen(int width, int height,
                                 bool useP3ColorSpace = false);

  std::shared_ptr<WindowContext> MakeWindow(void *swapChainPanel, int width,
                                            int height);

private:
  D3D12WindowContext();

  void ensureDevice();

  std::once_flag _initFlag;

  Microsoft::WRL::ComPtr<IDXGIFactory4> _factory;
  Microsoft::WRL::ComPtr<IDXGIAdapter1> _adapter;
  Microsoft::WRL::ComPtr<ID3D12Device> _device;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue;

  sk_sp<GrDirectContext> _grContext;
};

/**
 * Per-view rendering surface bound to a XAML SwapChainPanel (or any other
 * IDXGISwapChain consumer). Holds its swap chain, frame fences and the
 * SkSurfaces wrapping the back buffers.
 */
class D3D12WindowSurface : public WindowContext {
public:
  D3D12WindowSurface(D3D12WindowContext &context, void *swapChainPanel,
                     int width, int height);
  ~D3D12WindowSurface() override;

  sk_sp<SkSurface> getSurface() override;
  void present() override;
  void resize(int width, int height) override;
  int getWidth() override { return _width; }
  int getHeight() override { return _height; }

private:
  static constexpr int kFrameCount = 2;

  void createSwapChain(void *swapChainPanel);
  void releaseFrameResources();
  void waitForGPU();

  D3D12WindowContext &_context;
  int _width;
  int _height;

  Microsoft::WRL::ComPtr<IDXGISwapChain3> _swapChain;
  Microsoft::WRL::ComPtr<ID3D12Resource> _backBuffers[kFrameCount];
  sk_sp<SkSurface> _skSurfaces[kFrameCount];

  Microsoft::WRL::ComPtr<ID3D12Fence> _fence;
  uint64_t _fenceValues[kFrameCount] = {0, 0};
  HANDLE _fenceEvent = nullptr;
  uint32_t _frameIndex = 0;
};

} // namespace RNSkia
