#pragma once
#include "pch.h"
#include "TFMain.hpp"

namespace TranslucentFlyouts
{
	namespace MenuHandler
	{
		constexpr UINT MN_SETHMENU{ 0x01E0 };
#ifndef MN_GETHMENU
		constexpr UINT MN_GETHMENU { 0x01E1 };
#endif // !MN_GETHMENU
		constexpr UINT MN_SIZEWINDOW { 0x01E2 };
		constexpr UINT MN_OPENHIERARCHY{ 0x01E3 };
		constexpr UINT MN_CLOSEHIERARCHY{ 0x01E4 };
		constexpr UINT MN_SELECTITEM{ 0x01E5 };
		constexpr UINT MN_CANCELMENUS{ 0x01E6 };
		constexpr UINT MN_SELECTFIRSTVALIDITEM{ 0x01E7 };
		constexpr UINT MN_GETPOPUPMENU{ 0x01EA };	// obsolete
		constexpr UINT MN_FINDMENUWINDOWFROMPOINT{ 0x01EB };
		constexpr UINT MN_SHOWPOPUPWINDOW{ 0x01EC };
		constexpr UINT MN_MOUSEMOVE{ 0x01EE };
		constexpr UINT MN_BUTTONDOWN{ 0x01ED };
		constexpr UINT MN_BUTTONUP{ 0x01EF };
		constexpr UINT MN_SETTIMERTOOPENHIERARCHY{ 0x01F0 };
		constexpr UINT MN_DBLCLK{ 0x01F1 };
		constexpr UINT MN_ACTIVEMENU{ 0x01F2 };
		constexpr UINT MN_DODRAGDROP{ 0x01F3 };
		constexpr UINT MN_ENDMENU{ 0x01F4 };

		constexpr DWORD lightMode_HotColor{ 0x30000000 };
		constexpr DWORD darkMode_HotColor{ 0x41808080 };

		constexpr DWORD lightMode_DisabledHotColor{ 0x00000000 };
		constexpr DWORD darkMode_DisabledHotColor{ 0x00000000 };

		constexpr DWORD lightMode_SeparatorColor{ 0x30262626 };
		constexpr DWORD darkMode_SeparatorColor{ 0x30D9D9D9 };

		constexpr DWORD lightMode_FocusingColor{ 0xFF000000 };
		constexpr DWORD darkMode_FocusingColor{ 0xFFFFFFFF };

		constexpr DWORD focusingWidth{ 1000 };
		constexpr DWORD cornerRadius{ 8 };
		constexpr DWORD separatorWidth{ 1000 };

		constexpr int systemOutlineSize{ 1 };
		constexpr int nonClientMarginStandardSize{ 3 };

		struct MenuRenderingInfo
		{
			bool useUxTheme{ false };
			bool useDarkMode{ false };
			bool immersive{ false };
			COLORREF borderColor{ DWMWA_COLOR_NONE };

			inline void Reset()
			{
				useUxTheme = false;
				useDarkMode = false;
				immersive = false;
				borderColor = DWMWA_COLOR_NONE;
			}
		};
		MenuRenderingInfo GetMenuRenderingInfo(HWND hWnd);

		bool HandlePopupMenuNCBorderColors(HDC hdc, bool useDarkMode, const RECT& paintRect);

		HDC GetCurrentMenuDC();
		HDC GetCurrentListviewDC();

		// Indicate that we are using UxTheme.dll to render menu item
		void NotifyUxThemeRendering();
		// Indicate that the menu window is using dark mode or light mode
		void NotifyMenuDarkMode(bool darkMode);
		// Indicate that it is a immersive style menu
		void NotifyMenuStyle(bool immersive);
		// Indicate its border color (Windows 11)
		void NotifyMenuBorderColor(COLORREF color);

		void Startup();
		void Shutdown();

		void Prepare(const TFMain::InteractiveIO& io);
	}
}