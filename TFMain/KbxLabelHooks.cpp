#include "pch.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "HookHelper.hpp"
#include "ThemeHelper.hpp"
#include "ApiEx.hpp"
#include "KbxLabelHooks.hpp"
#include "TooltipHandler.hpp"
#include "HookDispatcher.hpp"
#include "HookLocks.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::KbxLabelHooks
{
	using namespace std::literals;
	int WINAPI MyDrawTextW(
		HDC     hdc,
		LPCWSTR lpchText,
		int     cchText,
		LPRECT  lprc,
		UINT    format
	);
	BOOL WINAPI MyGdiGradientFill(
		HDC        hdc,
		PTRIVERTEX pVertex,
		ULONG      nVertex,
		PVOID      pMesh,
		ULONG      nCount,
		ULONG      ulMode
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
				"gdi32.dll"sv,
				"ext-ms-win-gdi-desktop-l1-1-0.dll"sv
			},
			"GdiGradientFill",
			reinterpret_cast<PVOID>(MyGdiGradientFill)
		}
	};
	HookHelper::HookDispatcher g_hookDispatcher
	{
		g_hookDependency
	};
}
int WINAPI KbxLabelHooks::MyDrawTextW(
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
BOOL WINAPI KbxLabelHooks::MyGdiGradientFill(
	HDC        hdc,
	PTRIVERTEX pVertex,
	ULONG      nVertex,
	PVOID      pMesh,
	ULONG      nCount,
	ULONG      ulMode
)
{
	RECT paintRect{};
	GetClipBox(hdc, &paintRect);
	PatBlt(hdc, paintRect.left, paintRect.top, wil::rect_width(paintRect), wil::rect_height(paintRect), BLACKNESS);
	return TRUE;
}

void KbxLabelHooks::Prepare()
{
}

void KbxLabelHooks::Startup()
{
}

void KbxLabelHooks::Shutdown()
{
	DisableHooks();
}

void KbxLabelHooks::EnableHooks(bool enable)
{
	auto lock{ RibbonHooks::g_lock.lock_exclusive() };

	g_hookDispatcher.moduleAddress = GetModuleHandleW(L"UIRibbon.dll");
	g_hookDispatcher.EnableHook(0, enable);
	g_hookDispatcher.EnableHook(1, enable);
}

void KbxLabelHooks::DisableHooks()
{
	auto lock{ RibbonHooks::g_lock.lock_exclusive() };

	g_hookDispatcher.DisableAllHooks();
}