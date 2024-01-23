#include "pch.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "HookHelper.hpp"
#include "MenuHooks.hpp"
#include "ImmersiveHooks.hpp"
#include "UxThemeHooks.hpp"
#include "ApiEx.hpp"
#include "TraverseLogHooks.hpp"
#include "SystemHelper.hpp"
#include "MenuRendering.hpp"
#include "MenuAppearance.hpp"
#include "FlyoutAnimation.hpp"
#include "MenuHandler.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::MenuHandler
{
	struct ContextMenuRenderingData
	{
		wchar_t* text;
		size_t length;
		size_t reserved;

		UINT menuFlags;
		UINT unknown1;

		HBITMAP hbmpItem;
		HBITMAP hbmpChecked;
		HBITMAP hbmpUnchecked;

		UINT cmpt;
		UINT scaleType;
		UINT dpi;

		bool useDarkTheme;
		bool useSystemPadding;

		void OutputDebugInfo()
		{
			OutputDebugStringW(
				std::format(
					L"ContextMenuRenderingData: {}, menuflags:{:#x}, unknown1:{}, bi:{}, bc:{}, buc:{}, cmpt:{}, scaleTyep:{}, dpi:{}, dark:{}, systemPadding:{}\n", 
					Utils::IsBadReadPtr(this->text) ? L"(bad ptr)" : this->text,
					this->menuFlags,
					this->unknown1,
					reinterpret_cast<void*>(this->hbmpItem),
					reinterpret_cast<void*>(this->hbmpChecked),
					reinterpret_cast<void*>(this->hbmpUnchecked),
					this->cmpt,
					this->scaleType,
					this->dpi,
					this->useDarkTheme,
					this->useSystemPadding
				).c_str()
			);
		}
	};

	constexpr int popupMenuArrowUp{ -3 };
	constexpr int popupMenuArrowDown{ -4 };

	constexpr UINT WM_UAHDESTROYWINDOW{ 0x0090 };
	constexpr UINT WM_UAHDRAWMENU{ 0x0091 };			// lParam is UAHMENU, return TRUE after handling it
	constexpr UINT WM_UAHDRAWMENUITEM{ 0x0092 };		// lParam is UAHDRAWMENUITEM, return TRUE after handling it
	constexpr UINT WM_UAHINITMENU{ 0x0093 };
	constexpr UINT WM_UAHMEASUREMENUITEM{ 0x0094 };	// lParam is UAHMEASUREMENUITEM, return TRUE after handling it
	constexpr UINT WM_UAHNCPAINTMENUPOPUP{ 0x0095 };	// lParam is UAHMENU, return TRUE after handling it

	void WalkMenuItems(HMENU hMenu, const std::function<bool(bool ownerDraw, ULONG_PTR itemData, UINT itemID)>&& callback);
	bool IsImmersiveMenuDataStored(HWND hWnd);
	bool IsDarkModeActiveForImmeriveMenu(bool useDarkTheme, bool useSystemPadding);
	ContextMenuRenderingData* GetContextMenuDataForItem(HWND hWnd, ULONG_PTR itemData, UINT itemID);
	MARGINS GetPopupMenuNonClientMargins(HWND hWnd);

	LRESULT CALLBACK MenuSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK MenuOwnerSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

void MenuHandler::Prepare()
{
	MenuHooks::Prepare();
	UxThemeHooks::Prepare();
	ImmersiveHooks::Prepare();
	TraverseLogHooks::Prepare();
}

void MenuHandler::Startup()
{
	MenuHooks::Startup();
	UxThemeHooks::Startup();
	ImmersiveHooks::Startup();
	TraverseLogHooks::Startup();

	Update();
}

void MenuHandler::Shutdown()
{
	HookHelper::Subclass::DetachAll<MenuSubclassProc>();
	HookHelper::ForceSubclass::DetachAll<MenuOwnerSubclassProc>();

	MenuHooks::Shutdown();
	UxThemeHooks::Shutdown();
	ImmersiveHooks::Shutdown();
	TraverseLogHooks::Shutdown();
}

void MenuHandler::Update()
{
	if (Api::IsPartDisabled(L"Menu")) [[unlikely]]
	{
		HookHelper::Subclass::DetachAll<MenuSubclassProc>();
		HookHelper::ForceSubclass::DetachAll<MenuOwnerSubclassProc>();

		TraverseLogHooks::DisableHooks();
		ImmersiveHooks::DisableHooks();
		UxThemeHooks::DisableHooks();
		MenuHooks::DisableHooks();

		return;
	}

	MenuHooks::EnableHooks(true);

	if (!RegHelper::Get<DWORD>({ L"Menu" }, L"NoModernAppBackgroundColor", 1))
	{
		MenuHooks::EnableIconHooks(false);
	}
	else
	{
		MenuHooks::EnableIconHooks(true);
	}
}

void CALLBACK MenuHandler::HandleWinEvent(
	HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
	LONG idObject, LONG idChild,
	DWORD dwEventThread, DWORD dwmsEventTime
)
{
	if (Api::IsPartDisabled(L"Menu")) [[unlikely]]
	{
		return;
	}
	
	if (dwEvent == EVENT_OBJECT_CREATE)
	{
		if (Utils::IsPopupMenu(hWnd))
		{
			HookHelper::Subclass::Attach<MenuSubclassProc>(hWnd, true);
		}
	}
}


void MenuHandler::WalkMenuItems(HMENU hMenu, const std::function<bool(bool ownerDraw, ULONG_PTR itemData, UINT itemID)>&& callback)
{
	MENUITEMINFOW mii{};

	for (UINT i = 0; ; ++i)
	{
		mii = {
			.cbSize = sizeof(mii),
			.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID
		};
		if (!GetMenuItemInfoW(hMenu, i, TRUE, &mii) || !callback(WI_IsFlagSet(mii.fType, MFT_OWNERDRAW), mii.dwItemData, mii.wID))
		{
			break;
		}
	}
}

bool MenuHandler::IsImmersiveMenuDataStored(HWND hWnd)
{
	bool result{false};
	EnumPropsExW(hWnd, [](HWND hWnd, LPWSTR lpString, HANDLE hData, ULONG_PTR lParam)
	{
		if (HIWORD(lpString) && wcsstr(lpString, L"ImmersiveContextMenuArray_"))
		{
			*reinterpret_cast<bool*>(lParam) = true;
			return FALSE;
		}
		return TRUE;
	}, reinterpret_cast<LPARAM>(&result));

	return result;
}

bool MenuHandler::IsDarkModeActiveForImmeriveMenu(bool useDarkTheme, bool useSystemPadding)
{
	if (!useDarkTheme)
	{
		if (!useSystemPadding)
		{
			return ThemeHelper::ShouldAppsUseDarkMode() && ThemeHelper::IsDarkModeAllowedForApp();
		}

		return false;
	}

	return true;
}

MenuHandler::ContextMenuRenderingData* MenuHandler::GetContextMenuDataForItem(HWND hWnd, ULONG_PTR itemData, UINT itemID)
{
	return reinterpret_cast<MenuHandler::ContextMenuRenderingData*>(
		GetPropW(
			hWnd,
			std::format(
				L"ImmersiveContextMenuArray_{}-{}",
				static_cast<ULONG>(itemData),
				static_cast<ULONG>(itemID)
			).c_str()
		)
	);
}

MARGINS MenuHandler::GetPopupMenuNonClientMargins(HWND hWnd)
{
	RECT windowRect{};
	GetWindowRect(hWnd, &windowRect);

	MENUBARINFO mbi{ sizeof(MENUBARINFO) };
	GetMenuBarInfo(hWnd, OBJID_CLIENT, 0, &mbi);

	return { mbi.rcBar.left - windowRect.left, windowRect.right - mbi.rcBar.right, mbi.rcBar.top - windowRect.top, windowRect.bottom - mbi.rcBar.bottom };
}

LRESULT CALLBACK MenuHandler::MenuSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_WINDOWPOSCHANGED)
	{
		const auto& windowPos{ *reinterpret_cast<WINDOWPOS*>(lParam) };
		if (windowPos.flags & SWP_SHOWWINDOW)
		{
			auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };

			if (g_menuContext.noSystemDropShadow)
			{
				HWND backdropWindow{ GetWindow(hWnd, GW_HWNDNEXT) };
				if (Utils::IsWindowClass(backdropWindow, L"SysShadow"))
				{
					ShowWindow(backdropWindow, SW_HIDE);
				}
			}

			return result;
		}
	}
	// We need to fix the menu selection fade animation...
	// Actually this animation can also be triggered when the user hover on the popup menu
	// at a fast speed and then activate a menu item by a keyboard accelerator
	if (uMsg == MN_BUTTONUP)
	{
		try
		{
			// User didn't click on the menu item
			auto position{ static_cast<int>(wParam) };
			THROW_HR_IF(
				E_INVALIDARG,
				((position & 0xFFFF'FFFF'FFFF'FFF0) == 0xFFFF'FFFF'FFFF'FFF0)
			);

			int param{ 0 };
			SystemParametersInfoW(SPI_GETSELECTIONFADE, 0, &param, 0);
			// Fade out animation is disabled
			THROW_HR_IF(E_INVALIDARG, !param);

			HMENU hMenu{ reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0)) };
			MENUITEMINFO mii{ sizeof(MENUITEMINFO), MIIM_SUBMENU | MIIM_STATE | MIIM_FTYPE };
			THROW_IF_WIN32_BOOL_FALSE(GetMenuItemInfo(hMenu, position, TRUE, &mii));

			// There is no animation when user clicked on following item
			// 1. disabled item
			// 2. separator
			// 3. the sub menu
			THROW_HR_IF(
				E_INVALIDARG,
				(mii.fType & MFT_SEPARATOR) ||
				(mii.fState & MFS_DISABLED) ||
				mii.hSubMenu
			);

			MENUBARINFO mbi{ sizeof(MENUBARINFO) };
			THROW_IF_WIN32_BOOL_FALSE(GetMenuBarInfo(hWnd, OBJID_CLIENT, position + 1, &mbi));

			THROW_IF_FAILED(
				FlyoutAnimation::CreateMenuFadeOut(
					hWnd,
					mbi,
					std::chrono::milliseconds{ g_menuContext.animation.fadeOutTime }
				)
			);

			SystemParametersInfoW(SPI_SETSELECTIONFADE, 0, FALSE, 0);
			auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };
			SystemParametersInfoW(SPI_SETSELECTIONFADE, 0, reinterpret_cast<PVOID>(TRUE), 0);
			return result;
		}
		catch(...) {}
	}
	if (uMsg == WM_PRINT)
	{
		if (g_menuContext.type == MenuContext::Type::Immersive || g_menuContext.type == MenuContext::Type::TraverseLog)
		{
			auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };

			POINT pt{};
			Utils::unique_ext_hdc hdc{ reinterpret_cast<HDC>(wParam) };

			MARGINS mr{ GetPopupMenuNonClientMargins(hWnd) };

			RECT paintRect{};
			GetWindowRect(hWnd, &paintRect);
			OffsetRect(&paintRect, -paintRect.left, -paintRect.top);

			if (g_menuContext.border.cornerType == DWM_WINDOW_CORNER_PREFERENCE::DWMWCP_DONOTROUND)
			{
				MenuRendering::HandlePopupMenuNonClientBorderColors(hdc.get(), paintRect);
			}
			else
			{
				FrameRect(hdc.get(), &paintRect, GetStockBrush(BLACK_BRUSH));
			}

			return result;
		}
	}
	if (uMsg == WM_NCPAINT)
	{
		if (g_menuContext.type == MenuContext::Type::Immersive || g_menuContext.type == MenuContext::Type::TraverseLog)
		{
			auto hdc{ wil::GetWindowDC(hWnd) };

			if (wParam != NULLREGION && wParam != ERROR)
			{
				SelectClipRgn(hdc.get(), reinterpret_cast<HRGN>(wParam));
			}

			MARGINS mr{ GetPopupMenuNonClientMargins(hWnd) };

			RECT paintRect{};
			GetWindowRect(hWnd, &paintRect);
			OffsetRect(&paintRect, -paintRect.left, -paintRect.top);

			{
				Utils::unique_ext_hdc dc{ hdc.get() };
				ExcludeClipRect(dc.get(), paintRect.left + mr.cxLeftWidth, paintRect.top + mr.cyTopHeight, paintRect.right - mr.cxRightWidth, paintRect.bottom - mr.cyBottomHeight);
				PatBlt(dc.get(), paintRect.left, paintRect.top, wil::rect_width(paintRect), wil::rect_height(paintRect), BLACKNESS);
			}

			if (g_menuContext.border.cornerType == DWM_WINDOW_CORNER_PREFERENCE::DWMWCP_DONOTROUND && !MenuRendering::HandlePopupMenuNonClientBorderColors(hdc.get(), paintRect))
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
	}

	if (uMsg == WM_UAHDRAWMENU || uMsg == WM_UAHNCPAINTMENUPOPUP || uMsg == WM_UAHDRAWMENUITEM || uMsg == WM_UAHMEASUREMENUITEM)
	{
		g_uxhooksEnabled = true;
		auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };
		g_uxhooksEnabled = false;

		return result;
	}
	if (uMsg == WM_UAHINITMENU)
	{
		g_uxhooksEnabled = true;
		auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };
		g_uxhooksEnabled = false;

		if (result && g_menuContext.useUxTheme)
		{
			g_menuContext.type = MenuContext::Type::NativeTheme;
		}
		else
		{
			g_menuContext.type = MenuContext::Type::Classic;
			g_menuContext.useDarkMode = false;
		}

		return result;
	}
	if (uMsg == MN_SIZEWINDOW)
	{
		auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };

		HookHelper::HwndCallOnce(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), L"MN_SIZEWINDOW", [&]
		{
			g_menuContext.type = MenuContext::Type::ClassicOrNativeTheme;
			g_menuContext.useDarkMode = false;

			// determine menu type and mode
			HWND menuOwner{ Utils::GetCurrentMenuOwner() };
			auto menu{ reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0)) };
			if (!GetPropW(menuOwner, L"COwnerDrawPopupMenu_This"))
			{
				WalkMenuItems(menu, [&](bool ownerDraw, ULONG_PTR itemData, UINT itemID) -> bool
				{
					if (ownerDraw)
					{
						ContextMenuRenderingData* menuData{ nullptr };
						if (IsImmersiveMenuDataStored(menuOwner) && itemData && (menuData = GetContextMenuDataForItem(menuOwner, itemData, itemID)))
						{
							g_menuContext.type = MenuContext::Type::Immersive;
							g_menuContext.useDarkMode = IsDarkModeActiveForImmeriveMenu(menuData->useDarkTheme, menuData->useSystemPadding);

#ifdef _DEBUG
							menuData->OutputDebugInfo();
#endif // _DEBUG
						}
						else
						{
#ifdef _DEBUG
							OutputDebugStringW(std::format(L"itemData: {}, itemID: {}\n", itemData, itemID).c_str());
#endif // _DEBUG
							g_menuContext.type = MenuContext::Type::ThirdParty;
						}

						return false;
					}

					return true;
				});
			}
			else
			{
				g_menuContext.type = MenuContext::Type::TraverseLog;
				g_menuContext.useDarkMode = ThemeHelper::ShouldAppsUseDarkMode() && ThemeHelper::IsDarkModeAllowedForApp();
			}

			if (g_menuContext.type == MenuContext::Type::Immersive || g_menuContext.type == MenuContext::Type::TraverseLog)
			{
				MENUINFO mi{ sizeof(mi), MIM_BACKGROUND };
				if (GetMenuInfo(menu, &mi))
				{
					HookHelper::HwndProp::Set(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), menuOriginalBackgroundBrush, reinterpret_cast<HANDLE>(mi.hbrBack));
					mi.hbrBack = GetStockBrush(BLACK_BRUSH);
					SetMenuInfo(menu, &mi);
				}

				HookHelper::HwndProp::Set(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), menuOwnerAttached);
				HookHelper::ForceSubclass::Attach<MenuOwnerSubclassProc>(menuOwner, true);
			}
			if (g_menuContext.type == MenuContext::Type::ClassicOrNativeTheme)
			{
				UxThemeHooks::EnableHooks(true);
				HookHelper::HwndProp::Set(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), uxHooksAttached);

				// invoke WM_UAHINITMENU immediately!
				g_menuContext.useUxTheme = false;
				#pragma warning(suppress : 6387)
				PrintWindow(hWnd, nullptr, 0);
			}
			if (g_menuContext.type == MenuContext::Type::ThirdParty || g_menuContext.type == MenuContext::Type::Classic)
			{
				// This menu is using unknown owner drawn technique,
				// in order to prevent broken visual content, we need to detach and remove menu backdrop
				HookHelper::Subclass::Attach<MenuSubclassProc>(hWnd, false);
			}

			// read settings...
			g_menuContext.noSystemDropShadow = RegHelper::Get<DWORD>(
				{ L"Menu" },
				L"NoSystemDropShadow",
				0
			) != 0;
			g_menuContext.immersiveStyle = RegHelper::Get<DWORD>(
				{ L"Menu" },
				L"EnableImmersiveStyle",
				1
			) != 0;
			// backdrop effect
			Api::QueryBackdropEffectContext(L"Menu", g_menuContext.useDarkMode, g_menuContext.backdropEffect);
			// border part
			Api::QueryBorderContext(L"Menu", g_menuContext.useDarkMode, g_menuContext.border);
			// custom rendering
			Api::QueryMenuCustomRenderingContext(g_menuContext.useDarkMode, g_menuContext.customRendering);
			// icon background color removal
			Api::QueryMenuIconBackgroundColorRemovalContext(g_menuContext.iconBackgroundColorRemoval);
			// animation
			Api::QueryFlyoutAnimationContext(L"Menu", g_menuContext.animation);

			if (g_menuContext.type >= MenuContext::Type::NativeTheme)
			{
				// apply backdrop effect
				Api::ApplyEffect(hWnd, g_menuContext.useDarkMode, g_menuContext.backdropEffect, g_menuContext.border);
			}
			// We have menu scroll arrows, make it redraw itself.
			if (GetPopupMenuNonClientMargins(hWnd).cyTopHeight != MenuAppearance::nonClientMarginStandardSize)
			{
				PostMessageW(hWnd, MN_SELECTITEM, popupMenuArrowUp, 0);
				PostMessageW(hWnd, MN_SELECTITEM, popupMenuArrowDown, 0);

				SetWindowPos(
					hWnd,
					nullptr,
					0, 0,
					0, 0,
					SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE
				);
			}
			if (g_menuContext.animation.enable)
			{
				FlyoutAnimation::CreateMenuPopupIn(
					hWnd,
					static_cast<float>(g_menuContext.animation.startRatio) / 100.f,
					std::chrono::milliseconds{ g_menuContext.animation.popInTime },
					std::chrono::milliseconds{ g_menuContext.animation.fadeInTime },
					g_menuContext.animation.popInStyle
				);
			}

			return;
		});

		return result;
	}

	if (uMsg == HookHelper::TFM_ATTACH)
	{
		return 0;
	}
	if (uMsg == HookHelper::TFM_DETACH)
	{
		if (HookHelper::HwndProp::Get(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), uxHooksAttached))
		{
			UxThemeHooks::EnableHooks(false);
			HookHelper::HwndProp::Unset(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), uxHooksAttached);
		}
		if (g_menuContext.type == MenuContext::Type::Immersive)
		{
			MENUINFO mi{ .cbSize{sizeof(mi)}, .fMask{MIM_BACKGROUND}, .hbrBack{HookHelper::HwndProp::Get<HBRUSH>(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), menuOriginalBackgroundBrush)} };
			SetMenuInfo(reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0)), &mi);
			HookHelper::HwndProp::Unset(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), menuOriginalBackgroundBrush);
		}

		if (HookHelper::HwndProp::Get(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), menuOwnerAttached))
		{
			HookHelper::ForceSubclass::Attach<MenuOwnerSubclassProc>(Utils::GetCurrentMenuOwner(), false);
			HookHelper::HwndProp::Unset(hWnd, HookHelper::Subclass::GetNamespace<MenuSubclassProc>(), menuOwnerAttached);
		}

		if (wParam == 0 && g_menuContext.type >= MenuContext::Type::NativeTheme)
		{
			Api::DropEffect(L"Menu", hWnd);
		}

		return 0;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MenuHandler::MenuOwnerSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DRAWITEM)
	{
		g_drawItemStruct = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
		auto cleanUp = wil::scope_exit([&]
		{
			g_drawItemStruct = nullptr;
		});

		if (g_drawItemStruct->CtlType == ODT_MENU)
		{
			if (g_menuContext.type == MenuContext::Type::Immersive)
			{
				wil::unique_hbitmap my_hbmpItem{ nullptr };
				wil::unique_hbitmap my_hbmpChecked{ nullptr };
				wil::unique_hbitmap my_hbmpUnchecked{ nullptr };

				auto menuData{ GetContextMenuDataForItem(hWnd, g_drawItemStruct->itemData, g_drawItemStruct->itemID) };
#ifdef _DEBUG
				menuData->OutputDebugInfo();
#endif // _DEBUG
				HBITMAP hbmpItem{ menuData->hbmpItem };
				HBITMAP hbmpChecked{ menuData->hbmpChecked };
				HBITMAP hbmpUnchecked{ menuData->hbmpUnchecked };
				auto cleanUp = wil::scope_exit([&]
				{
					menuData->hbmpItem = hbmpItem;
					menuData->hbmpChecked = hbmpChecked;
					menuData->hbmpUnchecked = hbmpUnchecked;
				});

				if (!MenuRendering::HandleMenuBitmap(menuData->hbmpItem, my_hbmpItem))
				{
					if (hbmpChecked == hbmpUnchecked)
					{
						if (MenuRendering::HandleMenuBitmap(menuData->hbmpChecked, my_hbmpChecked))
						{
							menuData->hbmpUnchecked = my_hbmpChecked.get();
						}
					}
					else
					{
						MenuRendering::HandleMenuBitmap(menuData->hbmpChecked, my_hbmpChecked);
						MenuRendering::HandleMenuBitmap(menuData->hbmpUnchecked, my_hbmpUnchecked);
					}
				}

				auto result{ HookHelper::ForceSubclass::Storage<MenuOwnerSubclassProc>::CallOriginalWndProc(hWnd, uMsg, wParam, lParam) };
				return result;
			}
			if (g_menuContext.type == MenuContext::Type::TraverseLog)
			{
				auto result{ HookHelper::ForceSubclass::Storage<MenuOwnerSubclassProc>::CallOriginalWndProc(hWnd, uMsg, wParam, lParam) };
				return result;
			}
		}
	}

	if (uMsg == HookHelper::TFM_ATTACH)
	{
		if (g_menuContext.type == MenuContext::Type::Immersive)
		{
			ImmersiveHooks::EnableHooks(reinterpret_cast<PVOID>(MenuHooks::g_targetModule), true);
		}
		if (g_menuContext.type == MenuContext::Type::TraverseLog)
		{
			TraverseLogHooks::EnableHooks(true);
		}
		return 0;
	}
	if (uMsg == HookHelper::TFM_DETACH)
	{
		if (g_menuContext.type == MenuContext::Type::Immersive)
		{
			ImmersiveHooks::EnableHooks(reinterpret_cast<PVOID>(MenuHooks::g_targetModule), false);
		}
		if (g_menuContext.type == MenuContext::Type::TraverseLog)
		{
			TraverseLogHooks::EnableHooks(false);
		}
		return 0;
	}

	return HookHelper::ForceSubclass::Storage<MenuOwnerSubclassProc>::CallOriginalWndProc(hWnd, uMsg, wParam, lParam);
}