#include "pch.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "HookHelper.hpp"
#include "ThemeHelper.hpp"
#include "ApiEx.hpp"
#include "TooltipHooks.hpp"
#include "TooltipHandler.hpp"
#include "HookDispatcher.hpp"
#include "HookLocks.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::TooltipHooks
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
	HRESULT WINAPI MyGetThemeMargins(
		HTHEME  hTheme,
		HDC     hdc,
		int     iPartId,
		int     iStateId,
		int     iPropId,
		LPCRECT prc,
		MARGINS* pMargins
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
				"ext-ms-win-uxtheme-themes-l1-1-0.dll"sv
			},
			"DrawThemeBackground",
			reinterpret_cast<PVOID>(MyDrawThemeBackground)
		},
		std::tuple
		{
			std::array
			{
				"uxtheme.dll"sv,
				"ext-ms-win-uxtheme-themes-l1-1-0.dll"sv
			},
			"DrawThemeTextEx",
			reinterpret_cast<PVOID>(MyDrawThemeTextEx)
		},
		std::tuple
		{
			std::array
			{
				"uxtheme.dll"sv,
				"ext-ms-win-uxtheme-themes-l1-1-0.dll"sv
			},
			"GetThemeMargins",
			reinterpret_cast<PVOID>(MyGetThemeMargins)
		}
	};
	HookHelper::HookDispatcher g_hookDispatcher
	{
		g_hookDependency
	};

	HMODULE g_comctl32Module{ nullptr };

	HANDLE GetOrCreateActCtx();
	void RunInActCtx(std::function<void()> callback);
}

