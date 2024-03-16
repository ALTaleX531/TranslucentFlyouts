#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace DwmThumbnailAPI
	{
		enum class THUMBNAIL_TYPE
		{
			TT_DEFAULT,
			TT_SNAPSHOT,
			TT_ICONIC,
			TT_BITMAPPENDING,
			TT_BITMAP
		};

		constexpr UINT DWM_TNP_FREEZE{0x100000};
		constexpr UINT DWM_TNP_ENABLE3D{0x4000000};
		constexpr UINT DWM_TNP_DISABLE3D{0x8000000};
		constexpr UINT DWM_TNP_FORCECVI{0x40000000};
		constexpr UINT DWM_TNP_DISABLEFORCECVI{0x80000000};

		inline const HRESULT(WINAPI* g_actualDwmpQueryThumbnailType)(IN HTHUMBNAIL, OUT THUMBNAIL_TYPE*);
		inline const HRESULT(WINAPI* g_actualDwmpCreateSharedThumbnailVisual)(
			IN HWND hwndDestination,
			IN HWND hwndSource,
			IN DWORD dwThumbnailFlags,	// Pass 1 to get a thumbnail visual without hwndSource
			// 4 or 8 is invalid
			IN DWM_THUMBNAIL_PROPERTIES* pThumbnailProperties,
			IN VOID* pDCompDevice,
			OUT VOID** ppVisual,
			OUT PHTHUMBNAIL phThumbnailId
		);
		inline const HRESULT(WINAPI* g_actualDwmpQueryWindowThumbnailSourceSize)(
			IN HWND hwndSource,
			IN BOOL fSourceClientAreaOnly,
			OUT SIZE* pSize
		);

		// PRE-IRON
		// 19043 or older
		inline const HRESULT(WINAPI* g_actualDwmpCreateSharedVirtualDesktopVisual)(
			IN HWND hwndDestination,
			IN VOID* pDCompDevice,
			OUT VOID** ppVisual,
			OUT PHTHUMBNAIL phThumbnailId
		);
		inline const HRESULT(WINAPI* g_actualDwmpUpdateSharedVirtualDesktopVisual)(
			IN HTHUMBNAIL hThumbnailId,
			IN HWND* phwndsInclude,
			IN DWORD chwndsInclude,
			IN HWND* phwndsExclude,
			IN DWORD chwndsExclude,
			OUT RECT* prcSource,
			OUT SIZE* pDestinationSize
		);

		// 20xxx+
		// No changes except for the function name.
		inline const HRESULT(WINAPI* g_actualDwmpCreateSharedMultiWindowVisual)(
			IN HWND hwndDestination,
			IN VOID* pDCompDevice,
			OUT VOID** ppVisual,
			OUT PHTHUMBNAIL phThumbnailId
		);
		// Change: function name + new DWORD parameter.
		// Pass "1" in dwFlags. Feel free to explore other flags.
		inline const HRESULT(WINAPI* g_actualDwmpUpdateSharedMultiWindowVisual)(
			IN HTHUMBNAIL hThumbnailId,
			IN HWND* phwndsInclude,
			IN DWORD chwndsInclude,
			IN HWND* phwndsExclude,
			IN DWORD chwndsExclude,
			OUT RECT* prcSource,
			OUT SIZE* pDestinationSize,
			IN DWORD dwFlags
		);

		inline HRESULT Initialize() try
		{
			HMODULE dwmModule{ GetModuleHandleW(L"dwmapi.dll") };
			THROW_LAST_ERROR_IF_NULL(dwmModule);
			g_actualDwmpQueryThumbnailType = reinterpret_cast<decltype(g_actualDwmpQueryThumbnailType)>(GetProcAddress(dwmModule, MAKEINTRESOURCEA(114)));
			THROW_LAST_ERROR_IF_NULL(g_actualDwmpQueryThumbnailType);
			g_actualDwmpCreateSharedThumbnailVisual = reinterpret_cast<decltype(g_actualDwmpCreateSharedThumbnailVisual)>(GetProcAddress(dwmModule, MAKEINTRESOURCEA(147)));
			THROW_LAST_ERROR_IF_NULL(g_actualDwmpCreateSharedThumbnailVisual);
			g_actualDwmpQueryWindowThumbnailSourceSize = reinterpret_cast<decltype(g_actualDwmpQueryWindowThumbnailSourceSize)>(GetProcAddress(dwmModule, MAKEINTRESOURCEA(162)));
			THROW_LAST_ERROR_IF_NULL(g_actualDwmpQueryWindowThumbnailSourceSize);
			g_actualDwmpCreateSharedMultiWindowVisual = g_actualDwmpCreateSharedVirtualDesktopVisual = reinterpret_cast<decltype(g_actualDwmpCreateSharedVirtualDesktopVisual)>(GetProcAddress(dwmModule, MAKEINTRESOURCEA(163)));
			THROW_LAST_ERROR_IF_NULL(g_actualDwmpCreateSharedVirtualDesktopVisual);
			g_actualDwmpUpdateSharedMultiWindowVisual = reinterpret_cast<decltype(g_actualDwmpUpdateSharedMultiWindowVisual)>(g_actualDwmpUpdateSharedVirtualDesktopVisual = reinterpret_cast<decltype(g_actualDwmpUpdateSharedVirtualDesktopVisual)>(GetProcAddress(dwmModule, MAKEINTRESOURCEA(164))));
			THROW_LAST_ERROR_IF_NULL(g_actualDwmpUpdateSharedVirtualDesktopVisual);
			return S_OK;
		}
		CATCH_RETURN()
	}
}