#include "pch.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "HookHelper.hpp"
#include "FlyoutAnimation.hpp"
#include "DropDownHandler.hpp"
#include "DropDownHooks.hpp"
#include "SystemHelper.hpp"
#include "MenuAppearance.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::DropDownHandler
{
	bool HandleDropDownNonClientBorderColors(HDC hdc, const RECT& paintRect)
	{
		if (!g_dropDownContext.border.colorUseNone)
		{
			if (g_dropDownContext.border.colorUseDefault)
			{
				return false;
			}

			// Border color is enabled.
			wil::unique_hbrush brush{ Utils::CreateSolidColorBrushWithAlpha(Utils::MakeCOLORREF(g_dropDownContext.border.color), Utils::GetAlphaFromARGB(g_dropDownContext.border.color)) };
			if (brush)
			{
				FrameRect(hdc, &paintRect, brush.get());
			}
		}
		else
		{
			FrameRect(hdc, &paintRect, GetStockBrush(BLACK_BRUSH));
		}

		return true;
	};

	LRESULT CALLBACK DropDownSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ListviewpopupSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
}

LRESULT CALLBACK DropDownHandler::DropDownSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_NCPAINT)
	{
		auto hdc{ wil::GetWindowDC(hWnd) };

		if (wParam != NULLREGION && wParam != ERROR)
		{
			SelectClipRgn(hdc.get(), reinterpret_cast<HRGN>(wParam));
		}

		MARGINS mr{ 0, 0, 0, 0 };

		RECT paintRect{};
		GetWindowRect(hWnd, &paintRect);
		OffsetRect(&paintRect, -paintRect.left, -paintRect.top);

		{
			Utils::unique_ext_hdc dc{ hdc.get() };
			ExcludeClipRect(dc.get(), paintRect.left + mr.cxLeftWidth, paintRect.top + mr.cyTopHeight, paintRect.right - mr.cxRightWidth, paintRect.bottom - mr.cyBottomHeight);
			PatBlt(dc.get(), paintRect.left, paintRect.top, wil::rect_width(paintRect), wil::rect_height(paintRect), BLACKNESS);
		}

		if (g_dropDownContext.border.cornerType == DWM_WINDOW_CORNER_PREFERENCE::DWMWCP_DONOTROUND && !HandleDropDownNonClientBorderColors(hdc.get(), paintRect))
		{
			RECT windowRect{};
			GetWindowRect(hWnd, &windowRect);

			wil::unique_hrgn windowRegion{ CreateRectRgnIndirect(&windowRect) };
			wil::unique_hrgn windowRegionWithoutOutline{ CreateRectRgn(windowRect.left + MenuAppearance::systemOutlineSize, windowRect.top + MenuAppearance::systemOutlineSize, windowRect.right - MenuAppearance::systemOutlineSize, windowRect.bottom - MenuAppearance::systemOutlineSize) };
			CombineRgn(windowRegion.get(), windowRegion.get(), windowRegionWithoutOutline.get(), RGN_XOR);

			DefSubclassProc(hWnd, WM_NCPAINT, reinterpret_cast<WPARAM>(windowRegion.get()), 0);
		}

		return 0;
	}

	if (uMsg == HookHelper::TFM_ATTACH)
	{
		return 0;
	}
	if (uMsg == HookHelper::TFM_DETACH)
	{
		return 0;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DropDownHandler::ListviewpopupSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_PAINT)
	{
		DropDownHooks::EnableHooks(true);
		auto result{DefSubclassProc(hWnd, uMsg, wParam, lParam)};
		DropDownHooks::EnableHooks(false);

		return result;
	}
	if (uMsg == WM_ERASEBKGND)
	{
		HDC hdc{ reinterpret_cast<HDC>(wParam) };
		RECT clipRect{};
		GetClipBox(hdc, &clipRect);
		PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS);
		return true;
	}
	if (uMsg == WM_DRAWITEM)
	{
		g_drawItemStruct = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
		auto cleanUp = wil::scope_exit([&]
		{
			g_drawItemStruct = nullptr;
		});

		if (g_drawItemStruct->CtlType == ODT_LISTVIEW)
		{
			DropDownHooks::EnableHooks(true);
			auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };
			DropDownHooks::EnableHooks(false);

			return result;
		}
	}

	if (uMsg == HookHelper::TFM_ATTACH)
	{
		HWND root{GetAncestor(hWnd, GA_ROOT)};

		g_dropDownContext.useDarkMode = ThemeHelper::IsDarkModeAllowedForWindow(hWnd);
		// backdrop effect
		Api::QueryBackdropEffectContext(L"DropDown", g_dropDownContext.useDarkMode, g_dropDownContext.backdropEffect);
		// border part
		Api::QueryBorderContext(L"DropDown", g_dropDownContext.useDarkMode, g_dropDownContext.border);
		// animation
		Api::QueryFlyoutAnimationContext(L"DropDown", g_dropDownContext.animation);

		HookHelper::Subclass::Attach<DropDownSubclassProc>(root, true);
		// apply backdrop effect
		Api::ApplyEffect(root, g_dropDownContext.useDarkMode, g_dropDownContext.backdropEffect, g_dropDownContext.border);
		if (g_dropDownContext.animation.enable)
		{
			FlyoutAnimation::CreateDropDownPopupIn(
				root,
				static_cast<float>(g_dropDownContext.animation.startRatio) / 100.f,
				std::chrono::milliseconds{ g_dropDownContext.animation.popInTime },
				std::chrono::milliseconds{ g_dropDownContext.animation.fadeInTime },
				g_dropDownContext.animation.popInStyle
			);
		}

		return 0;
	}
	if (uMsg == HookHelper::TFM_DETACH)
	{
		return 0;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void DropDownHandler::Prepare()
{
	DropDownHooks::Prepare();
}
void DropDownHandler::Startup()
{
	DropDownHooks::Startup();

	Update();
}
void DropDownHandler::Shutdown()
{
	HookHelper::Subclass::DetachAll<ListviewpopupSubclassProc>();
	HookHelper::Subclass::DetachAll<DropDownSubclassProc>();

	DropDownHooks::Shutdown();
}
void DropDownHandler::Update()
{
	if (Api::IsPartDisabled(L"DropDown")) [[unlikely]]
	{
		HookHelper::Subclass::DetachAll<ListviewpopupSubclassProc>();
		HookHelper::Subclass::DetachAll<DropDownSubclassProc>();
		DropDownHooks::DisableHooks();

		return;
	}
}

void CALLBACK DropDownHandler::HandleWinEvent(
	HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
	LONG idObject, LONG idChild,
	DWORD dwEventThread, DWORD dwmsEventTime
)
{
	if (Api::IsPartDisabled(L"DropDown")) [[unlikely]]
	{
		return;
	}

	if (dwEvent == EVENT_OBJECT_CREATE)
	{
		if (Utils::IsWindowClass(hWnd, L"Listviewpopup"))
		{
			HookHelper::Subclass::Attach<ListviewpopupSubclassProc>(hWnd, true);
		}
	}
}