#include "pch.h"
#include "TFMain.hpp"
#include "resource.h"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuAnimation.hpp"
#include "SymbolResolver.hpp"

using namespace std;
using namespace wil;
using namespace TranslucentFlyouts;

namespace TranslucentFlyouts::MenuHandler
{
	// describes the sizes of the menu bar or menu item
	union UAHMENUITEMMETRICS
	{
		struct
		{
			DWORD cx;
			DWORD cy;
		} rgsizeBar[2];
		struct
		{
			DWORD cx;
			DWORD cy;
		} rgsizePopup[4];
	};

	// not really used in our case but part of the other structures
	struct UAHMENUPOPUPMETRICS
	{
		DWORD rgcx[4];
		DWORD fUpdateMaxWidths : 2; // from kernel symbols, padded to full dword
	};

	// hmenu is the main window menu; hdc is the context to draw in
	struct UAHMENU
	{
		HMENU hMenu;
		HDC hdc;
		DWORD dwFlags; // no idea what these mean, in my testing it's either 0x00000a00 or sometimes 0x00000a10
	};

	// menu items are always referred to by iPosition here
	struct UAHMENUITEM
	{
		int iPosition; // 0-based position of menu item in menubar
		UAHMENUITEMMETRICS umim;
		UAHMENUPOPUPMETRICS umpm;
	};

	// the DRAWITEMSTRUCT contains the states of the menu items, as well as
	// the position index of the item in the menu, which is duplicated in
	// the UAHMENUITEM's iPosition as well
	struct UAHDRAWMENUITEM
	{
		DRAWITEMSTRUCT dis; // itemID looks uninitialized
		UAHMENU um;
		UAHMENUITEM umi;
	};

	// the MEASUREITEMSTRUCT is intended to be filled with the size of the item
	// height appears to be ignored, but width can be modified
	struct UAHMEASUREMENUITEM
	{
		MEASUREITEMSTRUCT mis;
		UAHMENU um;
		UAHMENUITEM umi;
	};

	constexpr UINT WM_UAHDESTROYWINDOW{ 0x0090 };
	constexpr UINT WM_UAHDRAWMENU{ 0x0091 };			// lParam is UAHMENU, return TRUE after handling it
	constexpr UINT WM_UAHDRAWMENUITEM{ 0x0092 };		// lParam is UAHDRAWMENUITEM, return TRUE after handling it
	constexpr UINT WM_UAHINITMENU{ 0x0093 };
	constexpr UINT WM_UAHMEASUREMENUITEM{ 0x0094 };	// lParam is UAHMEASUREMENUITEM, return TRUE after handling it
	constexpr UINT WM_UAHNCPAINTMENUPOPUP{ 0x0095 };	// lParam is UAHMENU, return TRUE after handling it

	constexpr UINT DFCS_MENUARROWUP{ 0x0008 };
	constexpr UINT DFCS_MENUARROWDOWN{ 0x0010 };

	constexpr int popupMenuArrowUp{ -3 };
	constexpr int popupMenuArrowDown{ -4 };

	struct MenuRenderingContext
	{
		HDC menuDC{ nullptr };
		HDC listviewDC{ nullptr };
	};

	constexpr int popupMenuSubclassId{ 0 };
	constexpr int popupMenuBrushManagerSubclassId{ 1 };
	constexpr int dropDownSubclassId{ 2 };

	thread_local MenuRenderingContext g_sharedContext{ nullptr, nullptr };
	thread_local MenuRenderingInfo g_sharedMenuInfo{ false, false };

	const UINT WM_MHATTACH{ RegisterWindowMessageW(L"TranslucentFlyouts.MenuHandler.Attach") };
	const UINT WM_MHDETACH{ RegisterWindowMessageW(L"TranslucentFlyouts.MenuHandler.Detach") };
	const wstring_view InitializationMarkerPropName{ L"TranslucentFlyouts.MenuHandler.InitializationMarker" };
	const wstring_view BorderMarkerPropName{ L"TranslucentFlyouts.MenuHandler.BorderMarker" };

	list<HWND> g_hookedWindowList{};
	list<HWND> g_menuList{};

	HRESULT WINAPI GetBackgroundColorForAppUserModelId(PCWSTR pszItem, COLORREF* color);

