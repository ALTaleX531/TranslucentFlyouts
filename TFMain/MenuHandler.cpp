#include "pch.h"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuAnimation.hpp"

namespace TranslucentFlyouts
{
	using namespace std;
	thread_local decltype(MenuHandler::g_sharedDC) MenuHandler::g_sharedDC{nullptr, nullptr};
	thread_local decltype(MenuHandler::g_sharedMenuInfo) MenuHandler::g_sharedMenuInfo{false, false};

	const UINT MenuHandler::WM_MHDETACH{RegisterWindowMessageW(L"TranslucentFlyouts.MenuHandler.Detach")};
}

TranslucentFlyouts::MenuHandler& TranslucentFlyouts::MenuHandler::GetInstance()
{
	static MenuHandler instance{};
	return instance;
}

TranslucentFlyouts::MenuHandler::MenuHandler()
{

}

TranslucentFlyouts::MenuHandler::~MenuHandler() noexcept
{
	ShutdownHook();
}

void TranslucentFlyouts::MenuHandler::MenuOwnerMsgCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid)
{
	if (message == WM_DRAWITEM)
	{
		auto& drawItemStruct{*reinterpret_cast<LPDRAWITEMSTRUCT>(lParam)};

		if (!returnResultValid)
		{
			if (wParam == 0 && drawItemStruct.CtlType == ODT_MENU)
			{
				g_sharedDC.menuDC = drawItemStruct.hDC;
			}
			if (drawItemStruct.CtlType == ODT_LISTVIEW && Utils::IsWindowClass(GetParent(drawItemStruct.hwndItem), L"Listviewpopup"))
			{
				g_sharedDC.listviewDC = drawItemStruct.hDC;
			}
		}

		if (returnResultValid)
		{
			if (wParam == 0 && drawItemStruct.CtlType == ODT_MENU)
			{
				g_sharedDC.menuDC = nullptr;
			}
			if (drawItemStruct.CtlType == ODT_LISTVIEW && Utils::IsWindowClass(GetParent(drawItemStruct.hwndItem), L"Listviewpopup"))
			{
				g_sharedDC.listviewDC = nullptr;
			}
		}
	}
}

void TranslucentFlyouts::MenuHandler::ListviewpopupMsgCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid)
{
	DWORD itemDisabled
	{
		RegHelper::GetDword(
			L"DropDown",
			L"Disabled",
			0
		)
	};

	if (itemDisabled)
	{
		g_sharedDC.listviewDC = nullptr;
		return;
	}

	if (message == WM_ERASEBKGND && returnResultValid)
	{
		if (Utils::IsWindowClass(hwnd, L"Listviewpopup"))
		{
			HDC hdc{reinterpret_cast<HDC>(wParam)};
			RECT paintRect{};
			GetClipBox(hdc, &paintRect);
			FillRect(hdc, &paintRect, GetStockBrush(BLACK_BRUSH));
		}
	}

	if (message == WM_DRAWITEM)
	{
		auto& drawItemStruct{*reinterpret_cast<LPDRAWITEMSTRUCT>(lParam)};

		if (!returnResultValid)
		{
			if (drawItemStruct.CtlType == ODT_LISTVIEW && hwnd == GetParent(drawItemStruct.hwndItem))
			{
				g_sharedDC.listviewDC = drawItemStruct.hDC;
			}
		}

		if (returnResultValid)
		{
			if (drawItemStruct.CtlType == ODT_LISTVIEW && hwnd == GetParent(drawItemStruct.hwndItem))
			{
				g_sharedDC.listviewDC = nullptr;
			}
		}
	}
}

