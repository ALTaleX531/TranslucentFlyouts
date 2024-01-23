#include "pch.h"
#include "resource.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "HookHelper.hpp"
#include "SymbolResolver.hpp"
#include "SystemHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuRendering.hpp"
#include "MenuAppearance.hpp"
#include "UxThemeHooks.hpp"
#include "HookDispatcher.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::UxThemeHooks
{
	using namespace std::literals;
	HRESULT WINAPI MyDrawThemeBackground(
		HTHEME  hTheme,
		HDC     hdc,
		int     iPartId,
		int     iStateId,
		LPCRECT pRect,
		LPCRECT pClipRect
	);
	HRESULT WINAPI MyDrawThemeText(
		HTHEME hTheme,
		HDC hdc,
		int iPartId,
		int iStateId,
		LPCWSTR pszText,
		int cchText,
		DWORD dwTextFlags,
		DWORD,
		LPCRECT pRect
	);
	HRESULT WINAPI MyDwmSetWindowAttribute(
		HWND    hwnd,
		DWORD   dwAttribute,
		LPCVOID pvAttribute,
		DWORD   cbAttribute
	);
	BOOL WINAPI MyStretchBlt(
		HDC   hdcDest,
		int   xDest,
		int   yDest,
		int   wDest,
		int   hDest,
		HDC   hdcSrc,
		int   xSrc,
		int   ySrc,
		int   wSrc,
		int   hSrc,
		DWORD rop
	);
	struct CThemeMenu
	{
		void __thiscall MyDrawItemBitmap(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, int iStateId, const RECT* lprc);
		void __thiscall MyDrawItemBitmap2(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, bool noStretch, int iStateId, const RECT* lprc);	// Windows 11
	};

#pragma data_seg(".shared")
	bool g_useCompatibleMode{ false };
	HookHelper::OffsetStorage g_CThemeMenu_DrawItemBitmap_Offset{ 0 };

	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_1{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_2{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_1{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_2{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_3{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_4{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_1{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_2{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawClientArea_DrawThemeBackground_E8Offset_Offset_1{ 0 };
	HookHelper::OffsetStorage g_CThemeMenuPopup_DrawNonClientArea_DrawThemeBackground_E8Offset_Offset_1{ 0 };
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")
	
	class TrampolinePool
	{
	public:
		UCHAR* Allocate(PVOID detourFunction)
		{
			if (!m_buffer)
			{
				return nullptr;
			}
#ifdef _WIN64
			UCHAR jmpCode[14]{ 0xFF, 0x25 };
			memcpy_s(&jmpCode[6], sizeof(PVOID), &detourFunction, sizeof(detourFunction));
#else
			UCHAR jmpCode[]{ 0xB8, 0, 0, 0, 0, 0xFF, 0xE0, 0 };
			memcpy_s(&jmpCode[1], sizeof(PVOID), &detourFunction, sizeof(detourFunction));
#endif
			if (m_current + sizeof(jmpCode) > m_end)
			{
				return nullptr;
			}

			auto startPos{ m_current };
			memcpy(startPos, jmpCode, sizeof(jmpCode));
			m_current += sizeof(jmpCode);

			return startPos;
		}
		TrampolinePool(PVOID startAddress)
		{
			m_buffer.reset(
				static_cast<UCHAR*>(DetourAllocateRegionWithinJumpBounds(startAddress, &m_bufferSize))
			);

			m_current = m_buffer.get();
			m_end = m_current + m_bufferSize;
		}
		~TrampolinePool() = default;
	private:
		DWORD m_bufferSize{0};
		wil::unique_virtualalloc_ptr<UCHAR> m_buffer{ nullptr };
		UCHAR* m_current{ nullptr }, * m_end{ nullptr };
	};
	LONG* SearchE8Call(PVOID startAddress, PVOID targetFunction)
	{
		auto functionIsEnd = [](UCHAR* bytes, int offset)
		{
#ifdef _WIN64
			if (
				bytes[offset] == 0xC3 &&
				bytes[offset + 1] == 0xCC &&
				bytes[offset + 2] == 0xCC &&
				bytes[offset + 3] == 0xCC &&
				bytes[offset + 4] == 0xCC &&
				bytes[offset + 5] == 0xCC &&
				bytes[offset + 6] == 0xCC &&
				bytes[offset + 7] == 0xCC &&
				bytes[offset + 8] == 0xCC
			)
#else
			if (
				bytes[offset] == 0xC2 &&
				bytes[offset + 2] == 0x00
			)
#endif
			{
				return true;
			}
			return false;
		};

		auto functionBytes{reinterpret_cast<UCHAR*>(startAddress)};
		for (int i{0}; !functionIsEnd(functionBytes, i) && i < 65535; i++)
		{
			if (functionBytes[i] == 0xE8)
			{
				auto offsetAddress{ reinterpret_cast<LONG*>(&functionBytes[i + 1]) };
				auto callBaseAddress{ &functionBytes[i + 5] };
				auto targetAddress{ reinterpret_cast<PVOID>(callBaseAddress + *offsetAddress) };

				if (targetAddress == targetFunction)
				{
					return offsetAddress;
				}
			}
		}

		return nullptr;
	}
	LONG ReplaceE8Call(LONG* callOffset, PVOID trampolineAddress)
	{
		auto callBaseAddress{ reinterpret_cast<UCHAR*>(reinterpret_cast<DWORD64>(callOffset) + sizeof(LONG)) };
		auto originalOffset{ *callOffset };
		HookHelper::WriteMemory(callOffset, [&]
		{
			*callOffset = static_cast<LONG>(reinterpret_cast<UCHAR*>(trampolineAddress) - callBaseAddress);
		});
		return originalOffset;
	}

	wil::srwlock g_lock{};

	size_t g_hookRef{ 0 };
	bool g_compatibleMode{ false };
	HookHelper::HookDispatcherDependency g_hookDependency
	{
		std::tuple
		{
			std::array
			{
				"gdi32.dll"sv,
				"ext-ms-win-gdi-desktop-l1-1-0.dll"sv
			},
			"StretchBlt",
			reinterpret_cast<PVOID>(MyStretchBlt)
		},
		std::tuple
		{
			std::array
			{
				"dwmapi.dll"sv,
				""sv
			},
			"DwmSetWindowAttribute",
			reinterpret_cast<PVOID>(MyDwmSetWindowAttribute)
		}
	};
	HookHelper::HookDispatcher g_hookDispatcher
	{
		g_hookDependency
	};

	std::unique_ptr<TrampolinePool> g_trampolinePool{nullptr};
	std::unordered_map<LONG*, LONG> g_hookTable{};
	UCHAR* g_myDrawThemeText_Trampoline{ nullptr };
	UCHAR* g_myDrawThemeBackground_Trampoline{ nullptr };

	PVOID g_actualCThemeMenu_DrawItemBitmap{ nullptr };
	PVOID g_detourCThemeMenu_DrawItemBitmap{ nullptr };

	decltype(&MyDrawThemeBackground) g_actualDrawThemeBackground{ nullptr };
	decltype(&MyDrawThemeText) g_actualDrawThemeText{ nullptr };

	void DisableHooksInternal();
}

HRESULT WINAPI UxThemeHooks::MyDrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	HRESULT hr{S_OK};

	auto callerModule{ DetourGetContainingModule(_ReturnAddress()) };
	auto handler = [&]() -> bool
	{
		if (!MenuHandler::g_uxhooksEnabled)
		{
			return false;
		}

		WCHAR themeClassName[MAX_PATH + 1]{};
		if (FAILED(ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)))
		{
			return false;
		}

		if (_wcsicmp(themeClassName, L"Menu"))
		{
			return false;
		}

		if (IsRectEmpty(pRect))
		{
			return false;
		}

		if (callerModule != GetModuleHandleW(L"uxtheme.dll"))
		{
			return false;
		}

		COLORREF color{ DWMWA_COLOR_NONE }, expectedColor{ 0 };
		if (
			SUCCEEDED(GetThemeColor(hTheme, MENU_POPUPITEM, 0, TMT_TEXTCOLOR, &color)) &&
			SUCCEEDED(GetThemeColor(wil::unique_htheme{OpenThemeData(nullptr, L"DarkMode::Menu")}.get(), MENU_POPUPITEM, 0, TMT_TEXTCOLOR, &expectedColor)) &&
			color == expectedColor
		)
		{
			MenuHandler::g_menuContext.useDarkMode = true;
		}
		else
		{
			MenuHandler::g_menuContext.useDarkMode = false;
		}
		MenuHandler::g_menuContext.useUxTheme = true;

		RECT clipRect{ *pRect };
		if (pClipRect)
		{
			IntersectRect(&clipRect, &clipRect, pClipRect);
		}

		if (MenuHandler::g_menuContext.immersiveStyle)
		{
			// I don't know how this value come from
			constexpr int immersiveContextMenuSeparatorPadding{ 8 };

			RECT clipRect{};
			GetClipBox(hdc, &clipRect);

			if (iPartId == MENU_POPUPSEPARATOR)
			{
				SetRect(const_cast<LPRECT>(pRect), clipRect.left + immersiveContextMenuSeparatorPadding, pRect->top, clipRect.right - immersiveContextMenuSeparatorPadding, pRect->bottom);
			}

			if (iPartId == MENU_POPUPCHECK)
			{
				OffsetRect(const_cast<LPRECT>(pRect), clipRect.left + immersiveContextMenuSeparatorPadding - pRect->left, 0);
			}

			if (iPartId == MENU_POPUPCHECKBACKGROUND)
			{
				return true;
			}

			if (
				(iPartId == MENU_POPUPITEM || iPartId == MENU_POPUPITEM_FOCUSABLE) &&
				iStateId == MPI_HOT &&
				!MenuHandler::g_menuContext.customRendering.enable
			)
			{
				std::wstring_view themeClass
				{
					MenuHandler::g_menuContext.useDarkMode ?
					L"DarkMode_ImmersiveStart::Menu" :
					L"LightMode_ImmersiveStart::Menu"
				};
				wil::unique_htheme themeHandle{ OpenThemeData(nullptr, themeClass.data()) };
				if (!themeHandle)
				{
					return false;
				}

				g_actualDrawThemeBackground(
					themeHandle.get(),
					hdc,
					iPartId,
					iStateId,
					pRect,
					pClipRect
				);
				return true;
			}
		}

		if (iPartId == MENU_POPUPBORDERS)
		{
			PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS);
			if (!MenuRendering::HandlePopupMenuNonClientBorderColors(hdc, clipRect))
			{
				Utils::unique_ext_hdc dc{ hdc };

				ExcludeClipRect(dc.get(), clipRect.left + MenuAppearance::systemOutlineSize, clipRect.top + MenuAppearance::systemOutlineSize, clipRect.right - MenuAppearance::systemOutlineSize, clipRect.bottom - MenuAppearance::systemOutlineSize);

				g_actualDrawThemeBackground(
					hTheme,
					hdc,
					iPartId,
					iStateId,
					pRect,
					pClipRect
				);
			}

			return true;
		}

		if (!MenuRendering::HandleDrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect, g_actualDrawThemeBackground))
		{
			return false;
		}

		return true;
	};
	if (!handler())
	{
		return g_actualDrawThemeBackground(
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

HRESULT WINAPI UxThemeHooks::MyDrawThemeText(
	HTHEME hTheme,
	HDC hdc,
	int iPartId,
	int iStateId,
	LPCWSTR pszText,
	int cchText,
	DWORD dwTextFlags,
	DWORD,
	LPCRECT pRect
)
{
	HRESULT hr{ S_OK };

	auto callerModule{ DetourGetContainingModule(_ReturnAddress()) };
	auto handler = [&]() -> bool
	{
		if (!MenuHandler::g_uxhooksEnabled)
		{
			return false;
		}

		WCHAR themeClassName[MAX_PATH + 1]{};
		if (FAILED(ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)))
		{
			return false;
		}

		if (_wcsicmp(themeClassName, L"Menu"))
		{
			return false;
		}

		if (IsRectEmpty(pRect))
		{
			return false;
		}

		if (callerModule != GetModuleHandleW(L"uxtheme.dll"))
		{
			return false;
		}

		DTTOPTS options{ sizeof(DTTOPTS), DTT_COMPOSITED };

		auto f = [&](HDC memoryDC, HPAINTBUFFER, RGBQUAD*, int)
		{
			THROW_IF_FAILED(
				DrawThemeTextEx(
					hTheme,
					memoryDC,
					iPartId,
					iStateId,
					pszText,
					cchText,
					dwTextFlags,
					const_cast<LPRECT>(pRect),
					&options
				)
			);
		};
		if (FAILED(ThemeHelper::DrawThemeContent(hdc, *pRect, nullptr, nullptr, 0, f)))
		{
			return false;
		}

		return true;
	};
	if (!handler())
	{
		hr = g_actualDrawThemeText(
			hTheme,
			hdc,
			iPartId,
			iStateId,
			pszText,
			cchText,
			dwTextFlags,
			0,
			pRect
		);
	}

	return hr;
}

HRESULT WINAPI UxThemeHooks::MyDwmSetWindowAttribute(
	HWND    hwnd,
	DWORD   dwAttribute,
	LPCVOID pvAttribute,
	DWORD   cbAttribute
)
{
	COLORREF color{ Utils::MakeCOLORREF(MenuHandler::g_menuContext.border.color) };
	if (MenuHandler::g_uxhooksEnabled && Utils::IsPopupMenu(hwnd))
	{
		if (dwAttribute == DWMWA_WINDOW_CORNER_PREFERENCE)
		{
			if (MenuHandler::g_menuContext.border.cornerType != DWM_WINDOW_CORNER_PREFERENCE::DWMWCP_DEFAULT)
			{
				pvAttribute = &MenuHandler::g_menuContext.border.cornerType;
			}
		}

		if (dwAttribute == DWMWA_BORDER_COLOR)
		{
			if (MenuHandler::g_menuContext.border.colorUseNone)
			{
				color = DWMWA_COLOR_NONE;
				pvAttribute = &color;
			}
			else if (!MenuHandler::g_menuContext.border.colorUseDefault)
			{
				pvAttribute = &color;
			}
		}
	}

	return g_hookDispatcher.GetOrg<decltype(&MyDwmSetWindowAttribute), 1>()(
		hwnd,
		dwAttribute,
		pvAttribute,
		cbAttribute
	);
}

BOOL WINAPI UxThemeHooks::MyStretchBlt(
	HDC   hdcDest,
	int   xDest,
	int   yDest,
	int   wDest,
	int   hDest,
	HDC   hdcSrc,
	int   xSrc,
	int   ySrc,
	int   wSrc,
	int   hSrc,
	DWORD rop
)
{
	BOOL result{FALSE};

	auto callerModule{ DetourGetContainingModule(_ReturnAddress()) };
	auto handler = [&]() -> bool
	{
		if (!MenuHandler::g_uxhooksEnabled)
		{
			return false;
		}

		if (!wSrc || !hSrc || !wDest || !hDest)
		{
			return false;
		}

		if (callerModule != GetModuleHandleW(L"uxtheme.dll"))
		{
			return false;
		}

		wil::unique_hbitmap bitmap{ nullptr };
		HBITMAP hBitmap{ reinterpret_cast<HBITMAP>(GetCurrentObject(hdcSrc, OBJ_BITMAP)) };
		if (!MenuRendering::HandleMenuBitmap(hBitmap, bitmap))
		{
			return false;
		}

		auto selectObjectCleanUp = wil::SelectObject(hdcSrc, hBitmap);
		if (
			!GdiAlphaBlend(
				hdcDest,
				xDest, yDest,
				wDest, hDest,
				hdcSrc,
				xSrc, ySrc,
				wSrc, hSrc,
				{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA }
			)
		)
		{
			return false;
		}

		return true;
	};
	if (!handler())
	{
		result = g_hookDispatcher.GetOrg<decltype(&MyStretchBlt), 0>()(
			hdcDest,
			xDest, yDest,
			wDest, hDest,
			hdcSrc,
			xSrc, ySrc,
			wSrc, hSrc,
			rop
		);
	}

	return result;
}

void __thiscall UxThemeHooks::CThemeMenu::MyDrawItemBitmap(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, int iStateId, const RECT* lprc)
{
	const auto actualCThemeMenu_DrawItemBitmap
	{
		Utils::union_cast<void(__thiscall CThemeMenu::*)(HWND, HDC, HBITMAP, bool, int, const RECT*)>(g_actualCThemeMenu_DrawItemBitmap)
	};

	auto handler = [&]() -> bool
	{
		if (!MenuHandler::g_uxhooksEnabled)
		{
			return false;
		}
		if (!hBitmap)
		{
			return false;
		}
		if (!fromPopupMenu)
		{
			return false;
		}
		if (SystemHelper::g_buildNumber < 22000 && MenuHandler::g_menuContext.immersiveStyle)
		{
			RECT clipRect{};
			GetClipBox(hdc, &clipRect);
			constexpr int immersiveContextMenuSeparatorPadding{ 8 };
			OffsetRect(const_cast<LPRECT>(lprc), clipRect.left + immersiveContextMenuSeparatorPadding - lprc->left, 0);
		}

		wil::unique_hbitmap bitmap{ nullptr };
		if (!MenuRendering::HandleMenuBitmap(hBitmap, bitmap))
		{
			return false;
		}

		(this->*actualCThemeMenu_DrawItemBitmap)(hWnd, hdc, hBitmap, fromPopupMenu, iStateId, lprc);
		return true;
	};
	if(!handler())
	{
		(this->*actualCThemeMenu_DrawItemBitmap)(hWnd, hdc, hBitmap, fromPopupMenu, iStateId, lprc);
	}
}

void __thiscall UxThemeHooks::CThemeMenu::MyDrawItemBitmap2(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, bool noStretch, int iStateId, const RECT* lprc)
{
	const auto actualCThemeMenu_DrawItemBitmap
	{
		Utils::union_cast<void(__thiscall CThemeMenu::*)(HWND, HDC, HBITMAP, bool, bool, int, const RECT*)>(g_actualCThemeMenu_DrawItemBitmap)
	};

	auto handler = [&]() -> bool
	{
		if (!MenuHandler::g_uxhooksEnabled)
		{
			return false;
		}
		if (!hBitmap)
		{
			return false;
		}
		if (!fromPopupMenu)
		{
			return false;
		}
		if (SystemHelper::g_buildNumber < 22000 && MenuHandler::g_menuContext.immersiveStyle)
		{
			RECT clipRect{};
			GetClipBox(hdc, &clipRect);
			constexpr int immersiveContextMenuSeparatorPadding{ 8 };
			OffsetRect(const_cast<LPRECT>(lprc), clipRect.left + immersiveContextMenuSeparatorPadding - lprc->left, 0);
		}

		wil::unique_hbitmap bitmap{ nullptr };
		if (!MenuRendering::HandleMenuBitmap(hBitmap, bitmap))
		{
			return false;
		}

		(this->*actualCThemeMenu_DrawItemBitmap)(hWnd, hdc, hBitmap, fromPopupMenu, noStretch, iStateId, lprc);
		return true;
	};
	if (!handler())
	{
		(this->*actualCThemeMenu_DrawItemBitmap)(hWnd, hdc, hBitmap, fromPopupMenu, noStretch, iStateId, lprc);
	}
}

void UxThemeHooks::Prepare()
{
	g_useCompatibleMode = static_cast<bool>(RegHelper::Get<DWORD>({ L"Menu" }, L"EnableCompatibilityMode", 0));
	if (
		g_CThemeMenu_DrawItemBitmap_Offset.IsValid() && 
		g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_1.IsValid() &&
		g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_2.IsValid() &&
		g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_1.IsValid() &&
		g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_2.IsValid() &&
		g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_3.IsValid() &&
		g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_4.IsValid() &&
		g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_1.IsValid() &&
		g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_2.IsValid() &&
		g_CThemeMenuPopup_DrawClientArea_DrawThemeBackground_E8Offset_Offset_1.IsValid() &&
		g_CThemeMenuPopup_DrawNonClientArea_DrawThemeBackground_E8Offset_Offset_1.IsValid()
	)
	{
		return;
	}
	if (g_useCompatibleMode)
	{
		return;
	}

	HookHelper::OffsetStorage CThemeMenuPopup_DrawItem_Offset{ 0 };
	HookHelper::OffsetStorage CThemeMenuPopup_DrawItemCheck_Offset{ 0 };
	HookHelper::OffsetStorage CThemeMenuPopup_DrawClientArea_Offset{ 0 };
	HookHelper::OffsetStorage CThemeMenuPopup_DrawNonClientArea_Offset{ 0 };

	auto IsAPIOffsetReady = [&]()
	{
		return
			CThemeMenuPopup_DrawItem_Offset.IsValid() &&
			CThemeMenuPopup_DrawItemCheck_Offset.IsValid() &&
			CThemeMenuPopup_DrawClientArea_Offset.IsValid() &&
			CThemeMenuPopup_DrawNonClientArea_Offset.IsValid() &&
			g_CThemeMenu_DrawItemBitmap_Offset.IsValid();
	};

	auto uxthemeModule{ wil::unique_hmodule{LoadLibraryW(L"uxtheme.dll")} };
	auto currentVersion{ Utils::GetVersion(uxthemeModule.get()) };
	auto savedVersion{ RegHelper::__TryGet<std::wstring>({}, L"UxThemeVersion") };
	if (savedVersion && savedVersion.value() == currentVersion)
	{
		CThemeMenuPopup_DrawItem_Offset.value = RegHelper::__Get<DWORD64>({}, L"CThemeMenuPopup_DrawItem_Offset", 0);
		CThemeMenuPopup_DrawItemCheck_Offset.value = RegHelper::__Get<DWORD64>({}, L"CThemeMenuPopup_DrawItemCheck_Offset", 0);
		CThemeMenuPopup_DrawClientArea_Offset.value = RegHelper::__Get<DWORD64>({}, L"CThemeMenuPopup_DrawClientArea_Offset", 0);
		CThemeMenuPopup_DrawNonClientArea_Offset.value = RegHelper::__Get<DWORD64>({}, L"CThemeMenuPopup_DrawNonClientArea_Offset", 0);
		g_CThemeMenu_DrawItemBitmap_Offset.value = RegHelper::__Get<DWORD64>({}, L"CThemeMenu_DrawItemBitmap_Offset", 0);
	}

	while (!IsAPIOffsetReady())
	{
		HRESULT hr{ S_OK };
		SymbolResolver symbolResolver{ L"UxThemeHooks" };
		hr = symbolResolver.Walk(L"uxtheme.dll", "*!*", [&](PSYMBOL_INFO symInfo, ULONG symbolSize) -> bool
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
			auto functionOffset{HookHelper::OffsetStorage::From(symInfo->ModBase, symInfo->Address)};

			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenuPopup::DrawItem"))
			{
				CThemeMenuPopup_DrawItem_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenuPopup::DrawItemCheck"))
			{
				CThemeMenuPopup_DrawItemCheck_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenuPopup::DrawClientArea"))
			{
				CThemeMenuPopup_DrawClientArea_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenuPopup::DrawNonClientArea"))
			{
				CThemeMenuPopup_DrawNonClientArea_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenu::DrawItemBitmap"))
			{
				g_CThemeMenu_DrawItemBitmap_Offset = functionOffset;
			}

			if (IsAPIOffsetReady())
			{
				return false;
			}

			return true;
		});
		if (FAILED(hr))
		{
			Api::InteractiveIO::OutputToConsole(
				Api::InteractiveIO::StringType::Error,
				Api::InteractiveIO::WaitType::NoWait,
				IDS_STRING103,
				std::format(L"[UxThemeHooks] "),
				std::format(L"(hresult: {:#08x})\n", static_cast<ULONG>(hr))
			);
		}

		if (IsAPIOffsetReady())
		{
			if (symbolResolver.IsLoaded() && symbolResolver.IsInternetRequired())
			{
				std::filesystem::remove_all(Utils::make_current_folder_file_str(L"symbols"));
				Api::InteractiveIO::OutputToConsole(
					Api::InteractiveIO::StringType::Notification,
					Api::InteractiveIO::WaitType::NoWait,
					IDS_STRING101,
					std::format(L"[UxThemeHooks] "),
					L"\n"
				);
			}
		}
		else
		{
			if (GetConsoleWindow())
			{
				if (symbolResolver.IsLoaded())
				{
					Api::InteractiveIO::OutputToConsole(
						Api::InteractiveIO::StringType::Error,
						Api::InteractiveIO::WaitType::NoWait,
						IDS_STRING107,
						std::format(L"[UxThemeHooks] "),
						L"\n"
					);
					break;
				}
				else
				{
					if (
						Api::InteractiveIO::OutputToConsole(
							Api::InteractiveIO::StringType::Warning,
							Api::InteractiveIO::WaitType::WaitYN,
							IDS_STRING102,
							std::format(L"[UxThemeHooks] "),
							L"\n"
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
				Api::InteractiveIO::OutputToConsole(
					Api::InteractiveIO::StringType::Error,
					Api::InteractiveIO::WaitType::NoWait,
					IDS_STRING107,
					std::format(L"[UxThemeHooks] "),
					L"\n"
				);
				break;
			}
		}
	}

	if (IsAPIOffsetReady())
	{
		RegHelper::__Set({}, L"UxThemeVersion", currentVersion.c_str());
		RegHelper::__Set<DWORD64>({}, L"CThemeMenuPopup_DrawItem_Offset", CThemeMenuPopup_DrawItem_Offset.value);
		RegHelper::__Set<DWORD64>({}, L"CThemeMenuPopup_DrawItemCheck_Offset", CThemeMenuPopup_DrawItemCheck_Offset.value);
		RegHelper::__Set<DWORD64>({}, L"CThemeMenuPopup_DrawClientArea_Offset", CThemeMenuPopup_DrawClientArea_Offset.value);
		RegHelper::__Set<DWORD64>({}, L"CThemeMenuPopup_DrawNonClientArea_Offset", CThemeMenuPopup_DrawNonClientArea_Offset.value);
		RegHelper::__Set<DWORD64>({}, L"CThemeMenu_DrawItemBitmap_Offset", g_CThemeMenu_DrawItemBitmap_Offset.value);
		Api::InteractiveIO::OutputToConsole(
			Api::InteractiveIO::StringType::Notification,
			Api::InteractiveIO::WaitType::NoWait,
			IDS_STRING108,
			std::format(L"[UxThemeHooks] "),
			std::format(L" ({})\n", currentVersion),
			true
		);

		[&]
		{
			LONG* searchAddress{ nullptr };
			g_actualDrawThemeText = reinterpret_cast<decltype(g_actualDrawThemeText)>(GetProcAddress(uxthemeModule.get(), "DrawThemeText"));
			g_actualDrawThemeBackground = reinterpret_cast<decltype(g_actualDrawThemeBackground)>(GetProcAddress(uxthemeModule.get(), "DrawThemeBackground"));
			// 
			searchAddress = CThemeMenuPopup_DrawItem_Offset.To<LONG*>(uxthemeModule.get());
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeText);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_1 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeText);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_2 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			//
			searchAddress = CThemeMenuPopup_DrawItem_Offset.To<LONG*>(uxthemeModule.get());
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeBackground);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_1 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeBackground);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_2 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeBackground);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_3 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeBackground);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_4 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			//
			searchAddress = CThemeMenuPopup_DrawItemCheck_Offset.To<LONG*>(uxthemeModule.get());
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeBackground);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_1 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeBackground);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_2 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			//
			searchAddress = CThemeMenuPopup_DrawClientArea_Offset.To<LONG*>(uxthemeModule.get());
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeBackground);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawClientArea_DrawThemeBackground_E8Offset_Offset_1 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
			//
			searchAddress = CThemeMenuPopup_DrawNonClientArea_Offset.To<LONG*>(uxthemeModule.get());
			searchAddress = SearchE8Call(searchAddress, g_actualDrawThemeBackground);
			if (searchAddress)
			{
				g_CThemeMenuPopup_DrawNonClientArea_DrawThemeBackground_E8Offset_Offset_1 = HookHelper::OffsetStorage::From(uxthemeModule.get(), searchAddress);
				searchAddress += 1;
			}
			else
			{
				return;
			}
		} ();
	}

	Api::InteractiveIO::OutputToConsole(
		Api::InteractiveIO::StringType::Notification,
		Api::InteractiveIO::WaitType::NoWait,
		0,
		std::format(L"[UxThemeHooks] "),
		std::format(L"Over. \n"),
		true
	);
}

void UxThemeHooks::Startup()
{
	HMODULE uxthemeModule{ GetModuleHandleW(L"uxtheme.dll") };

	if (!uxthemeModule)
	{
		return;
	}

	g_compatibleMode = g_useCompatibleMode;
	g_actualDrawThemeBackground = reinterpret_cast<decltype(g_actualDrawThemeBackground)>(GetProcAddress(uxthemeModule, "DrawThemeBackground"));
	g_actualDrawThemeText = reinterpret_cast<decltype(g_actualDrawThemeText)>(GetProcAddress(uxthemeModule, "DrawThemeText"));

	g_detourCThemeMenu_DrawItemBitmap = (SystemHelper::g_buildNumber >= 22000 ? Utils::union_cast<PVOID>(&UxThemeHooks::CThemeMenu::MyDrawItemBitmap2) : Utils::union_cast<PVOID>(&UxThemeHooks::CThemeMenu::MyDrawItemBitmap));
	g_actualCThemeMenu_DrawItemBitmap = g_CThemeMenu_DrawItemBitmap_Offset.To(uxthemeModule);

	if (!g_compatibleMode)
	{
		g_trampolinePool = std::make_unique<TrampolinePool>(uxthemeModule);
		g_myDrawThemeText_Trampoline = g_trampolinePool->Allocate(UxThemeHooks::MyDrawThemeText);
		g_myDrawThemeBackground_Trampoline = g_trampolinePool->Allocate(UxThemeHooks::MyDrawThemeBackground);
	}
	g_hookDispatcher.moduleAddress = uxthemeModule;
	g_hookDispatcher.CacheHookData();
}

void UxThemeHooks::Shutdown()
{
	DisableHooks();
	g_trampolinePool.reset();
}

void UxThemeHooks::EnableHooks(bool enable)
{
	auto lock{ g_lock.lock_exclusive() };

	bool hookChanged{ false };
	if ((enable && g_hookRef == 0) || (!enable && g_hookRef == 1))
	{
		hookChanged = true;
	}

	if (!hookChanged)
	{
		g_hookRef += enable ? 1 : -1;
		return;
	}

	HMODULE uxthemeModule{ GetModuleHandleW(L"uxtheme.dll") };
	
	if (enable)
	{
		if (!g_compatibleMode)
		{
			g_hookTable[g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_1.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_1.To<LONG*>(uxthemeModule),
					g_myDrawThemeText_Trampoline
				);
			g_hookTable[g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_2.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawItem_DrawThemeText_E8Offset_Offset_2.To<LONG*>(uxthemeModule),
					g_myDrawThemeText_Trampoline
				);
			//
			g_hookTable[g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_1.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_1.To<LONG*>(uxthemeModule),
					g_myDrawThemeBackground_Trampoline
				);
			g_hookTable[g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_2.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_2.To<LONG*>(uxthemeModule),
					g_myDrawThemeBackground_Trampoline
				);
			g_hookTable[g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_3.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_3.To<LONG*>(uxthemeModule),
					g_myDrawThemeBackground_Trampoline
				);
			g_hookTable[g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_4.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawItem_DrawThemeBackground_E8Offset_Offset_4.To<LONG*>(uxthemeModule),
					g_myDrawThemeBackground_Trampoline
				);
			//
			g_hookTable[g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_1.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_1.To<LONG*>(uxthemeModule),
					g_myDrawThemeBackground_Trampoline
				);
			g_hookTable[g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_2.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawItemCheck_DrawThemeBackground_E8Offset_Offset_2.To<LONG*>(uxthemeModule),
					g_myDrawThemeBackground_Trampoline
				);
			//
			g_hookTable[g_CThemeMenuPopup_DrawClientArea_DrawThemeBackground_E8Offset_Offset_1.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawClientArea_DrawThemeBackground_E8Offset_Offset_1.To<LONG*>(uxthemeModule),
					g_myDrawThemeBackground_Trampoline
				);
			//
			g_hookTable[g_CThemeMenuPopup_DrawNonClientArea_DrawThemeBackground_E8Offset_Offset_1.To<LONG*>(uxthemeModule)] =
				ReplaceE8Call(
					g_CThemeMenuPopup_DrawNonClientArea_DrawThemeBackground_E8Offset_Offset_1.To<LONG*>(uxthemeModule),
					g_myDrawThemeBackground_Trampoline
				);
		}
		else
		{
			g_hookDispatcher.EnableHook(0, enable);
		}

		HookHelper::Detours::Write([&]()
		{
			if (g_compatibleMode)
			{
				HookHelper::Detours::Attach(reinterpret_cast<PVOID*>(&g_actualDrawThemeBackground), MyDrawThemeBackground);
				HookHelper::Detours::Attach(reinterpret_cast<PVOID*>(&g_actualDrawThemeText), MyDrawThemeText);
			}
			else
			{
				HookHelper::Detours::Attach(&g_actualCThemeMenu_DrawItemBitmap, g_detourCThemeMenu_DrawItemBitmap);
			}
		});
		g_hookDispatcher.EnableHook(1, enable);

		g_hookRef += 1;
	}
	else
	{
		DisableHooksInternal();
	}
}

void UxThemeHooks::DisableHooks()
{
	auto lock{ g_lock.lock_exclusive() };

	DisableHooksInternal();
}

void UxThemeHooks::DisableHooksInternal()
{
	if (g_hookRef)
	{
		HMODULE uxthemeModule{ GetModuleHandleW(L"uxtheme.dll") };

		if (!g_compatibleMode)
		{
			for (auto& [offsetAddress, originalOffset] : g_hookTable)
			{
				HookHelper::WriteMemory(offsetAddress, [&]
				{
					*offsetAddress = originalOffset;
				});
			}
			g_hookTable.clear();
		}
		g_hookDispatcher.DisableAllHooks();

		HookHelper::Detours::Write([&]()
		{
			if (g_compatibleMode)
			{
				HookHelper::Detours::Detach(reinterpret_cast<PVOID*>(&g_actualDrawThemeBackground), MyDrawThemeBackground);
				HookHelper::Detours::Detach(reinterpret_cast<PVOID*>(&g_actualDrawThemeText), MyDrawThemeText);
			}
			else
			{
				HookHelper::Detours::Detach(&g_actualCThemeMenu_DrawItemBitmap, g_detourCThemeMenu_DrawItemBitmap);
			}
		});

		g_hookRef = 0;
	}
}