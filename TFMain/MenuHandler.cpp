#include "pch.h"
#include "TFMain.hpp"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuAnimation.hpp"

using namespace std;
using namespace wil;
using namespace TranslucentFlyouts;

thread_local decltype(MenuHandler::g_sharedContext) MenuHandler::g_sharedContext{nullptr, nullptr};
thread_local decltype(MenuHandler::g_sharedMenuInfo) MenuHandler::g_sharedMenuInfo{false, false};
const UINT MenuHandler::WM_MHDETACH{RegisterWindowMessageW(L"TranslucentFlyouts.MenuHandler.Detach")};

MenuHandler& MenuHandler::GetInstance()
{
	static MenuHandler instance{};
	return instance;
}

MenuHandler::MenuHandler()
{

}

MenuHandler::~MenuHandler() noexcept
{
	ShutdownHook();
}

void MenuHandler::MenuOwnerMsgCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid)
{
	if (message == WM_DRAWITEM)
	{
		auto& drawItemStruct{*reinterpret_cast<LPDRAWITEMSTRUCT>(lParam)};

		if (!returnResultValid)
		{
			if (wParam == 0 && drawItemStruct.CtlType == ODT_MENU)
			{
				g_sharedContext.menuDC = drawItemStruct.hDC;
				g_sharedMenuInfo.Reset();
			}
		}

		if (returnResultValid)
		{
			if (wParam == 0 && drawItemStruct.CtlType == ODT_MENU)
			{
				g_sharedContext.menuDC = nullptr;

				HWND menuWindow{WindowFromDC(drawItemStruct.hDC)};

				GetInstance().HandleSysBorderColors(L"Menu"sv, menuWindow, g_sharedMenuInfo.useDarkMode, g_sharedMenuInfo.borderColor);
				GetInstance().HandleRoundCorners(L"Menu"sv, menuWindow);
			}
		}
	}
}

void MenuHandler::ListviewpopupMsgCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid)
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
		g_sharedContext.listviewDC = nullptr;
		return;
	}

	if (message == WM_ERASEBKGND && returnResultValid)
	{
		if (Utils::IsWindowClass(hwnd, L"Listviewpopup"))
		{
			HDC hdc{reinterpret_cast<HDC>(wParam)};
			RECT clipRect{};
			GetClipBox(hdc, &clipRect);
			PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS);
		}
	}

	if (message == WM_DRAWITEM)
	{
		auto& drawItemStruct{*reinterpret_cast<LPDRAWITEMSTRUCT>(lParam)};

		if (!returnResultValid)
		{
			if (drawItemStruct.CtlType == ODT_LISTVIEW)
			{
				g_sharedContext.listviewDC = drawItemStruct.hDC;

				PatBlt(drawItemStruct.hDC, drawItemStruct.rcItem.left, drawItemStruct.rcItem.top, drawItemStruct.rcItem.right - drawItemStruct.rcItem.left, drawItemStruct.rcItem.bottom - drawItemStruct.rcItem.top, BLACKNESS);
			}
		}

		if (returnResultValid)
		{
			if (drawItemStruct.CtlType == ODT_LISTVIEW)
			{
				g_sharedContext.listviewDC = nullptr;
			}
		}
	}
}