LRESULT CALLBACK TranslucentFlyouts::MenuHandler::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	bool handled{false};
	LRESULT result{0};

	// Popup menu
	if (uIdSubclass == popupMenuSubclassId)
	{
		// If we can handle WM_UAHDRAWMENU, WM_UAHDRAWMENUITEM, WM_UAHNCPAINTMENUPOPUP properly, (like StartAllBack)
		// then we don't need TranslucentFlyouts::UxThemePatcher anymore!!!
		// However, this method may break the vanilla behaviour of popup menu
		if (uMsg == WM_UAHDRAWMENU)
		{
			handled = true;

			auto& uah{*reinterpret_cast<UAHMENU*>(lParam)};
			g_sharedDC.menuDC = uah.hdc;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			g_sharedDC.menuDC = nullptr;
		}

		if (uMsg == WM_UAHDRAWMENUITEM)
		{
			handled = true;

			auto& uah{*reinterpret_cast<UAHDRAWMENUITEM*>(lParam)};
			g_sharedDC.menuDC = uah.dis.hDC;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			g_sharedDC.menuDC = nullptr;
		}

		if (uMsg == WM_UAHINITMENU)
		{
			handled = true;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}

		if (uMsg == WM_UAHNCPAINTMENUPOPUP)
		{
			handled = true;

			auto& uah{*reinterpret_cast<UAHMENU*>(lParam)};
			g_sharedDC.menuDC = uah.hdc;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			g_sharedDC.menuDC = nullptr;
		}

		// We need to fix the menu selection fade animation...
		// Actually this animation can also be triggered when the user hover on the popup menu
		// at a fast speed and then activate a menu item by a keyboard accelerator
		if (uMsg == MN_BUTTONUP)
		{
			try
			{
				// User didn't click on the menu item
				auto position{static_cast<int>(wParam)};
				THROW_HR_IF(E_INVALIDARG, position == 0xFFFFFFFF);

				int param{0};
				SystemParametersInfoW(SPI_GETSELECTIONFADE, 0, &param, 0);
				// Fade out animation is disabled
				THROW_HR_IF(E_INVALIDARG, !param);

				HMENU hMenu{reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0))};
				MENUITEMINFO mii{sizeof(MENUITEMINFO), MIIM_SUBMENU | MIIM_STATE | MIIM_FTYPE};
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

				MENUBARINFO mbi{sizeof(MENUBARINFO)};
				THROW_IF_WIN32_BOOL_FALSE(GetMenuBarInfo(hWnd, OBJID_CLIENT, position + 1, &mbi));

				THROW_IF_FAILED(
					MenuAnimation::CreateFadeOut(
						hWnd, mbi, MenuAnimation::standardFadeoutDuration
					)
				);

				handled = true;
				SystemParametersInfoW(SPI_SETSELECTIONFADE, 0, FALSE, 0);
				result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				SystemParametersInfoW(SPI_SETSELECTIONFADE, 0, reinterpret_cast<PVOID>(TRUE), 0);
			}
			CATCH_LOG()
		}

		if (uMsg == MN_SIZEWINDOW)
		{
			handled = true;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

			HWND menuOwner{Utils::GetCurrentMenuOwner()};

			if (menuOwner != nullptr)
			{
				SetWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId, reinterpret_cast<DWORD_PTR>(menuOwner));
				Hooking::MsgHooks::GetInstance().Install(menuOwner);
				GetInstance().m_hookedWindowList.push_back(menuOwner);
			}

			// This menu is using unknown owner drawn technique,
			// in order to prevent broken visual content, we need to detach and remove menu backdrop
			try
			{
				g_sharedMenuInfo.useUxTheme = false;
				g_sharedMenuInfo.useDarkMode = false;

				{
					wil::unique_hdc memoryDC{CreateCompatibleDC(nullptr)};
					THROW_LAST_ERROR_IF_NULL(memoryDC);
					wil::unique_hbitmap bitmap{CreateCompatibleBitmap(memoryDC.get(), 1, 1)};
					THROW_LAST_ERROR_IF_NULL(bitmap);

					{
						auto selectedObject{wil::SelectObject(memoryDC.get(), bitmap.get())};
						THROW_IF_WIN32_BOOL_FALSE(PrintWindow(hWnd, memoryDC.get(), 0));
					}

					THROW_HR_IF(E_NOTIMPL, !g_sharedMenuInfo.useUxTheme);
				}
			}
			catch (...)
			{
				LOG_CAUGHT_EXCEPTION();
				SendNotifyMessage(hWnd, WM_MHDETACH, 0, 0);
				return result;
			}

			DWORD effectType
			{
				RegHelper::GetDword(
					L"Menu",
					L"EffectType",
					static_cast<DWORD>(EffectHelper::EffectType::ModernAcrylicBlur)
				)
			};

			DWORD enableDropShadow
			{
				RegHelper::GetDword(
					L"Menu",
					L"EnableDropShadow",
					0
				)
			};

			// Set effect for the popup menu
			DWORD opacity{0};
			DWORD gradientColor{0};
			if (g_sharedMenuInfo.useDarkMode)
			{
				opacity = RegHelper::GetDword(
							  L"Menu",
							  L"DarkMode_Opacity",
							  65
						  );
				gradientColor = RegHelper::GetDword(
									L"Menu",
									L"DarkMode_GradientColor",
									0x2B2B2B
								);

				EffectHelper::EnableWindowDarkMode(hWnd, TRUE);
				EffectHelper::SetWindowBackdrop(hWnd, enableDropShadow, (gradientColor | (opacity << 24)), effectType);
			}
			else
			{
				opacity = RegHelper::GetDword(
							  L"Menu",
							  L"LightMode_Opacity",
							  158
						  );
				gradientColor = RegHelper::GetDword(
									L"Menu",
									L"LightMode_GradientColor",
									0xDDDDDD
								);

				EffectHelper::SetWindowBackdrop(hWnd, enableDropShadow, (gradientColor | (opacity << 24)), effectType);
			}

			// Whether to remove the system outline?
			DWORD noOutline
			{
				RegHelper::GetDword(
					L"Menu",
					L"NoSystemOutline",
					0,
					false
				)
			};
			if (noOutline)
			{
				SetPropW(hWnd, L"IsZachMenuDWMAttributeSet", reinterpret_cast<HANDLE>(HANDLE_FLAG_INHERIT));

				DWORD noBorderColor
				{
					RegHelper::GetDword(
						L"Menu\\Border",
						L"NoBorderColor",
						0,
						false
					)
				};

				DWORD borderColor{DWMWA_COLOR_NONE};
				if (noBorderColor != 1)
				{
					if (g_sharedMenuInfo.useDarkMode)
					{
						borderColor = RegHelper::GetDword(
										  L"Menu\\Border",
										  L"DarkMode_Color",
										  0x303030
									  );
					}
					else
					{
						borderColor = RegHelper::GetDword(
										  L"Menu\\Border",
										  L"LightMode_Color",
										  0xD9D9D9
									  );
					}

					try
					{
						DWORD enableThemeColorization
						{
							RegHelper::GetDword(
								L"Menu\\Border",
								L"EnableThemeColorization",
								0,
								false
							)
						};

						DWORD opacity{0};

						THROW_HR_IF(E_NOTIMPL, !enableThemeColorization);
						THROW_IF_FAILED(Utils::GetDwmThemeColor(borderColor, opacity));
					}
					catch (...)
					{
						if (wil::ResultFromCaughtException() != E_NOTIMPL)
						{
							LOG_CAUGHT_EXCEPTION();
						}
					}
				}

				DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));

				DWORD cornerType
				{
					RegHelper::GetDword(
						L"Menu\\Border",
						L"CornerType",
						3,
						false
					)
				};
				if (cornerType == 0)
				{
					cornerType = 3;
				}
				auto roundCorner{static_cast<DWM_WINDOW_CORNER_PREFERENCE>(cornerType)};
				DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &roundCorner, sizeof(roundCorner));
			}
		}

		if (uMsg == WM_PRINT)
		{
			if (IsImmersiveContextMenu(hWnd))
			{
				handled = true;

				POINT pt{};
				Utils::unique_ext_hdc hdc{reinterpret_cast<HDC>(wParam)};

				RECT paintRect{};
				GetClipBox(hdc.get(), &paintRect);
				FillRect(hdc.get(), &paintRect, GetStockBrush(BLACK_BRUSH));

				SetViewportOrgEx(hdc.get(), nonClientMarginSize, nonClientMarginSize, &pt);
				result = DefSubclassProc(hWnd, WM_PRINTCLIENT, wParam, lParam);

				// Whether to keep the system outline?
				DWORD noOutline
				{
					RegHelper::GetDword(
						L"Menu",
						L"NoSystemOutline",
						0,
						false
					)
				};
				if (noOutline == 0)
				{
					Utils::unique_ext_hdc dc{hdc.get()};
					SetViewportOrgEx(dc.get(), pt.x, pt.y, nullptr);
					ExcludeClipRect(dc.get(), paintRect.left + systemOutlineSize, paintRect.top + systemOutlineSize, paintRect.right - systemOutlineSize, paintRect.bottom - systemOutlineSize);
					result = DefSubclassProc(hWnd, WM_PRINT, wParam, lParam);
				}
				else
				{
					SetViewportOrgEx(hdc.get(), pt.x, pt.y, nullptr);
					hdc.reset(reinterpret_cast<HDC>(wParam));

					try
					{
						DWORD enableThemeColorization
						{
							RegHelper::GetDword(
								L"Menu\\Border",
								L"EnableThemeColorization",
								0,
								false
							)
						};

						COLORREF color{0};
						DWORD opacity{0};

						THROW_HR_IF(E_NOTIMPL, !enableThemeColorization);
						THROW_IF_FAILED(Utils::GetDwmThemeColor(color, opacity));
						wil::unique_hbrush brush{Utils::CreateSolidColorBrushWithAlpha(color, static_cast<std::byte>(opacity))};
						THROW_LAST_ERROR_IF_NULL(brush);
						THROW_LAST_ERROR_IF(FrameRect(hdc.get(), &paintRect, brush.get()) == 0);
					}
					catch (...)
					{
						if (wil::ResultFromCaughtException() != E_NOTIMPL)
						{
							LOG_CAUGHT_EXCEPTION();
						}
					}
				}
			}
		}

		if (uMsg == WM_NCPAINT)
		{
			if (IsImmersiveContextMenu(hWnd))
			{
				handled = true;

				auto hdc{wil::GetWindowDC(hWnd)};

				if (wParam != NULLREGION && wParam != ERROR)
				{
					SelectClipRgn(hdc.get(), reinterpret_cast<HRGN>(wParam));
				}

				RECT paintRect{};
				GetClipBox(hdc.get(), &paintRect);
				FillRect(hdc.get(), &paintRect, GetStockBrush(BLACK_BRUSH));

				// Whether to keep the system outline?
				DWORD noOutline
				{
					RegHelper::GetDword(
						L"Menu",
						L"NoSystemOutline",
						0,
						false
					)
				};
				if (noOutline == 0)
				{
					Utils::unique_ext_hdc dc{hdc.get()};
					ExcludeClipRect(dc.get(), paintRect.left + systemOutlineSize, paintRect.top + systemOutlineSize, paintRect.right - systemOutlineSize, paintRect.bottom - systemOutlineSize);
					result = DefSubclassProc(hWnd, WM_PRINT, reinterpret_cast<WPARAM>(dc.get()), lParam);
				}
				else
				{
					try
					{
						DWORD enableThemeColorization
						{
							RegHelper::GetDword(
								L"Menu\\Border",
								L"EnableThemeColorization",
								0,
								false
							)
						};

						COLORREF color{0};
						DWORD opacity{0};

						THROW_HR_IF(E_NOTIMPL, !enableThemeColorization);
						THROW_IF_FAILED(Utils::GetDwmThemeColor(color, opacity));
						wil::unique_hbrush brush{Utils::CreateSolidColorBrushWithAlpha(color, static_cast<std::byte>(opacity))};
						THROW_LAST_ERROR_IF_NULL(brush);
						THROW_LAST_ERROR_IF(FrameRect(hdc.get(), &paintRect, brush.get()) == 0);
					}
					catch (...)
					{
						if (wil::ResultFromCaughtException() != E_NOTIMPL)
						{
							LOG_CAUGHT_EXCEPTION();
						}
					}
				}
			}
		}

		if (uMsg == WM_ERASEBKGND)
		{
			if (IsImmersiveContextMenu(hWnd))
			{
				handled = true;

				HDC hdc{reinterpret_cast<HDC>(wParam)};

				RECT paintRect{};
				GetClipBox(hdc, &paintRect);
				FillRect(hdc, &paintRect, GetStockBrush(BLACK_BRUSH));
			}
		}

		if (uMsg == WM_WINDOWPOSCHANGED)
		{
			WINDOWPOS& wp{*reinterpret_cast<LPWINDOWPOS>(lParam)};
			if (!(wp.flags & SWP_NOMOVE))
			{
				DwmTransitionOwnedWindow(hWnd, DWMTRANSITION_OWNEDWINDOW_TARGET::DWMTRANSITION_OWNEDWINDOW_REPOSITION);
			}
		}

		if (uMsg == WM_NCDESTROY || uMsg == WM_MHDETACH)
		{
			HWND menuOwner{reinterpret_cast<HWND>(dwRefData)};
			if (uMsg == WM_MHDETACH)
			{
				handled = true;
				RemovePropW(hWnd, L"IsZachMenuDWMAttributeSet");
				EffectHelper::SetWindowBackdrop(hWnd, FALSE, 0, static_cast<DWORD>(EffectHelper::EffectType::None));
				InvalidateRect(hWnd, nullptr, TRUE);
			}
			GetInstance().DetachPopupMenu(hWnd);
			Hooking::MsgHooks::GetInstance().Uninstall(menuOwner);
			GetInstance().m_hookedWindowList.remove(menuOwner);
		}
	}


	if (uIdSubclass == dropDownSubclassId)
	{
		if (uMsg == WM_NCDESTROY || uMsg == WM_MHDETACH)
		{
			if (uMsg == WM_MHDETACH)
			{
				handled = true;
				EffectHelper::SetWindowBackdrop(hWnd, FALSE, 0, static_cast<DWORD>(EffectHelper::EffectType::None));
				InvalidateRect(hWnd, nullptr, TRUE);
			}
			GetInstance().DetachDropDown(hWnd);
		}
	}

	if (!handled)
	{
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	return result;
}

