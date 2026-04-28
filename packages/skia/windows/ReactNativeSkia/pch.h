#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <unknwn.h>
#include <windows.h>

#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>

#include <CppWinRTIncludes.h>
#include <NativeModules.h>
#include <ReactCommon/CallInvoker.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