HRESULT MenuHandler::HandleSysBorderColors(std::wstring_view keyName, HWND hWnd, bool useDarkMode, COLORREF color)
{
	DWORD noBorderColor
	{
		RegHelper::GetDword(
			keyName,
			L"NoBorderColor",
			0
		)
	};

	DWORD borderColor{color};
	if (!noBorderColor)
	{
		try
		{
			DWORD enableThemeColorization
			{
				RegHelper::GetDword(
					keyName,
					L"EnableThemeColorization",
					0
				)
			};

			THROW_HR_IF(E_NOTIMPL, !enableThemeColorization);
			THROW_IF_FAILED(Utils::GetDwmThemeColor(borderColor));
		}
		catch (...)
		{
			if (ResultFromCaughtException() != E_NOTIMPL)
			{
				LOG_CAUGHT_EXCEPTION();
			}
		}

		if (useDarkMode)
		{
			borderColor = RegHelper::GetDword(
					keyName,
					L"DarkMode_BorderColor",
					borderColor
			);
		}
		else
		{
			borderColor = RegHelper::GetDword(
					keyName,
					L"LightMode_BorderColor",
					borderColor
			);
		}

		borderColor = Utils::MakeCOLORREF(borderColor);
	}
	else
	{
		borderColor = DWMWA_COLOR_NONE;
	}

	return DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

bool MenuHandler::HandlePopupMenuNCBorderColors(HDC hdc, bool useDarkMode, const RECT& paintRect)
{
	DWORD noBorderColor
	{
		RegHelper::GetDword(
			L"Menu",
			L"NoBorderColor",
			0
		)
	};

	DWORD borderColor{DWMWA_COLOR_NONE};

	// Border color is enabled.
	if (!noBorderColor)
	{
		try
		{
			DWORD enableThemeColorization
			{
				RegHelper::GetDword(
					L"Menu",
					L"EnableThemeColorization",
					0
				)
			};

			THROW_HR_IF(E_NOTIMPL, !enableThemeColorization);
			THROW_IF_FAILED(Utils::GetDwmThemeColor(borderColor));
		}
		catch (...)
		{
			if (ResultFromCaughtException() != E_NOTIMPL)
			{
				LOG_CAUGHT_EXCEPTION();
			}
		}

		if (useDarkMode)
		{
			borderColor = RegHelper::GetDword(
							  L"Menu",
							  L"DarkMode_BorderColor",
							  borderColor
						  );
		}
		else
		{
			borderColor = RegHelper::GetDword(
							  L"Menu",
							  L"LightMode_BorderColor",
							  borderColor
						  );
		}

		if (borderColor == DWMWA_COLOR_NONE)
		{
			return false;
		}

		unique_hbrush brush{Utils::CreateSolidColorBrushWithAlpha(Utils::MakeCOLORREF(borderColor), Utils::GetAlpha(borderColor))};
		LOG_LAST_ERROR_IF_NULL(brush);
		if (brush)
		{
			LOG_LAST_ERROR_IF(!FrameRect(hdc, &paintRect, brush.get()));
		}
	}

	return true;
}

HRESULT MenuHandler::HandleRoundCorners(std::wstring_view keyName, HWND hWnd)
{
	DWORD cornerType
	{
		RegHelper::GetDword(
			keyName,
			L"CornerType",
			3
		)
	};
	if (cornerType != 0)
	{
		return DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerType, sizeof(DWM_WINDOW_CORNER_PREFERENCE));
	}
	return S_OK;
}

void MenuHandler::ApplyEffect(std::wstring_view keyName, HWND hWnd, bool darkMode)
{
	DWORD effectType
	{
		RegHelper::GetDword(
			keyName,
			L"EffectType",
			static_cast<DWORD>(EffectHelper::EffectType::ModernAcrylicBlur)
		)
	};
	DWORD enableDropShadow
	{
		RegHelper::GetDword(
			keyName,
			L"EnableDropShadow",
			0
		)
	};
	// Set effect for the popup menu
	DWORD gradientColor{0};
	if (darkMode)
	{
		gradientColor = RegHelper::GetDword(
							keyName,
							L"DarkMode_GradientColor",
							darkMode_GradientColor
						);

		EffectHelper::EnableWindowDarkMode(hWnd, TRUE);
	}
	else
	{
		gradientColor = RegHelper::GetDword(
							keyName,
							L"LightMode_GradientColor",
							lightMode_GradientColor
						);

	}
	EffectHelper::SetWindowBackdrop(hWnd, enableDropShadow, gradientColor, effectType);
	DwmTransitionOwnedWindow(hWnd, DWMTRANSITION_OWNEDWINDOW_REPOSITION);
}

bool MenuHandler::IsMenuPartlyOwnerDrawn(HMENU hMenu)
{
	auto ownerDrawnMenuItem{0};
	auto totMenuItem{GetMenuItemCount(hMenu)};
	for (auto i = 0; i < totMenuItem; i++)
	{
		MENUITEMINFOW mii{sizeof(mii), MIIM_FTYPE};
		if (GetMenuItemInfoW(hMenu, i, TRUE, &mii))
		{
			ownerDrawnMenuItem += (mii.fType & MFT_OWNERDRAW ? 1 : 0);
		}
	}

	return (totMenuItem != ownerDrawnMenuItem && ownerDrawnMenuItem != 0);
}