void TranslucentFlyouts::MenuHandler::AttachPopupMenu(HWND hWnd)
{
	DWORD itemDisabled
	{
		RegHelper::GetDword(
			L"Menu",
			L"Disabled",
			0
		)
	};

	if (itemDisabled)
	{
		return;
	}
	m_menuList.push_back(hWnd);
	SetWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId, 0);
}

void TranslucentFlyouts::MenuHandler::DetachPopupMenu(HWND hWnd)
{
	RemoveWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId);
	m_menuList.remove(hWnd);
}

void TranslucentFlyouts::MenuHandler::AttachDropDown(HWND hWnd)
{
	// DropDown doesn't support dark mode, so here we only get the values used in light mode
	DWORD effectType
	{
		RegHelper::GetDword(
			L"DropDown",
			L"EffectType",
			static_cast<DWORD>(EffectHelper::EffectType::ModernAcrylicBlur)
		)
	};
	DWORD enableDropShadow
	{
		RegHelper::GetDword(
			L"DropDown",
			L"EnableDropShadow",
			0
		)
	};
	DWORD opacity
	{
		RegHelper::GetDword(
			L"DropDown",
			L"LightMode_Opacity",
			158
		)
	};
	DWORD gradientColor
	{
		RegHelper::GetDword(
			L"DropDown",
			L"LightMode_GradientColor",
			0xDDDDDD
		)
	};
	m_menuList.push_back(hWnd);
	SetWindowSubclass(hWnd, SubclassProc, dropDownSubclassId, 0);
	EffectHelper::SetWindowBackdrop(hWnd, enableDropShadow, (gradientColor | (opacity << 24)), effectType);
}

