#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	// A class that aims to patch the calls to related Theme APIs within UxTheme.dll
	class UxThemePatcher
	{
	public:
		static UxThemePatcher& GetInstance();
		~UxThemePatcher() noexcept;
		UxThemePatcher(const UxThemePatcher&) = delete;
		UxThemePatcher& operator=(const UxThemePatcher&) = delete;

		void StartupHook();
		void ShutdownHook();
		static void PrepareUxTheme();
		static bool IsUxThemeAPIOffsetReady();
	private:
		UxThemePatcher();
		static HRESULT WINAPI DrawThemeBackground(
			HTHEME  hTheme,
			HDC     hdc,
			int     iPartId,
			int     iStateId,
			LPCRECT pRect,
			LPCRECT pClipRect
		);
		static HRESULT WINAPI DrawThemeText(
			HTHEME hTheme,
			HDC hdc,
			int iPartId,
			int iStateId,
			LPCWSTR pszText,
			int cchText,
			DWORD dwTextFlags,
			DWORD,
			LPCRECT pRect
		);
		struct CThemeMenu
		{
			void __thiscall DrawItemBitmap(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, int iStateId, const RECT* lprc);
			void __thiscall DrawItemBitmap2(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, bool noStretch, int iStateId, const RECT* lprc);	// Windows 11
		};

		static void InitUxThemeOffset() noexcept(false);

#pragma data_seg("uxthemeOffset")
		static int g_uxthemeVersion;
		static DWORD64 g_CThemeMenuPopup_DrawItem_Offset;
		static DWORD64 g_CThemeMenuPopup_DrawItemCheck_Offset;
		static DWORD64 g_CThemeMenuPopup_DrawClientArea_Offset;
		static DWORD64 g_CThemeMenuPopup_DrawNonClientArea_Offset;
		static DWORD64 g_CThemeMenu_DrawItemBitmap_Offset;
#pragma data_seg()
#pragma comment(linker,"/SECTION:uxthemeOffset,RWS")

		bool m_hooked{false};
		bool m_internalError{false};

		[[maybe_unused]] PVOID m_actualCThemeMenuPopup_DrawItem{nullptr};
		[[maybe_unused]] PVOID m_actualCThemeMenuPopup_DrawItemCheck{nullptr};
		[[maybe_unused]] PVOID m_actualCThemeMenuPopup_DrawClientArea{nullptr};
		[[maybe_unused]] PVOID m_actualCThemeMenuPopup_DrawNonClientArea{nullptr};
		[[maybe_unused]] PVOID m_actualCThemeMenu_DrawItemBitmap{nullptr};

		decltype(DrawThemeBackground)* m_actualDrawThemeBackground{nullptr};
		decltype(DrawThemeText)* m_actualDrawThemeText{nullptr};

		Hooking::FunctionCallHook m_callHook{};
	};
}