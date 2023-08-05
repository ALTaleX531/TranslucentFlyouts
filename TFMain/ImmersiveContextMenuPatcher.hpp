#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	// A class that aims to tweak the appearance of immersive context menu
	class ImmersiveContextMenuPatcher
	{
	public:
		static ImmersiveContextMenuPatcher& GetInstance();
		~ImmersiveContextMenuPatcher() noexcept;
		ImmersiveContextMenuPatcher(const ImmersiveContextMenuPatcher&) = delete;
		ImmersiveContextMenuPatcher& operator=(const ImmersiveContextMenuPatcher&) = delete;

		void StartupHook();
		void ShutdownHook();
	private:
		ImmersiveContextMenuPatcher();
		static int WINAPI DrawTextW(
			HDC     hdc,
			LPCWSTR lpchText,
			int     cchText,
			LPRECT  lprc,
			UINT    format
		);
		static HRESULT WINAPI DrawThemeBackground(
			HTHEME  hTheme,
			HDC     hdc,
			int     iPartId,
			int     iStateId,
			LPCRECT pRect,
			LPCRECT pClipRect
		);
		static BOOL WINAPI BitBlt(
			HDC   hdc,
			int   x,
			int   y,
			int   cx,
			int   cy,
			HDC   hdcSrc,
			int   x1,
			int   y1,
			DWORD rop
		);
		static BOOL WINAPI StretchBlt(
			HDC   hdcDest,
			int   xDest,
			int   yDest,
			int   wDest,
			int   hDest,
			HDC   hdcSrc,
			int   xSrc,
			int   ySrc,
			int   wSrc,
			int   hSrc,
			DWORD rop
		);

		static void DllNotificationCallback(bool load, Hooking::DllNotifyRoutine::DllInfo info);

		void DoIATHook(PVOID moduleBaseAddress);
		void UndoIATHook(PVOID moduleBaseAddress);

		decltype(DrawTextW)* m_actualDrawTextW{nullptr};
		decltype(DrawThemeBackground)* m_actualDrawThemeBackground{nullptr};
		decltype(BitBlt)* m_actualBitBlt{nullptr};
		decltype(StretchBlt)* m_actualStretchBlt{nullptr};

		bool m_internalError{false};
		bool m_startup{false};
		bool m_hooked{false};
		size_t m_hookedFunctions{0};
	};
}