void TranslucentFlyouts::MenuHandler::DetachDropDown(HWND hWnd)
{
	RemoveWindowSubclass(hWnd, SubclassProc, dropDownSubclassId);
	m_menuList.remove(hWnd);
}

void TranslucentFlyouts::MenuHandler::AttachListViewPopup(HWND hWnd)
{
	DWORD itemDisabled
	{
		RegHelper::GetDword(
			L"DropDown",
			L"Disabled",
			0
		)
	};

	if (itemDisabled)
	{
		return;
	}

	HWND dropDown{GetAncestor(hWnd, GA_ROOT)};
	AttachDropDown(dropDown);
	Hooking::MsgHooks::GetInstance().Install(hWnd);
	m_hookedWindowList.push_back(hWnd);
}

void TranslucentFlyouts::MenuHandler::DetachListViewPopup(HWND hWnd)
{
	HWND dropDown{GetAncestor(hWnd, GA_ROOT)};
	DetachDropDown(dropDown);
	Hooking::MsgHooks::GetInstance().Uninstall(hWnd);
	m_hookedWindowList.remove(hWnd);
}

HDC TranslucentFlyouts::MenuHandler::GetCurrentMenuDC()
{
	return g_sharedDC.menuDC;
}

HDC TranslucentFlyouts::MenuHandler::GetCurrentListviewDC()
{
	return g_sharedDC.listviewDC;
}

