#include "pch.h"
#include "Utils.hpp"
#include "HookHelper.hpp"
#include "SystemHelper.hpp"
#include "TraverseLogHooks.hpp"
#include "ThemeHelper.hpp"
#include "MenuHooks.hpp"
#include "MenuHandler.hpp"
#include "MenuRendering.hpp"
#include "HookDispatcher.hpp"
#include "HookLocks.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::TraverseLogHooks
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
		}
	};
	HookHelper::HookDispatcher g_hookDispatcher
	{
		g_hookDependency
	};
}

int WINAPI TraverseLogHooks::MyDrawTextW(
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
		if (MenuHandler::g_drawItemStruct == nullptr)
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

HRESULT WINAPI TraverseLogHooks::MyDrawThemeBackground(
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
		if (MenuHandler::g_drawItemStruct == nullptr)
		{
			return false;
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

void TraverseLogHooks::Prepare()
{

}
void TraverseLogHooks::Startup()
{

}
void TraverseLogHooks::Shutdown()
{
	DisableHooks();
}

void TraverseLogHooks::EnableHooks(bool enable)
{
	auto lock{ ExplorerFrameHooks::g_lock.lock_exclusive() };

	g_hookDispatcher.moduleAddress = GetModuleHandleW(L"explorerframe.dll");
	g_hookDispatcher.EnableHook(0, enable);
	g_hookDispatcher.EnableHook(1, enable);
}

void TraverseLogHooks::DisableHooks()
{
	auto lock{ ExplorerFrameHooks::g_lock.lock_exclusive() };

	g_hookDispatcher.DisableAllHooks();
}