MenuHandler::MenuRenderingInfo MenuHandler::GetMenuRenderingInfo(HWND hWnd) try
{
	auto info{g_sharedMenuInfo};
	g_sharedMenuInfo.Reset();

	unique_hdc memoryDC{CreateCompatibleDC(nullptr)};
	THROW_LAST_ERROR_IF_NULL(memoryDC);
	unique_hbitmap bitmap{CreateCompatibleBitmap(memoryDC.get(), 1, 1)};
	THROW_LAST_ERROR_IF_NULL(bitmap);

	{
		auto selectedObject{wil::SelectObject(memoryDC.get(), bitmap.get())};
		SendMessageW(hWnd, WM_PRINT, reinterpret_cast<WPARAM>(memoryDC.get()), PRF_CHILDREN | PRF_NONCLIENT);
	}

	swap(g_sharedMenuInfo, info);
	return info;
}
catch (...)
{
	return{};
}

LRESULT CALLBACK MenuHandler::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	bool handled{false};
	LRESULT result{0};
	MenuHandler& menuHandler{GetInstance()};

	// Popup menu
	if (uIdSubclass == popupMenuSubclassId)
	{
		// If we can handle WM_UAHDRAWMENU, WM_UAHDRAWMENUITEM, WM_UAHNCPAINTMENUPOPUP properly, (like StartAllBack)
		// then we don't need UxThemePatcher anymore!!!
		// However, this method may break the vanilla behaviour of popup menu
		
		// If this menu is a immersive context menu, then it won't receive following 4 messages...
		if (uMsg == WM_UAHDRAWMENU)
		{
			handled = true;

			auto& uah{*reinterpret_cast<UAHMENU*>(lParam)};
			g_sharedContext.menuDC = uah.hdc;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			g_sharedContext.menuDC = nullptr;
		}

		if (uMsg == WM_UAHDRAWMENUITEM)
		{
			handled = true;

			auto& uah{*reinterpret_cast<UAHDRAWMENUITEM*>(lParam)};
			g_sharedContext.menuDC = uah.dis.hDC;
			// This is a patch which in order to remove the ugly white line at the end of the popup menu...
			if (uah.dis.CtlID == GetMenuItemID(uah.um.hMenu, GetMenuItemCount(uah.um.hMenu) - 1))
			{
				PatBlt(uah.dis.hDC, uah.dis.rcItem.left, uah.dis.rcItem.top, uah.dis.rcItem.right - uah.dis.rcItem.left, uah.dis.rcItem.bottom - uah.dis.rcItem.top, BLACKNESS);
			}
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			g_sharedContext.menuDC = nullptr;
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
			g_sharedContext.menuDC = uah.hdc;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			g_sharedContext.menuDC = nullptr;

			GetInstance().HandleSysBorderColors(L"Menu"sv, hWnd, g_sharedMenuInfo.useDarkMode, g_sharedMenuInfo.borderColor);
			GetInstance().HandleRoundCorners(L"Menu"sv, hWnd);
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

			menuHandler.AttachPopupMenuOwner(hWnd);

			// This menu is using unknown owner drawn technique,
			// in order to prevent broken visual content, we need to detach and remove menu backdrop
			auto info{menuHandler.GetMenuRenderingInfo(hWnd)};
			if (!info.useUxTheme || menuHandler.IsMenuPartlyOwnerDrawn(reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0))))
			{
				SendNotifyMessage(hWnd, WM_MHDETACH, 0, 0);
			}
			else
			{
				menuHandler.ApplyEffect(L"Menu"sv, hWnd, info.useDarkMode);
				GetInstance().HandleSysBorderColors(L"Menu"sv, hWnd, info.useDarkMode, info.borderColor);
				GetInstance().HandleRoundCorners(L"Menu"sv, hWnd);
			}

			DWORD enableFluentAnimation
			{
				RegHelper::GetDword(
					L"Menu",
					L"EnableFluentAnimation",
					0,
					false
				)
			};
			if (enableFluentAnimation)
			{
				MenuAnimation::CreatePopupIn(
					hWnd, MenuAnimation::standardStartPosRatio, MenuAnimation::standardPopupInDuration, MenuAnimation::standardFadeInDuration
				);
			}
		}

		if (uMsg == WM_PRINT)
		{
			if (IsImmersiveContextMenu(hWnd))
			{
				handled = true;

				POINT pt{};
				Utils::unique_ext_hdc hdc{reinterpret_cast<HDC>(wParam)};

				RECT windowRect{};
				GetWindowRect(hWnd, &windowRect);
				OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

				{
					Utils::unique_ext_hdc dc{hdc.get()};
					ExcludeClipRect(dc.get(), windowRect.left + nonClientMarginSize, windowRect.top + nonClientMarginSize, windowRect.right - nonClientMarginSize, windowRect.bottom - nonClientMarginSize);
					FillRect(dc.get(), &windowRect, GetStockBrush(BLACK_BRUSH));
				}

				SetViewportOrgEx(hdc.get(), nonClientMarginSize, nonClientMarginSize, &pt);
				result = DefSubclassProc(hWnd, WM_PRINTCLIENT, wParam, lParam);
				SetViewportOrgEx(hdc.get(), pt.x, pt.y, nullptr);

				if (!menuHandler.HandlePopupMenuNCBorderColors(hdc.get(), g_sharedMenuInfo.useDarkMode, windowRect))
				{
					Utils::unique_ext_hdc dc{hdc.get()};
					ExcludeClipRect(dc.get(), windowRect.left + systemOutlineSize, windowRect.top + systemOutlineSize, windowRect.right - systemOutlineSize, windowRect.bottom - systemOutlineSize);
					result = DefSubclassProc(hWnd, WM_PRINT, wParam, lParam);
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

				RECT windowRect{};
				GetWindowRect(hWnd, &windowRect);
				OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

				{
					Utils::unique_ext_hdc dc{hdc.get()};
					ExcludeClipRect(dc.get(), windowRect.left + nonClientMarginSize, windowRect.top + nonClientMarginSize, windowRect.right - nonClientMarginSize, windowRect.bottom - nonClientMarginSize);
					FillRect(dc.get(), &windowRect, GetStockBrush(BLACK_BRUSH));
				}

				if (!menuHandler.HandlePopupMenuNCBorderColors(hdc.get(), g_sharedMenuInfo.useDarkMode, windowRect))
				{
					Utils::unique_ext_hdc dc{hdc.get()};
					ExcludeClipRect(dc.get(), windowRect.left + systemOutlineSize, windowRect.top + systemOutlineSize, windowRect.right - systemOutlineSize, windowRect.bottom - systemOutlineSize);
					DefSubclassProc(hWnd, WM_PRINT, reinterpret_cast<WPARAM>(dc.get()), lParam);
				}
			}
		}

		if (uMsg == WM_ERASEBKGND)
		{
			handled = true;

			HDC hdc{reinterpret_cast<HDC>(wParam)};

			g_sharedContext.menuDC = hdc;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			g_sharedContext.menuDC = nullptr;

			if (IsImmersiveContextMenu(hWnd))
			{
				RECT clipRect{};
				GetClipBox(hdc, &clipRect);
				PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS);
			}
		}

		if (uMsg == WM_NCDESTROY || uMsg == WM_MHDETACH)
		{
			if (uMsg == WM_MHDETACH)
			{
				RemovePropW(hWnd, L"IsZachMenuDWMAttributeSet");
				EffectHelper::SetWindowBackdrop(hWnd, FALSE, 0, static_cast<DWORD>(EffectHelper::EffectType::None));
				InvalidateRect(hWnd, nullptr, TRUE);
			}

			menuHandler.DetachPopupMenuOwner(hWnd);
			menuHandler.DetachPopupMenu(hWnd);
		}
	}

	if (uIdSubclass == dropDownSubclassId)
	{
		if (uMsg == WM_NCDESTROY || uMsg == WM_MHDETACH)
		{
			if (uMsg == WM_MHDETACH)
			{
				EffectHelper::SetWindowBackdrop(hWnd, FALSE, 0, static_cast<DWORD>(EffectHelper::EffectType::None));
				InvalidateRect(hWnd, nullptr, TRUE);
			}
			menuHandler.DetachDropDown(hWnd);
		}
	}

	if (!handled)
	{
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	return result;
}