void TranslucentFlyouts::MenuHandler::NotifyUxThemeRendering()
{
	g_sharedMenuInfo.useUxTheme = true;
}

void TranslucentFlyouts::MenuHandler::NotifyMenuDarkMode(bool darkMode)
{
	g_sharedMenuInfo.useDarkMode = darkMode;
}

bool TranslucentFlyouts::MenuHandler::IsImmersiveContextMenu(HWND hWnd)
{
	bool result{false};
	auto menu{reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0))};

	if (menu)
	{
		MENUINFO mi{sizeof(MENUINFO), MIM_BACKGROUND};
		if (GetMenuInfo(menu, &mi) && mi.hbrBack)
		{
			result = true;
		}
	}

	return result;
}

void TranslucentFlyouts::MenuHandler::StartupHook()
{
	Hooking::MsgHooks::GetInstance().AddCallback(MenuOwnerMsgCallback);
	Hooking::MsgHooks::GetInstance().AddCallback(ListviewpopupMsgCallback);
}

void TranslucentFlyouts::MenuHandler::ShutdownHook()
{
	Hooking::MsgHooks::GetInstance().DeleteCallback(MenuOwnerMsgCallback);
	Hooking::MsgHooks::GetInstance().DeleteCallback(ListviewpopupMsgCallback);
	// Remove all the hooks
	if (!m_hookedWindowList.empty())
	{
		auto hookedList{m_hookedWindowList};
		for (auto hookedWindow : hookedList)
		{
			Hooking::MsgHooks::GetInstance().Uninstall(hookedWindow);
		}
		m_hookedWindowList.clear();
	}
	// Remove subclass for all existing popup menu
	if (!m_menuList.empty())
	{
		auto menuList{m_menuList};
		for (auto menuWindow : menuList)
		{
			SendMessage(menuWindow, WM_MHDETACH, 0, 0);
		}
		m_menuList.clear();
	}
}