int WINAPI TooltipHooks::MyDrawTextW(
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
		if ((format & DT_CALCRECT) || (format & DT_INTERNAL) || (format & DT_NOCLIP))
		{
			return false;
		}

		auto oldColor{ SetTextColor(hdc, TooltipHandler::g_tooltipContext.renderingContext.color) };
		auto cleanUp = wil::scope_exit([&]{ SetTextColor(hdc, oldColor); });
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
HRESULT WINAPI TooltipHooks::MyDrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	HRESULT hr{ S_OK };
	auto handler = [&]() -> bool
	{
		if (IsRectEmpty(pRect))
		{
			return false;
		}

		WCHAR themeClassName[MAX_PATH + 1]{};
		if (FAILED(ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)))
		{
			return false;
		}

		if (_wcsicmp(themeClassName, L"Tooltip") != 0)
		{
			return false;
		}

		if (iPartId != TTP_STANDARD)
		{
			return false;
		}

		RECT paintRect{ *pRect };
		if (pClipRect != nullptr)
		{
			IntersectRect(&paintRect, &paintRect, pClipRect);
		}
		PatBlt(hdc, paintRect.left, paintRect.top, paintRect.right - paintRect.left, paintRect.bottom - paintRect.top, BLACKNESS);

		return true;
	};
	if (!handler())
	{
		hr = g_hookDispatcher.GetOrg<decltype(&MyDrawThemeBackground), 1>()(
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
HRESULT WINAPI TooltipHooks::MyDrawThemeTextEx(
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
	const auto actualDrawThemeTextEx{ g_hookDispatcher.GetOrg<decltype(&MyDrawThemeTextEx), 2>()};

	WCHAR themeClassName[MAX_PATH + 1]{};
	if (SUCCEEDED(ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)) && !_wcsicmp(themeClassName, L"TreeView"))
	{
		TooltipHandler::g_tooltipContext.useDarkMode = ThemeHelper::ShouldAppsUseDarkMode() && ThemeHelper::IsDarkModeAllowedForApp();

		if (pOptions)
		{
			if (!(pOptions->dwFlags & (DTT_COMPOSITED)) && !(pOptions->dwFlags & (DTT_CALCRECT)))
			{
				DTTOPTS options = *pOptions;
				options.dwFlags |= DTT_COMPOSITED | DTT_TEXTCOLOR;
				options.crText = TooltipHandler::g_tooltipContext.renderingContext.color;
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
HRESULT WINAPI TooltipHooks::MyGetThemeMargins(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	int     iPropId,
	LPCRECT prc,
	MARGINS* pMargins
)
{
	HRESULT hr
	{
		g_hookDispatcher.GetOrg<decltype(&MyGetThemeMargins), 3>()(
			hTheme,
			hdc,
			iPartId,
			iStateId,
			iPropId,
			prc,
			pMargins
		)
	};

	WCHAR themeClassName[MAX_PATH + 1]{};
	if (
		g_hookDispatcher.IsHookEnabled(3) &&
		iStateId == 0 &&
		iPropId == TMT_CONTENTMARGINS &&
		prc == nullptr && 
		SUCCEEDED(ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)) &&
		!_wcsicmp(themeClassName, L"Tooltip")
	)
	{
		if (TooltipHandler::g_tooltipContext.renderingContext.marginsType == 0)
		{
			*pMargins =
			{
				TooltipHandler::g_tooltipContext.renderingContext.margins.cxLeftWidth + pMargins->cxLeftWidth,
				TooltipHandler::g_tooltipContext.renderingContext.margins.cxRightWidth + pMargins->cxRightWidth,
				TooltipHandler::g_tooltipContext.renderingContext.margins.cyTopHeight + pMargins->cyTopHeight,
				TooltipHandler::g_tooltipContext.renderingContext.margins.cyBottomHeight + pMargins->cyBottomHeight
			};
		}
		if (TooltipHandler::g_tooltipContext.renderingContext.marginsType == 1)
		{
			*pMargins = TooltipHandler::g_tooltipContext.renderingContext.margins;
		}
	}

	return hr;
}

HANDLE TooltipHooks::GetOrCreateActCtx()
{
	auto release_actctx = [](const HANDLE actCtx) { ReleaseActCtx(actCtx); };
	static wil::unique_any_handle_invalid<decltype(release_actctx), release_actctx> actCtxHandle
	{
		[]
		{
			WCHAR systemDirectory[MAX_PATH + 1]{};
			GetSystemDirectoryW(systemDirectory, _countof(systemDirectory));

			ACTCTXW actCtx
			{
				.cbSize{sizeof(ACTCTXW)}, 
				.dwFlags{
					ACTCTX_FLAG_RESOURCE_NAME_VALID |
					ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID
				},
				.lpSource{L"shell32.dll"},
				.lpAssemblyDirectory{systemDirectory},
				.lpResourceName{MAKEINTRESOURCEW(124)}
			};
			return CreateActCtxW(&actCtx);
		} ()
	};
	return actCtxHandle.get();
}
void TooltipHooks::RunInActCtx(std::function<void()> callback)
{
	auto actCtxHandle{ GetOrCreateActCtx() };
	ULONG_PTR cookie{ 0 };
	if (actCtxHandle != INVALID_HANDLE_VALUE && ActivateActCtx(actCtxHandle, &cookie))
	{
		callback();
		DeactivateActCtx(0, cookie);
	}
}

void TooltipHooks::Prepare()
{
}

void TooltipHooks::Startup()
{
	RunInActCtx([]
	{
		g_comctl32Module = GetModuleHandleW(L"comctl32.dll");
	});

	if (!g_comctl32Module)
	{
		return;
	}

	g_hookDispatcher.moduleAddress = g_comctl32Module;
	g_hookDispatcher.CacheHookData();
}

void TooltipHooks::Shutdown()
{
	if (!g_comctl32Module)
	{
		return;
	}

	DisableHooks();
}

void TooltipHooks::EnableHooks(bool enable)
{
	auto lock{ ExplorerFrameHooks::g_lock.lock_exclusive() };

	if (!g_comctl32Module)
	{
		return;
	}

	g_hookDispatcher.EnableHook(0, enable);
	g_hookDispatcher.EnableHook(1, enable);
	g_hookDispatcher.EnableHook(2, enable);
}

void TooltipHooks::EnableMarginHooks(bool enable)
{
	auto lock{ ExplorerFrameHooks::g_lock.lock_exclusive() };

	if (!g_comctl32Module)
	{
		return;
	}

	g_hookDispatcher.EnableHookNoRef(3, enable);
}

void TooltipHooks::DisableHooks()
{
	auto lock{ ExplorerFrameHooks::g_lock.lock_exclusive() };

	g_hookDispatcher.DisableAllHooks();
}