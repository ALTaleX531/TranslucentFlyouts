#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	// A class that aims to handling related drawing messages properly and privide backdrop effect
	class MenuHandler
	{
	public:
		// describes the sizes of the menu bar or menu item
		union UAHMENUITEMMETRICS
		{
			struct
			{
				DWORD cx;
				DWORD cy;
			} rgsizeBar[2];
			struct
			{
				DWORD cx;
				DWORD cy;
			} rgsizePopup[4];
		};

		// not really used in our case but part of the other structures
		struct UAHMENUPOPUPMETRICS
		{
			DWORD rgcx[4];
			DWORD fUpdateMaxWidths : 2; // from kernel symbols, padded to full dword
		};

		// hmenu is the main window menu; hdc is the context to draw in
		struct UAHMENU
		{
			HMENU hMenu;
			HDC hdc;
			DWORD dwFlags; // no idea what these mean, in my testing it's either 0x00000a00 or sometimes 0x00000a10
		};

		// menu items are always referred to by iPosition here
		struct UAHMENUITEM
		{
			int iPosition; // 0-based position of menu item in menubar
			UAHMENUITEMMETRICS umim;
			UAHMENUPOPUPMETRICS umpm;
		};

		// the DRAWITEMSTRUCT contains the states of the menu items, as well as
		// the position index of the item in the menu, which is duplicated in
		// the UAHMENUITEM's iPosition as well
		struct UAHDRAWMENUITEM
		{
			DRAWITEMSTRUCT dis; // itemID looks uninitialized
			UAHMENU um;
			UAHMENUITEM umi;
		};

		// the MEASUREITEMSTRUCT is intended to be filled with the size of the item
		// height appears to be ignored, but width can be modified
		struct UAHMEASUREMENUITEM
		{
			MEASUREITEMSTRUCT mis;
			UAHMENU um;
			UAHMENUITEM umi;
		};

		struct MenuRenderingInfo
		{
			bool useUxTheme{false};
			bool useDarkMode{false};
			bool immersive{false};
			COLORREF borderColor{DWMWA_COLOR_NONE};

			inline void Reset() { useUxTheme = false; useDarkMode = false; immersive = false; }
		};
		
		static MenuHandler& GetInstance();
		~MenuHandler() noexcept;
		MenuHandler(const MenuHandler&) = delete;
		MenuHandler& operator=(const MenuHandler&) = delete;

		static HDC GetCurrentMenuDC();
		static HDC GetCurrentListviewDC();

		// Indicate that we are using UxTheme.dll to render menu item
		static void NotifyUxThemeRendering();
		// Indicate that the menu window is using dark mode or light mode
		static void NotifyMenuDarkMode(bool darkMode);
		// Indicate that it is a immersive style menu
		static void NotifyMenuStyle(bool immersive);
		// Indicate its border color (Windows 11)
		static void NotifyMenuBorderColor(COLORREF color);

		void StartupHook();
		void ShutdownHook();


		MenuRenderingInfo GetMenuRenderingInfo(HWND hWnd);
		void ApplyEffect(std::wstring_view keyName, HWND hWnd, bool darkMode, bool noDropShadow = false);
		void HandleSysBorderColors(std::wstring_view keyName, HWND hWnd, bool useDarkMode, COLORREF color);
		void HandleRoundCorners(std::wstring_view keyName, HWND hWnd);
		bool HandlePopupMenuNCBorderColors(HDC hdc, bool useDarkMode, const RECT& paintRect);

		static constexpr DWORD lightMode_GradientColor{0x9EDDDDDD};
		static constexpr DWORD darkMode_GradientColor{0x412B2B2B};

		static constexpr DWORD lightMode_HotColor{0x30000000};
		static constexpr DWORD darkMode_HotColor{0x41808080};

		static constexpr DWORD lightMode_DisabledHotColor{0x00000000};
		static constexpr DWORD darkMode_DisabledHotColor{0x00000000};

		static constexpr DWORD lightMode_SeparatorColor{0x30262626};
		static constexpr DWORD darkMode_SeparatorColor{0x30D9D9D9};

		static constexpr DWORD lightMode_FocusingColor{0xFF000000};
		static constexpr DWORD darkMode_FocusingColor{0xFFFFFFFF};

		static constexpr DWORD focusingWidth{1000};
		static constexpr DWORD cornerRadius{8};
		static constexpr DWORD separatorWidth{1000};

		static constexpr int systemOutlineSize{1};
	private:
		struct MenuRenderingContext
		{
			HDC menuDC{nullptr};
			HDC listviewDC{nullptr};
		};

		MenuHandler();
		static void WinEventCallback(HWND hWnd, DWORD event);
		// In certain situations, using SetWindowSubclass can't receive WM_DRAWITEM (eg. Windows 11 Taskmgr),
		// so here we use hooks instead
		static void MenuOwnerMsgCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid);
		static void ListviewpopupMsgCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid);
		
		static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
		static bool IsImmersiveContextMenu(HWND hWnd);

		void AttachDropDown(HWND hWnd);
		void DetachDropDown(HWND hWnd);

		void AttachPopupMenuOwner(HWND hWnd);
		void DetachPopupMenuOwner(HWND hWnd);

		void AttachPopupMenu(HWND hWnd);
		void DetachPopupMenu(HWND hWnd);

		void AttachListViewPopup(HWND hWnd);
		void DetachListViewPopup(HWND hWnd);

		bool IsMenuPartlyOwnerDrawn(HMENU hMenu);

		static constexpr UINT WM_UAHDESTROYWINDOW{0x0090};
		static constexpr UINT WM_UAHDRAWMENU{0x0091};			// lParam is UAHMENU, return TRUE after handling it
		static constexpr UINT WM_UAHDRAWMENUITEM{0x0092};		// lParam is UAHDRAWMENUITEM, return TRUE after handling it
		static constexpr UINT WM_UAHINITMENU{0x0093};
		static constexpr UINT WM_UAHMEASUREMENUITEM{0x0094};	// lParam is UAHMEASUREMENUITEM, return TRUE after handling it
		static constexpr UINT WM_UAHNCPAINTMENUPOPUP{0x0095};	// lParam is UAHMENU, return TRUE after handling it

		static constexpr UINT MN_SETHMENU{0x01E0};