	void CalcAPIAddress();
	bool IsAPIOffsetReady();

#pragma data_seg(".shared")
	static DWORD64 g_GetBackgroundColorForAppUserModelId_Offset { 0 };
	static DWORD64 g_CreateStoreIcon_Offset{0};
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")
	PVOID g_actualGetBackgroundColorForAppUserModelId { nullptr };
	PVOID g_actualCreateStoreIcon{ nullptr };

	namespace
	{
		PVOID g_flagsDataAddress{nullptr};
		BYTE g_oldData[] {0x04, 0x00, 0x00, 0x20};
	}

	bool g_startup{ false };
	bool g_hooked{ false };

	void WinEventCallback(HWND hWnd, DWORD event);
	// In certain situations, using SetWindowSubclass can't receive WM_DRAWITEM (eg. Windows 11 Taskmgr),
	// so here we use hooks instead
	void MenuOwnerMsgCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid);
	void ListviewpopupMsgCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid);

	LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	bool IsImmersiveContextMenu(HWND hWnd);

	void AttachDropDown(HWND hWnd);
	void DetachDropDown(HWND hWnd);

	void AttachPopupMenuOwner(HWND hWnd);
	void DetachPopupMenuOwner(HWND hWnd);

	void AttachPopupMenu(HWND hWnd);
	void DetachPopupMenu(HWND hWnd);

	void AttachListViewPopup(HWND hWnd);
	void DetachListViewPopup(HWND hWnd);

	bool IsMenuPartlyOwnerDrawn(HMENU hMenu);
	MARGINS GetPopupMenuNonClientMargins(HWND hWnd);
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

				if (g_sharedMenuInfo.useUxTheme)
				{
					DWORD cornerType
					{
						RegHelper::GetDword(
							L"Menu",
							L"CornerType",
							3
						)
					};
					if (cornerType != 1 && SUCCEEDED(TFMain::ApplySysBorderColors(L"Menu"sv, menuWindow, g_sharedMenuInfo.useDarkMode, g_sharedMenuInfo.borderColor, g_sharedMenuInfo.borderColor)))
					{
						SetPropW(menuWindow, BorderMarkerPropName.data(), reinterpret_cast<HANDLE>(HANDLE_FLAG_INHERIT));
					}
					TFMain::ApplyRoundCorners(L"Menu"sv, menuWindow);
				}
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
		DWORD enableThemeColorization
		{
			RegHelper::GetDword(
				L"Menu",
				L"EnableThemeColorization",
				0
			)
		};

		if (enableThemeColorization)
		{
			LOG_IF_FAILED(Utils::GetDwmThemeColor(borderColor));
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
	else
	{
		LOG_LAST_ERROR_IF(!FrameRect(hdc, &paintRect, GetStockBrush(BLACK_BRUSH)));
	}

	return true;
}

