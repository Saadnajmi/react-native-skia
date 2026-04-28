#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Windows.UI.Xaml.Controls.h>

#include "RNSkD3D12CanvasProvider.h"
#include "RNSkPictureView.h"

namespace RNSkia {

/**
 * Windows hosting class for a single SkiaPictureView.
 *
 * Owns:
 *   - a XAML SwapChainPanel that's added to the RN composition tree as the
 *     view's content
 *   - the RNSkPictureRenderer (cross-platform) that records / replays
 *     SkPicture commands
 *   - the RNSkD3D12CanvasProvider (Windows-specific) that manages the swap
 *     chain bound to the panel
 *
 * The view is registered with the RNSkManager by its `nativeID` prop, mirroring
 * the iOS / Android view managers; that's the handle the JSI Skia API uses to
 * push picture updates and request redraws.
 */
class SkiaPictureView : public std::enable_shared_from_this<SkiaPictureView> {
public:
  SkiaPictureView();
  ~SkiaPictureView();

  winrt::Windows::UI::Xaml::Controls::SwapChainPanel const &Panel() const {
    return _panel;
  }

  void SetNativeId(std::string const &nativeId);
  void SetDebug(bool debug);
  void SetOpaque(bool opaque);
  void OnSizeChanged(double width, double height);
  void OnUnloaded();

private:
  void RequestRedraw();
  void Register();
  void Unregister();

  winrt::Windows::UI::Xaml::Controls::SwapChainPanel _panel{nullptr};
  std::shared_ptr<RNSkPictureRenderer> _renderer;
  std::shared_ptr<RNSkD3D12CanvasProvider> _canvasProvider;
  std::shared_ptr<RNSkPictureView> _selfStrong;
  std::string _nativeId;
  size_t _nativeIdNumeric = 0;
  bool _registered = false;
  std::mutex _mutex;
};

} // namespace RNSkia