#ifndef MN_GETHMENU
		static constexpr UINT MN_GETHMENU{0x01E1};
#endif // !MN_GETHMENU
		static constexpr UINT MN_SIZEWINDOW{0x01E2};
		static constexpr UINT MN_OPENHIERARCHY{0x01E3};
		static constexpr UINT MN_CLOSEHIERARCHY{0x01E4};
		static constexpr UINT MN_SELECTITEM{0x01E5};
		static constexpr UINT MN_CANCELMENUS{0x01E6};
		static constexpr UINT MN_SELECTFIRSTVALIDITEM{0x01E7};
		static constexpr UINT MN_GETPOPUPMENU{0x01EA};	// obsolete
		static constexpr UINT MN_FINDMENUWINDOWFROMPOINT{0x01EB};
		static constexpr UINT MN_SHOWPOPUPWINDOW{0x01EC};
		static constexpr UINT MN_MOUSEMOVE{0x01EE};
		static constexpr UINT MN_BUTTONDOWN{0x01ED};
		static constexpr UINT MN_BUTTONUP{0x01EF};
		static constexpr UINT MN_SETTIMERTOOPENHIERARCHY{0x01F0};
		static constexpr UINT MN_DBLCLK{0x01F1};
		static constexpr UINT MN_ENDMENU{0x01F2};
		static constexpr UINT MN_DODRAGDROP{0x01F3};
		static constexpr UINT MN_ENDMENU2{0x01F4};

		static constexpr int nonClientMarginSize{3};
		static constexpr int popupMenuSubclassId{0};
		static constexpr int dropDownSubclassId{0};

		static thread_local MenuRenderingContext g_sharedContext;
		static thread_local MenuRenderingInfo g_sharedMenuInfo;

		static const UINT WM_MHDETACH;

		bool m_internalError{false};
		std::list<HWND> m_hookedWindowList{};
		std::list<HWND> m_menuList{};
	};
}