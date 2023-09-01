#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	// Tweak the appearance of immersive context menu
	namespace ImmersiveContextMenuPatcher
	{
		namespace
		{
			using namespace std::literals;
			// A list of modules that contain the symbol of C++ class ImmersiveContextMenuHelper,
			// which it means these modules provide methods to create a immersive context menu
			constexpr std::array g_hookModuleList
			{
				L"explorer.exe"sv,
				L"Narrator.exe"sv,
				L"minecraft.exe"sv,				// Minecraft Launcher
				L"MusNotifyIcon.exe"sv,
				L"ApplicationFrame.dll"sv,
				L"ExplorerFrame.dll"sv,
				L"InputSwitch.dll"sv,
				L"pnidui.dll"sv,
				L"SecurityHealthSSO.dll"sv,
				L"shell32.dll"sv,
				L"SndVolSSO.dll"sv,
				L"twinui.dll"sv,
				L"twinui.pcshell.dll"sv,
				L"bthprops.cpl"sv,
				// Windows 11
				L"Taskmgr.exe"sv,
				L"museuxdocked.dll"sv,
				L"SecurityHealthSsoUdk.dll"sv,
				L"Taskbar.dll"sv,
				L"Windows.UI.FileExplorer.dll"sv,
				L"Windows.UI.FileExplorer.WASDK.dll"sv,
				L"stobject.dll"sv,
				// Third-party apps
				L"StartIsBack64.dll"sv,
				L"StartIsBack32.dll"sv
			};
		}

		HRESULT WINAPI DrawThemeBackground(
			HTHEME  hTheme,
			HDC     hdc,
			int     iPartId,
			int     iStateId,
			LPCRECT pRect,
			LPCRECT pClipRect
		);

		void Startup();
		void Shutdown();
	}
}