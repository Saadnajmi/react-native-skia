#include "pch.h"

#include "D3D12WindowContext.h"

#include <stdexcept>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include "include/core/SkColorSpace.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/d3d/GrD3DBackendContext.h"
#include "include/gpu/ganesh/d3d/GrD3DTypes.h"

#pragma clang diagnostic pop

#include <winrt/Windows.UI.Xaml.Controls.h>

using Microsoft::WRL::ComPtr;

namespace RNSkia {

namespace {

constexpr DXGI_FORMAT kBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

void ThrowIfFailed(HRESULT hr, const char *what) {
  if (FAILED(hr)) {
    char message[256];
    snprintf(message, sizeof(message), "%s failed: 0x%08lX", what,
             static_cast<unsigned long>(hr));
    throw std::runtime_error(message);
  }
}

} // namespace

D3D12WindowContext &D3D12WindowContext::getInstance() {
  static D3D12WindowContext instance;
  return instance;
}

D3D12WindowContext::D3D12WindowContext() = default;

void D3D12WindowContext::ensureDevice() {
  std::call_once(_initFlag, [this]() {
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&_factory)),
                  "CreateDXGIFactory2");

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0;
         _factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        continue;
      }
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                      IID_PPV_ARGS(&_device)))) {
        _adapter = adapter;
        break;
      }
    }

    if (_device == nullptr) {
      throw std::runtime_error(
          "RNSkia: no D3D12-capable adapter found on this system.");
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(_device->CreateCommandQueue(&queueDesc,
                                              IID_PPV_ARGS(&_commandQueue)),
                  "CreateCommandQueue");

    GrD3DBackendContext backendContext;
    backendContext.fAdapter = _adapter;
    backendContext.fDevice = _device;
    backendContext.fQueue = _commandQueue;
    backendContext.fProtectedContext = GrProtected::kNo;

    _grContext = GrDirectContext::MakeDirect3D(backendContext);
    if (!_grContext) {
      throw std::runtime_error(
          "RNSkia: GrDirectContext::MakeDirect3D returned null.");
    }
  });
}

GrDirectContext *D3D12WindowContext::getDirectContext() {
  ensureDevice();
  return _grContext.get();
}

sk_sp<SkSurface> D3D12WindowContext::MakeOffscreen(int width, int height,
                                                   bool useP3ColorSpace) {
  ensureDevice();
  auto colorSpace = useP3ColorSpace
                        ? SkColorSpace::MakeRGB(SkNamedTransferFn::kSRGB,
                                                SkNamedGamut::kDisplayP3)
                        : SkColorSpace::MakeSRGB();
  SkImageInfo info =
      SkImageInfo::Make(width, height, kRGBA_8888_SkColorType,
                        kPremul_SkAlphaType, std::move(colorSpace));
  return SkSurfaces::RenderTarget(_grContext.get(), skgpu::Budgeted::kNo, info);
}

std::shared_ptr<WindowContext>
D3D12WindowContext::MakeWindow(void *swapChainPanel, int width, int height) {
  ensureDevice();
  return std::make_shared<D3D12WindowSurface>(*this, swapChainPanel, width,
                                              height);
}

D3D12WindowSurface::D3D12WindowSurface(D3D12WindowContext &context,
                                       void *swapChainPanel, int width,
                                       int height)
    : _context(context), _width(width), _height(height) {
  _fenceEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (_fenceEvent == nullptr) {
    throw std::runtime_error("CreateEvent for D3D12 fence failed.");
  }
  createSwapChain(swapChainPanel);
}

D3D12WindowSurface::~D3D12WindowSurface() {
  waitForGPU();
  if (_fenceEvent != nullptr) {
    ::CloseHandle(_fenceEvent);
    _fenceEvent = nullptr;
  }
}

