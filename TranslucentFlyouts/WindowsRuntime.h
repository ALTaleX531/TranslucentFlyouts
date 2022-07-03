#pragma once
// Windows Í·ÎÄ¼þ
#include <windows.h>
#include <Unknwn.h>
#include <roapi.h>
#include <DispatcherQueue.h>
// Windows Runtime Library
#include <wrl.h>
#include <wrl\client.h>
#include <wrl\module.h>
#include <wrl\implements.h>
#include <wrl\wrappers\corewrappers.h>
// Windows.UI.Composition
#include <windows.ui.composition.h>
#include <windows.ui.composition.core.h>
#include <windows.ui.composition.effects.h>
#include <windows.ui.composition.desktop.h>
#include <windows.ui.composition.interop.h>
#include <windows.ui.composition.interactions.h>
// Windows.UI.Xaml
#undef GetCurrentTime
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <windows.ui.xaml.hosting.h>
#include <windows.ui.xaml.controls.h>
#include <windows.ui.xaml.controls.maps.h>
#include <windows.ui.xaml.controls.primitives.h>
#include <windows.ui.xaml.media.h>

#pragma comment(lib, "runtimeobject.lib")

namespace
{
	using namespace ABI;
	using namespace ABI::Windows;
	using namespace Microsoft::WRL;
	using namespace Microsoft::WRL::Wrappers;
}