void MenuHandler::AttachPopupMenu(HWND hWnd)
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

void MenuHandler::DetachPopupMenu(HWND hWnd)
{
	RemoveWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId);
	m_menuList.remove(hWnd);
}

void MenuHandler::AttachDropDown(HWND hWnd)
{
	auto info{GetMenuRenderingInfo(hWnd)};
	ApplyEffect(L"DropDown"sv, hWnd, info.useDarkMode);
	GetInstance().HandleSysBorderColors(L"DropDown"sv, hWnd, info.useDarkMode, DWMWA_COLOR_NONE);
	GetInstance().HandleRoundCorners(L"DropDown"sv, hWnd);

	m_menuList.push_back(hWnd);
	SetWindowSubclass(hWnd, SubclassProc, dropDownSubclassId, 0);
}

void MenuHandler::DetachDropDown(HWND hWnd)
{
	RemoveWindowSubclass(hWnd, SubclassProc, dropDownSubclassId);
	m_menuList.remove(hWnd);
}

void MenuHandler::AttachPopupMenuOwner(HWND hWnd)
{
	HWND menuOwner{Utils::GetCurrentMenuOwner()};

	if (menuOwner)
	{
		SetWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId, reinterpret_cast<DWORD_PTR>(menuOwner));
		Hooking::MsgHooks::GetInstance().Install(menuOwner);
		m_hookedWindowList.push_back(menuOwner);
	}
}

