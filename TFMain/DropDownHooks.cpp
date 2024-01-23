#include "pch.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "HookHelper.hpp"
#include "ThemeHelper.hpp"
#include "ApiEx.hpp"
#include "DropDownHooks.hpp"
#include "DropDownHandler.hpp"
#include "MenuRendering.hpp"
#include "HookDispatcher.hpp"
#include "HookLocks.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::DropDownHooks
{
	using namespace std::literals;
	int WINAPI MyDrawTextW(
		HDC     hdc,
		LPCWSTR lpchText,
		int     cchText,
		LPRECT  lprc,
		UINT    format
	);
	HRESULT WINAPI MyDrawThemeBackground(
		HTHEME  hTheme,
		HDC     hdc,
		int     iPartId,
		int     iStateId,
		LPCRECT pRect,
		LPCRECT pClipRect
	);
	HRESULT WINAPI MyDrawThemeTextEx(
		HTHEME        hTheme,
		HDC           hdc,
		int           iPartId,
		int           iStateId,
		LPCWSTR       pszText,
		int           cchText,
		DWORD         dwTextFlags,
		LPRECT        pRect,
		const DTTOPTS* pOptions
	);

	HookHelper::HookDispatcherDependency g_hookDependency
	{
		std::tuple
		{
			std::array
			{
				"user32.dll"sv,
				"ext-ms-win-ntuser-draw-l1-1-0.dll"sv
			},
			"DrawTextW",
			reinterpret_cast<PVOID>(MyDrawTextW)
		},
		std::tuple
		{
			std::array
			{
				"uxtheme.dll"sv,
				"ext-ms-win-ntuser-draw-l1-1-0.dll"sv
			},
			"DrawThemeBackground",
			reinterpret_cast<PVOID>(MyDrawThemeBackground)
		},
		std::tuple
		{
			std::array
			{
				"uxtheme.dll"sv,
				"ext-ms-win-ntuser-draw-l1-1-0.dll"sv
			},
			"DrawThemeTextEx",
			reinterpret_cast<PVOID>(MyDrawThemeTextEx)
		}
	};
	HookHelper::HookDispatcher g_hookDispatcher
	{
		g_hookDependency
	};
}

int WINAPI DropDownHooks::MyDrawTextW(
	HDC     hdc,
	LPCWSTR lpchText,
	int     cchText,
	LPRECT  lprc,
	UINT    format
)
{
	int result{ 0 };
	auto handler = [&]() -> bool
	{
		if (DropDownHandler::g_drawItemStruct == nullptr)
		{
			return false;
		}
		if ((format & DT_CALCRECT) || (format & DT_INTERNAL) || (format & DT_NOCLIP))
		{
			return false;
		}
		if (
			FAILED(
				ThemeHelper::DrawTextWithAlpha(
					hdc,
					lpchText,
					cchText,
					lprc,
					format,
					result
				)
			)
		)
		{
			return false;
		}

		return true;
	};
	if (!handler())
	{
		result = g_hookDispatcher.GetOrg<decltype(&MyDrawTextW), 0>()(hdc, lpchText, cchText, lprc, format);
	}

	return result;
}
HRESULT WINAPI DropDownHooks::MyDrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	HRESULT hr{ S_OK };
	auto actualDrawThemeBackground{ g_hookDispatcher.GetOrg<decltype(&MyDrawThemeBackground), 1>() };

	auto handler = [&]() -> bool
	{
		if (DropDownHandler::g_drawItemStruct == nullptr)
		{
			return false;
		}

		WCHAR themeClassName[MAX_PATH + 1]{};
		if (SUCCEEDED(ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)) && !_wcsicmp(themeClassName, L"ListviewPopup"))
		{
			RECT clipRect{ *pRect };
			if (pClipRect)
			{
				IntersectRect(&clipRect, &clipRect, pClipRect);
			}

			PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS);
			return true;
		}

		return MenuRendering::HandleDrawThemeBackground(
			hTheme, hdc, iPartId, iStateId, pRect, pClipRect,
			actualDrawThemeBackground
		);
	};
	if (!handler())
	{
		hr = actualDrawThemeBackground(
			hTheme,
			hdc,
			iPartId,
			iStateId,
			pRect,
			pClipRect
		);
	}

	return hr;
}
HRESULT WINAPI DropDownHooks::MyDrawThemeTextEx(
	HTHEME        hTheme,
	HDC           hdc,
	int           iPartId,
	int           iStateId,
	LPCWSTR       pszText,
	int           cchText,
	DWORD         dwTextFlags,
	LPRECT        pRect,
	const DTTOPTS* pOptions
)
{
	const auto actualDrawThemeTextEx{ g_hookDispatcher.GetOrg<decltype(&MyDrawThemeTextEx), 2>() };

	WCHAR themeClassName[MAX_PATH + 1]{};
	if (SUCCEEDED(ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)))
	{
		if (pOptions)
		{
			if (!(pOptions->dwFlags & (DTT_COMPOSITED)) && !(pOptions->dwFlags & (DTT_CALCRECT)))
			{
				DTTOPTS options = *pOptions;
				options.dwFlags |= DTT_COMPOSITED;
				return ThemeHelper::DrawThemeContent(
					hdc,
					*pRect,
					nullptr,
					nullptr,
					0,
					[&](HDC memoryDC, HPAINTBUFFER, RGBQUAD*, int)
					{
						actualDrawThemeTextEx(hTheme, memoryDC, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, &options);
					}
				);
			}
		}
		else
		{
			DTTOPTS options{ sizeof(DTTOPTS) };
			return MyDrawThemeTextEx(
				hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, &options
			);
		}
	}

	return actualDrawThemeTextEx(
		hTheme,
		hdc,
		iPartId,
		iStateId,
		pszText,
		cchText,
		dwTextFlags,
		pRect,
		pOptions
	);
}

void DropDownHooks::Prepare()
{

}
void DropDownHooks::Startup()
{

}
void DropDownHooks::Shutdown()
{
	DisableHooks();
}

void DropDownHooks::EnableHooks(bool enable)
{
	auto lock{ ExplorerFrameHooks::g_lock.lock_exclusive() };

	g_hookDispatcher.moduleAddress = GetModuleHandleW(L"explorerframe.dll");
	g_hookDispatcher.EnableHook(0, enable);
	g_hookDispatcher.EnableHook(1, enable);
	g_hookDispatcher.EnableHook(2, enable);
}

void DropDownHooks::DisableHooks()
{
	auto lock{ ExplorerFrameHooks::g_lock.lock_exclusive() };

	g_hookDispatcher.DisableAllHooks();
}