void D3D12WindowSurface::createSwapChain(void *swapChainPanel) {
  auto *device = _context._device.Get();
  auto *queue = _context._commandQueue.Get();
  auto *factory = _context._factory.Get();

  DXGI_SWAP_CHAIN_DESC1 desc{};
  desc.Width = _width;
  desc.Height = _height;
  desc.Format = kBackBufferFormat;
  desc.Stereo = FALSE;
  desc.SampleDesc.Count = 1;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = kFrameCount;
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

  ComPtr<IDXGISwapChain1> swapChain1;
  ThrowIfFailed(factory->CreateSwapChainForComposition(queue, &desc, nullptr,
                                                       &swapChain1),
                "CreateSwapChainForComposition");
  ThrowIfFailed(swapChain1.As(&_swapChain), "QI IDXGISwapChain3");

  // Hand the swap chain to the SwapChainPanel via its native interop.
  if (swapChainPanel != nullptr) {
    auto panel = winrt::Windows::UI::Xaml::Controls::SwapChainPanel{
        winrt::Windows::Foundation::IInspectable{
            reinterpret_cast<winrt::Windows::Foundation::IUnknown *>(
                swapChainPanel)
                ->as<winrt::Windows::Foundation::IInspectable>()}};
    auto native =
        panel.as<ISwapChainPanelNative>(); // requires <windows.ui.xaml.media.dxinterop.h>
    ThrowIfFailed(native->SetSwapChain(_swapChain.Get()),
                  "ISwapChainPanelNative::SetSwapChain");
  }

  ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                    IID_PPV_ARGS(&_fence)),
                "CreateFence");

  for (int i = 0; i < kFrameCount; ++i) {
    ThrowIfFailed(_swapChain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i])),
                  "SwapChain::GetBuffer");

    GrD3DTextureResourceInfo info{};
    info.fResource = _backBuffers[i];
    info.fResourceState = D3D12_RESOURCE_STATE_PRESENT;
    info.fFormat = kBackBufferFormat;
    info.fSampleCount = 1;
    info.fLevelCount = 1;
    info.fProtected = GrProtected::kNo;

    GrBackendRenderTarget backendRT(_width, _height, info);
    _skSurfaces[i] = SkSurfaces::WrapBackendRenderTarget(
        _context._grContext.get(), backendRT, kTopLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
  }

  _frameIndex = _swapChain->GetCurrentBackBufferIndex();
}

void D3D12WindowSurface::releaseFrameResources() {
  for (int i = 0; i < kFrameCount; ++i) {
    _skSurfaces[i].reset();
    _backBuffers[i].Reset();
  }
}

void D3D12WindowSurface::waitForGPU() {
  if (!_fence || !_context._commandQueue) {
    return;
  }
  const uint64_t value = _fenceValues[_frameIndex] + 1;
  if (FAILED(_context._commandQueue->Signal(_fence.Get(), value))) {
    return;
  }
  _fenceValues[_frameIndex] = value;
  if (_fence->GetCompletedValue() < value) {
    if (SUCCEEDED(_fence->SetEventOnCompletion(value, _fenceEvent))) {
      ::WaitForSingleObject(_fenceEvent, INFINITE);
    }
  }
}

sk_sp<SkSurface> D3D12WindowSurface::getSurface() {
  _frameIndex = _swapChain->GetCurrentBackBufferIndex();
  return _skSurfaces[_frameIndex];
}

void D3D12WindowSurface::present() {
  auto *grContext = _context._grContext.get();
  if (auto surface = _skSurfaces[_frameIndex]) {
    grContext->flush(surface.get(), {});
    grContext->submit(GrSyncCpu::kNo);
  }
  _swapChain->Present(1, 0);

  const uint64_t signalValue = _fenceValues[_frameIndex] + 1;
  _context._commandQueue->Signal(_fence.Get(), signalValue);
  _fenceValues[_frameIndex] = signalValue;

  _frameIndex = _swapChain->GetCurrentBackBufferIndex();
  if (_fence->GetCompletedValue() < _fenceValues[_frameIndex]) {
    _fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent);
    ::WaitForSingleObject(_fenceEvent, INFINITE);
  }
}

void D3D12WindowSurface::resize(int width, int height) {
  if (width == _width && height == _height) {
    return;
  }
  waitForGPU();
  releaseFrameResources();

  _width = width;
  _height = height;

  ThrowIfFailed(_swapChain->ResizeBuffers(kFrameCount, width, height,
                                          kBackBufferFormat, 0),
                "SwapChain::ResizeBuffers");

  for (int i = 0; i < kFrameCount; ++i) {
    ThrowIfFailed(_swapChain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i])),
                  "SwapChain::GetBuffer (resize)");

    GrD3DTextureResourceInfo info{};
    info.fResource = _backBuffers[i];
    info.fResourceState = D3D12_RESOURCE_STATE_PRESENT;
    info.fFormat = kBackBufferFormat;
    info.fSampleCount = 1;
    info.fLevelCount = 1;
    info.fProtected = GrProtected::kNo;

    GrBackendRenderTarget backendRT(_width, _height, info);
    _skSurfaces[i] = SkSurfaces::WrapBackendRenderTarget(
        _context._grContext.get(), backendRT, kTopLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
  }
  _frameIndex = _swapChain->GetCurrentBackBufferIndex();
}

} // namespace RNSkia