MARGINS MenuHandler::GetPopupMenuNonClientMargins(HWND hWnd)
{
	RECT windowRect{};
	GetWindowRect(hWnd, &windowRect);

	MENUBARINFO mbi{sizeof(MENUBARINFO)};
	GetMenuBarInfo(hWnd, OBJID_CLIENT, 0, &mbi);

	return {mbi.rcBar.left - windowRect.left, windowRect.right - mbi.rcBar.right, mbi.rcBar.top - windowRect.top, windowRect.bottom - mbi.rcBar.bottom};
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
		THROW_IF_WIN32_BOOL_FALSE(PrintWindow(hWnd, memoryDC.get(), 0));
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

			DWORD cornerType
			{
				RegHelper::GetDword(
					L"Menu",
					L"CornerType",
					3
				)
			};
			if (cornerType != 1 && SUCCEEDED(TFMain::ApplySysBorderColors(L"Menu"sv, hWnd, g_sharedMenuInfo.useDarkMode, g_sharedMenuInfo.borderColor, g_sharedMenuInfo.borderColor)))
			{
				SetPropW(hWnd, BorderMarkerPropName.data(), reinterpret_cast<HANDLE>(HANDLE_FLAG_INHERIT));
			}
			TFMain::ApplyRoundCorners(L"Menu"sv, hWnd);
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
				THROW_HR_IF(
					E_INVALIDARG,
					((position & 0xFFFF'FFFF'FFFF'FFF0) == 0xFFFF'FFFF'FFFF'FFF0)
				);

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
						hWnd, mbi,
						chrono::milliseconds
				{
					RegHelper::GetDword(
						L"Menu\\Animation",
						L"FadeOutTime",
						static_cast<DWORD>(MenuAnimation::standardFadeoutDuration.count()),
						false
					)
				}
					)
				);

				handled = true;
				SystemParametersInfoW(SPI_SETSELECTIONFADE, 0, FALSE, 0);
				result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				SystemParametersInfoW(SPI_SETSELECTIONFADE, 0, reinterpret_cast<PVOID>(TRUE), 0);
			}
			CATCH_LOG()
		}

		if (uMsg == WM_MHATTACH)
		{
			handled = true;

			SetPropW(hWnd, InitializationMarkerPropName.data(), reinterpret_cast<HANDLE>(HANDLE_FLAG_INHERIT));
			AttachPopupMenuOwner(hWnd);

			// This menu is using unknown owner drawn technique,
			// in order to prevent broken visual content, we need to detach and remove menu backdrop
			auto info{GetMenuRenderingInfo(hWnd)};
			if (!info.useUxTheme || IsMenuPartlyOwnerDrawn(reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0))))
			{
				SendNotifyMessage(hWnd, WM_MHDETACH, 0, 0);
			}
			else
			{
				TFMain::ApplyBackdropEffect(L"Menu"sv, hWnd, info.useDarkMode, TFMain::darkMode_GradientColor, TFMain::lightMode_GradientColor);
				DWORD cornerType
				{
					RegHelper::GetDword(
						L"Menu",
						L"CornerType",
						3
					)
				};
				if (cornerType != 1 && SUCCEEDED(TFMain::ApplySysBorderColors(L"Menu"sv, hWnd, info.useDarkMode, info.borderColor, info.borderColor)))
				{
					SetPropW(hWnd, BorderMarkerPropName.data(), reinterpret_cast<HANDLE>(HANDLE_FLAG_INHERIT));
				}
				TFMain::ApplyRoundCorners(L"Menu"sv, hWnd);

				try
				{
					auto hMenu{reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0))};
					MENUINFO mi{sizeof(mi), MIM_BACKGROUND};
					THROW_IF_WIN32_BOOL_FALSE(GetMenuInfo(hMenu, &mi));
					THROW_HR_IF(E_FAIL, !SetWindowSubclass(hWnd, SubclassProc, popupMenuBrushManagerSubclassId, reinterpret_cast<DWORD_PTR>(mi.hbrBack)));
					mi.hbrBack = GetStockBrush(BLACK_BRUSH);
					// SetMenuInfo has a bug that it fails without setting last error.
					LOG_HR_IF(E_FAIL, !SetMenuInfo(hMenu, &mi));
				}
				CATCH_LOG()

				// We have menu scroll arrows, make it redraw itself.
				if (GetPopupMenuNonClientMargins(hWnd).cyTopHeight != nonClientMarginStandardSize)
				{
					PostMessageW(hWnd, MN_SELECTITEM, popupMenuArrowUp, 0);
					PostMessageW(hWnd, MN_SELECTITEM, popupMenuArrowDown, 0);
				}
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
				float ratio
				{
					static_cast<float>(
						RegHelper::GetDword(
							L"Menu\\Animation",
							L"StartRatio",
							lround(MenuAnimation::standardStartPosRatio * 100.f),
							false
						)
					) / 100.f
				};
				MenuAnimation::CreatePopupIn(
					hWnd,
					ratio,
					chrono::milliseconds
				{
					RegHelper::GetDword(
						L"Menu\\Animation",
						L"PopInTime",
						static_cast<DWORD>(MenuAnimation::standardPopupInDuration.count()),
						false
					)
				},
				chrono::milliseconds
				{
					RegHelper::GetDword(
						L"Menu\\Animation",
						L"FadeInTime",
						static_cast<DWORD>(MenuAnimation::standardFadeInDuration.count()),
						false
					)
				},
				RegHelper::GetDword(
					L"Menu\\Animation",
					L"PopInStyle",
					0,
					false
				)
				);
			}
		}

		// Some special popup menu will receive this message several times
		if (uMsg == MN_SIZEWINDOW)
		{
			handled = true;
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

			if (!GetPropW(hWnd, InitializationMarkerPropName.data()))
			{
				SendMessageW(hWnd, WM_MHATTACH, 0, 0);
			}

			InvalidateRect(hWnd, nullptr, TRUE);
			DwmTransitionOwnedWindow(hWnd, DWMTRANSITION_OWNEDWINDOW_REPOSITION);
		}

		if (uMsg == WM_WINDOWPOSCHANGED)
		{
			const auto& windowPos{*reinterpret_cast<WINDOWPOS*>(lParam)};
			if (windowPos.flags & SWP_SHOWWINDOW)
			{
				handled = true;
				result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

				DWORD noSystemDropShadow
				{
					RegHelper::GetDword(
						L"Menu",
						L"NoSystemDropShadow",
						0,
						false
					)
				};
				if (noSystemDropShadow)
				{
					HWND backdropWindow{GetWindow(hWnd, GW_HWNDNEXT)};
					if (Utils::IsWindowClass(backdropWindow, L"SysShadow"))
					{
						ShowWindow(backdropWindow, SW_HIDE);
					}
				}
			}
		}

		if (uMsg == WM_PRINT)
		{
			if (IsImmersiveContextMenu(hWnd))
			{
				handled = true;
				result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

				POINT pt{};
				Utils::unique_ext_hdc hdc{reinterpret_cast<HDC>(wParam)};

				MARGINS mr{GetPopupMenuNonClientMargins(hWnd)};

				RECT paintRect{};
				GetWindowRect(hWnd, &paintRect);
				OffsetRect(&paintRect, -paintRect.left, -paintRect.top);

				if (!GetPropW(hWnd, BorderMarkerPropName.data()))
				{
					HandlePopupMenuNCBorderColors(hdc.get(), g_sharedMenuInfo.useDarkMode, paintRect);
				}
				else
				{
					LOG_LAST_ERROR_IF(!FrameRect(hdc.get(), &paintRect, GetStockBrush(BLACK_BRUSH)));
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

				MARGINS mr{GetPopupMenuNonClientMargins(hWnd)};

				RECT paintRect{};
				GetWindowRect(hWnd, &paintRect);
				OffsetRect(&paintRect, -paintRect.left, -paintRect.top);

				{
					Utils::unique_ext_hdc dc{hdc.get()};
					ExcludeClipRect(dc.get(), paintRect.left + mr.cxLeftWidth, paintRect.top + mr.cyTopHeight, paintRect.right - mr.cxRightWidth, paintRect.bottom - mr.cyBottomHeight);
					FillRect(dc.get(), &paintRect, GetStockBrush(BLACK_BRUSH));
				}

				if (!GetPropW(hWnd, BorderMarkerPropName.data()) && !HandlePopupMenuNCBorderColors(hdc.get(), g_sharedMenuInfo.useDarkMode, paintRect))
				{
					RECT windowRect{};
					GetWindowRect(hWnd, &windowRect);

					unique_hrgn windowRegion{CreateRectRgnIndirect(&windowRect)};
					unique_hrgn windowRegionWithoutOutline{CreateRectRgn(windowRect.left + systemOutlineSize, windowRect.top + systemOutlineSize, windowRect.right - systemOutlineSize, windowRect.bottom - systemOutlineSize)};
					CombineRgn(windowRegion.get(), windowRegion.get(), windowRegionWithoutOutline.get(), RGN_XOR);

					DefSubclassProc(hWnd, WM_NCPAINT, reinterpret_cast<WPARAM>(windowRegion.get()), 0);
				}
			}
		}

		if (uMsg == WM_DESTROY || uMsg == WM_MHDETACH)
		{
			if (GetPropW(hWnd, InitializationMarkerPropName.data()))
			{
				RemovePropW(hWnd, InitializationMarkerPropName.data());
			}

			if (GetPropW(hWnd, BorderMarkerPropName.data()))
			{
				RemovePropW(hWnd, BorderMarkerPropName.data());
			}

			if (uMsg == WM_MHDETACH)
			{
				RemovePropW(hWnd, L"IsZachMenuDWMAttributeSet");
				EffectHelper::SetWindowBackdrop(hWnd, FALSE, 0, static_cast<DWORD>(EffectHelper::EffectType::None));
				InvalidateRect(hWnd, nullptr, TRUE);
			}

			DetachPopupMenuOwner(hWnd);
			DetachPopupMenu(hWnd);
		}
	}

	if (uIdSubclass == popupMenuBrushManagerSubclassId)
	{
		if (uMsg == WM_DESTROY || uMsg == WM_MHDETACH)
		{
			HBRUSH brush{nullptr};

			if (GetWindowSubclass(hWnd, SubclassProc, popupMenuBrushManagerSubclassId, reinterpret_cast<DWORD_PTR*>(&brush)))
			{
				MENUINFO mi{ .cbSize{sizeof(mi)}, .fMask{MIM_BACKGROUND}, .hbrBack{brush} };
				LOG_HR_IF(E_FAIL, !SetMenuInfo(reinterpret_cast<HMENU>(DefSubclassProc(hWnd, MN_GETHMENU, 0, 0)), &mi));
			}
		}
	}

	if (uIdSubclass == dropDownSubclassId)
	{
		if (uMsg == WM_MHATTACH)
		{
			handled = true;

			HWND listviewpopup{ FindWindowExW(hWnd, nullptr, L"ListViewPopup", nullptr) };

			auto info{ GetMenuRenderingInfo(hWnd) };
			if (info.useUxTheme)
			{
				TFMain::ApplyBackdropEffect(L"DropDown"sv, hWnd, info.useDarkMode, TFMain::darkMode_GradientColor, TFMain::lightMode_GradientColor);
				TFMain::ApplySysBorderColors(L"DropDown"sv, hWnd, info.useDarkMode, DWMWA_COLOR_NONE, DWMWA_COLOR_NONE);
				TFMain::ApplyRoundCorners(L"DropDown"sv, hWnd);

				HWND toolbar{ FindWindowExW(listviewpopup, nullptr, L"ToolbarWindow32", nullptr) };
				if (toolbar)
				{
					DetachListViewPopup(listviewpopup);
					SendMessageW(hWnd, WM_MHDETACH, 0, 0);

					InvalidateRect(toolbar, nullptr, TRUE);
				}
			}
			else
			{
				DetachListViewPopup(listviewpopup);
				SendMessageW(hWnd, WM_MHDETACH, 0, 0);
			}
		}

		if (uMsg == WM_DESTROY || uMsg == WM_MHDETACH)
		{
			DetachDropDown(hWnd);
			if (uMsg == WM_MHDETACH)
			{
				handled = true;
				EffectHelper::SetWindowBackdrop(hWnd, FALSE, 0, static_cast<DWORD>(EffectHelper::EffectType::None));
				InvalidateRect(hWnd, nullptr, TRUE);
			}
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

	g_menuList.push_back(hWnd);
	SetWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId, 0);
}

