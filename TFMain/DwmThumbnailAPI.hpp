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

		static const auto g_actualDwmpQueryThumbnailType
		{
			reinterpret_cast<HRESULT(WINAPI*)(IN HTHUMBNAIL, OUT THUMBNAIL_TYPE*)>(
				GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), MAKEINTRESOURCEA(114))
			)
		};
		static const auto g_actualDwmpCreateSharedThumbnailVisual
		{
			reinterpret_cast <
			HRESULT(WINAPI*)(
				IN HWND hwndDestination,
				IN HWND hwndSource,
				IN DWORD dwThumbnailFlags,	// Pass 1 to get a thumbnail visual without hwndSource
											// 4 or 8 is invalid
				IN DWM_THUMBNAIL_PROPERTIES * pThumbnailProperties,
				IN VOID* pDCompDevice,
				OUT VOID** ppVisual,
				OUT PHTHUMBNAIL phThumbnailId
			)
			> (
				GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), MAKEINTRESOURCEA(147))
			)
		};
		static const auto g_actualDwmpQueryWindowThumbnailSourceSize
		{
			reinterpret_cast <
			HRESULT(WINAPI*)(
				IN HWND hwndSource,
				IN BOOL fSourceClientAreaOnly,
				OUT SIZE * pSize
			)
			> (
				GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), MAKEINTRESOURCEA(162))
			)
		};

		// PRE-IRON
		// 19043 or older
		static const auto g_actualDwmpCreateSharedVirtualDesktopVisual
		{
			reinterpret_cast <
			HRESULT(WINAPI*)(
				IN HWND hwndDestination,
				IN VOID* pDCompDevice,
				OUT VOID** ppVisual,
				OUT PHTHUMBNAIL phThumbnailId
			)
			> (
				GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), MAKEINTRESOURCEA(163))
			)
		};
		static const auto g_actualDwmpUpdateSharedVirtualDesktopVisual
		{
			reinterpret_cast <
			HRESULT(WINAPI*)(
				IN HTHUMBNAIL hThumbnailId,
				IN HWND * phwndsInclude,
				IN DWORD chwndsInclude,
				IN HWND * phwndsExclude,
				IN DWORD chwndsExclude,
				OUT RECT * prcSource,
				OUT SIZE * pDestinationSize
			)
			> (
				GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), MAKEINTRESOURCEA(164))
			)
		};

		// 20xxx+
		// No changes except for the function name.
		static const auto g_actualDwmpCreateSharedMultiWindowVisual
		{
			reinterpret_cast <
			HRESULT(WINAPI*)(
				IN HWND hwndDestination,
				IN VOID* pDCompDevice,
				OUT VOID** ppVisual,
				OUT PHTHUMBNAIL phThumbnailId
			)
			> (
				GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), MAKEINTRESOURCEA(163))
			)
		};
		// Change: function name + new DWORD parameter.
		// Pass "1" in dwFlags. Feel free to explore other flags.
		static const auto g_actualDwmpUpdateSharedMultiWindowVisual
		{
			reinterpret_cast <
			HRESULT(WINAPI*)(
				IN HTHUMBNAIL hThumbnailId,
				IN HWND * phwndsInclude,
				IN DWORD chwndsInclude,
				IN HWND * phwndsExclude,
				IN DWORD chwndsExclude,
				OUT RECT * prcSource,
				OUT SIZE * pDestinationSize,
				IN DWORD dwFlags
			)
			> (
				GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), MAKEINTRESOURCEA(164))
			)
		};
	}
}