void MenuHandler::DetachPopupMenuOwner(HWND hWnd)
{
	HWND menuOwner{nullptr};

	if (GetWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId, reinterpret_cast<DWORD_PTR*>(&menuOwner)) && menuOwner)
	{
		Hooking::MsgHooks::GetInstance().Uninstall(menuOwner);
		m_hookedWindowList.remove(menuOwner);
	}
}

void MenuHandler::AttachListViewPopup(HWND hWnd)
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

	Hooking::MsgHooks::GetInstance().Install(hWnd);
	m_hookedWindowList.push_back(hWnd);

	HWND dropDown{GetAncestor(hWnd, GA_ROOT)};
	AttachDropDown(dropDown);
}

void MenuHandler::DetachListViewPopup(HWND hWnd)
{
	HWND dropDown{GetAncestor(hWnd, GA_ROOT)};
	DetachDropDown(dropDown);

	Hooking::MsgHooks::GetInstance().Uninstall(hWnd);
	m_hookedWindowList.remove(hWnd);
}

HDC MenuHandler::GetCurrentMenuDC()
{
	return g_sharedContext.menuDC;
}

HDC MenuHandler::GetCurrentListviewDC()
{
	return g_sharedContext.listviewDC;
}

void MenuHandler::NotifyUxThemeRendering()
{
	g_sharedMenuInfo.useUxTheme = true;
}

void MenuHandler::NotifyMenuDarkMode(bool darkMode)
{
	g_sharedMenuInfo.useDarkMode = darkMode;
}

void MenuHandler::NotifyMenuStyle(bool immersive)
{
	g_sharedMenuInfo.immersive = immersive;
}

void MenuHandler::NotifyMenuBorderColor(COLORREF color)
{
	g_sharedMenuInfo.borderColor = color;
}

bool MenuHandler::IsImmersiveContextMenu(HWND hWnd)
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

void MenuHandler::WinEventCallback(HWND hWnd, DWORD event)
{
	auto& menuHandler{GetInstance()};

	if (event == EVENT_OBJECT_CREATE)
	{
		if (Utils::IsWin32PopupMenu(hWnd))
		{
			menuHandler.AttachPopupMenu(hWnd);
		}

		HWND parentWindow{GetParent(hWnd)};
		if (Utils::IsWindowClass(hWnd, L"Listviewpopup") && Utils::IsWindowClass(parentWindow, L"DropDown"))
		{
			menuHandler.AttachListViewPopup(hWnd);
		}
	}
}

void MenuHandler::StartupHook()
{
	Hooking::MsgHooks::GetInstance().AddCallback(MenuOwnerMsgCallback);
	Hooking::MsgHooks::GetInstance().AddCallback(ListviewpopupMsgCallback);

	MainDLL::GetInstance().AddCallback(WinEventCallback);
}

void MenuHandler::ShutdownHook()
{
	MainDLL::GetInstance().DeleteCallback(WinEventCallback);

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