void MenuHandler::DetachPopupMenu(HWND hWnd)
{
	RemoveWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId);
	g_menuList.remove(hWnd);
}

void MenuHandler::AttachDropDown(HWND hWnd)
{
	g_menuList.push_back(hWnd);
	SetWindowSubclass(hWnd, SubclassProc, dropDownSubclassId, 0);

	PostMessageW(hWnd, WM_MHATTACH, 0, 0);
}

void MenuHandler::DetachDropDown(HWND hWnd)
{
	RemoveWindowSubclass(hWnd, SubclassProc, dropDownSubclassId);
	g_menuList.remove(hWnd);
}

void MenuHandler::AttachPopupMenuOwner(HWND hWnd)
{
	HWND menuOwner{Utils::GetCurrentMenuOwner()};

	if (menuOwner)
	{
		SetWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId, reinterpret_cast<DWORD_PTR>(menuOwner));
		Hooking::MsgHooks::GetInstance().Install(menuOwner);
		g_hookedWindowList.push_back(menuOwner);
	}
}

void MenuHandler::DetachPopupMenuOwner(HWND hWnd)
{
	HWND menuOwner{nullptr};

	if (GetWindowSubclass(hWnd, SubclassProc, popupMenuSubclassId, reinterpret_cast<DWORD_PTR*>(&menuOwner)) && menuOwner)
	{
		Hooking::MsgHooks::GetInstance().Uninstall(menuOwner);
		g_hookedWindowList.remove(menuOwner);
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
	g_hookedWindowList.push_back(hWnd);

	HWND dropDown{GetAncestor(hWnd, GA_ROOT)};
	AttachDropDown(dropDown);
}

void MenuHandler::DetachListViewPopup(HWND hWnd)
{
	Hooking::MsgHooks::GetInstance().Uninstall(hWnd);
	g_hookedWindowList.remove(hWnd);
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
	if (event == EVENT_OBJECT_CREATE)
	{
		if (Utils::IsWin32PopupMenu(hWnd))
		{
			AttachPopupMenu(hWnd);
		}

		HWND parentWindow{GetParent(hWnd)};
		if (Utils::IsWindowClass(hWnd, L"Listviewpopup") && Utils::IsWindowClass(parentWindow, L"DropDown"))
		{
			AttachListViewPopup(hWnd);
		}
	}
}

HRESULT WINAPI MenuHandler::GetBackgroundColorForAppUserModelId(PCWSTR pszItem, COLORREF* color)
{
	if (RegHelper::GetDword(L"Menu"sv, L"NoModernAppBackgroundColor", 1, false))
	{
		return E_NOTIMPL;
	}
	return (reinterpret_cast<decltype(GetBackgroundColorForAppUserModelId)*>(g_actualGetBackgroundColorForAppUserModelId))(
			   pszItem, color
		   );
}

void MenuHandler::Startup()
{
	if (g_startup)
	{
		return;
	}
	CalcAPIAddress();

	Hooking::MsgHooks::GetInstance().AddCallback(MenuOwnerMsgCallback);
	Hooking::MsgHooks::GetInstance().AddCallback(ListviewpopupMsgCallback);

	TFMain::AddCallback(WinEventCallback);

	g_startup = true;

	if (RegHelper::GetDword(L"Menu"sv, L"NoModernAppBackgroundColor", 1, false))
	{
		g_flagsDataAddress = Hooking::SearchData(g_actualCreateStoreIcon, g_oldData, sizeof(g_oldData));
		if (g_flagsDataAddress)
		{
			Hooking::WriteMemory(g_flagsDataAddress, [&]()
			{
				DWORD flags{ SIIGBF_RESIZETOFIT };
				memcpy_s(g_flagsDataAddress, sizeof(flags), &flags, sizeof(flags));
			});
		}
	}

	if (g_hooked)
	{
		return;
	}

	HRESULT hr
	{
		Hooking::Detours::Write([&]()
		{
			Hooking::Detours::Attach(&g_actualGetBackgroundColorForAppUserModelId, MenuHandler::GetBackgroundColorForAppUserModelId);
		})
	};

	if (SUCCEEDED(hr))
	{
		g_hooked = true;
	}
}

void MenuHandler::Shutdown()
{
	if (!g_startup)
	{
		return;
	}

	g_sharedContext.menuDC = nullptr;
	g_sharedContext.listviewDC = nullptr;
	g_sharedMenuInfo.Reset();

	TFMain::DeleteCallback(WinEventCallback);

	Hooking::MsgHooks::GetInstance().DeleteCallback(MenuOwnerMsgCallback);
	Hooking::MsgHooks::GetInstance().DeleteCallback(ListviewpopupMsgCallback);
	// Remove all the hooks
	if (!g_hookedWindowList.empty())
	{
		auto hookedList{g_hookedWindowList};
		for (auto hookedWindow : hookedList)
		{
			Hooking::MsgHooks::GetInstance().Uninstall(hookedWindow);
		}
		g_hookedWindowList.clear();
	}
	// Remove subclass for all existing popup menu
	if (!g_menuList.empty())
	{
		auto menuList{g_menuList};
		for (auto menuWindow : menuList)
		{
			SendMessage(menuWindow, WM_MHDETACH, 0, 0);
		}
		g_menuList.clear();
	}

	if (g_flagsDataAddress)
	{
		Hooking::WriteMemory(g_flagsDataAddress, [&]()
		{
			memcpy_s(g_flagsDataAddress, sizeof(g_oldData), g_oldData, sizeof(g_oldData));
		});
	}

	if (!g_hooked)
	{
		return;
	}

	HRESULT hr
	{
		Hooking::Detours::Write([&]()
		{
			Hooking::Detours::Detach(&g_actualGetBackgroundColorForAppUserModelId, MenuHandler::GetBackgroundColorForAppUserModelId);

		})
	};

	LOG_IF_FAILED(hr);

	g_startup = false;
	g_hooked = false;
}

bool MenuHandler::IsAPIOffsetReady()
{
	if (
		g_GetBackgroundColorForAppUserModelId_Offset &&
		g_CreateStoreIcon_Offset
	)
	{
		return true;
	}
	return false;
}

void MenuHandler::Prepare(const TFMain::InteractiveIO& io)
{
	using TFMain::InteractiveIO;
	// TO-DO
	// Cache the offset information into the registry so that we don't need to calculate them every time

	while (!IsAPIOffsetReady())
	{
		HRESULT hr{ S_OK };
		SymbolResolver symbolResolver{ L"MenuHandler",  io};
		hr = symbolResolver.Walk(L"shell32.dll"sv, "*!*", [](PSYMBOL_INFO symInfo, ULONG symbolSize) -> bool
		{
			auto functionName{ reinterpret_cast<const CHAR*>(symInfo->Name) };
			CHAR unDecoratedFunctionName[MAX_PATH + 1]{};
			UnDecorateSymbolName(
				functionName, unDecoratedFunctionName, MAX_PATH,
				UNDNAME_COMPLETE | UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_THROW_SIGNATURES
			);
			CHAR fullyUnDecoratedFunctionName[MAX_PATH + 1]{};
			UnDecorateSymbolName(
				functionName, fullyUnDecoratedFunctionName, MAX_PATH,
				UNDNAME_NAME_ONLY
			);
			auto functionOffset{ symInfo->Address - symInfo->ModBase };

			if (!strcmp(fullyUnDecoratedFunctionName, "GetBackgroundColorForAppUserModelId"))
			{
				g_GetBackgroundColorForAppUserModelId_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CreateStoreIcon"))
			{
				g_CreateStoreIcon_Offset = functionOffset;
			}

			if (IsAPIOffsetReady())
			{
				return false;
			}

			return true;
		});
		if (FAILED(hr))
		{
			io.OutputString(
				InteractiveIO::StringType::Error,
				InteractiveIO::WaitType::NoWait,
				IDS_STRING103,
				std::format(L"[MenuHandler] "),
				std::format(L"(hresult: {:#08x})\n", static_cast<ULONG>(hr))
			);
		}

		if (IsAPIOffsetReady())
		{
			if (symbolResolver.GetSymbolStatus() && symbolResolver.GetSymbolSource())
			{
				io.OutputString(
					InteractiveIO::StringType::Notification,
					InteractiveIO::WaitType::NoWait,
					IDS_STRING101,
					std::format(L"[MenuHandler] "),
					L"\n"sv
				);
			}
		}
		else if (GetConsoleWindow())
		{
			if (symbolResolver.GetSymbolStatus())
			{
				io.OutputString(
					InteractiveIO::StringType::Error,
					InteractiveIO::WaitType::NoWait,
					IDS_STRING107,
					std::format(L"[MenuHandler] "),
					L"\n"sv
				);
			}
			else
			{
				if (
					io.OutputString(
						InteractiveIO::StringType::Warning,
						InteractiveIO::WaitType::WaitYN,
						IDS_STRING102,
						std::format(L"[MenuHandler] "),
						L"\n"sv
					)
				)
				{
					continue;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			io.OutputString(
				InteractiveIO::StringType::Error,
				InteractiveIO::WaitType::NoWait,
				IDS_STRING107,
				std::format(L"[MenuHandler] "),
				L"\n"sv
			);
			break;
		}
	}

	io.OutputString(
		TFMain::InteractiveIO::StringType::Notification,
		TFMain::InteractiveIO::WaitType::NoWait,
		0,
		std::format(L"[MenuHandler] "),
		std::format(L"Done. \n"),
		true
	);
}

void MenuHandler::CalcAPIAddress() try
{
	PVOID shell32Base{ reinterpret_cast<PVOID>(GetModuleHandleW(L"shell32.dll")) };
	THROW_LAST_ERROR_IF_NULL(shell32Base);

	if (g_GetBackgroundColorForAppUserModelId_Offset)
		g_actualGetBackgroundColorForAppUserModelId = reinterpret_cast<decltype(g_actualGetBackgroundColorForAppUserModelId)>(reinterpret_cast<DWORD64>(shell32Base) + g_GetBackgroundColorForAppUserModelId_Offset);
	if (g_CreateStoreIcon_Offset)
		g_actualCreateStoreIcon = reinterpret_cast<decltype(g_actualCreateStoreIcon)>(reinterpret_cast<DWORD64>(shell32Base) + g_CreateStoreIcon_Offset);
}
CATCH_LOG_RETURN()