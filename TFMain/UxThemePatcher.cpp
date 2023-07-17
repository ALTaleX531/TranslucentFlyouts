#include "pch.h"
#include "RegHelper.hpp"
#include "resource.h"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuRendering.hpp"
#include "SymbolResolver.hpp"
#include "DXHelper.hpp"
#include "UxThemePatcher.hpp"

namespace TranslucentFlyouts
{
	using namespace std;

#pragma data_seg("uxthemeOffset")
	int UxThemePatcher::g_uxthemeVersion {-1};
	DWORD64 UxThemePatcher::g_CThemeMenuPopup_DrawItem_Offset{0};
	DWORD64 UxThemePatcher::g_CThemeMenuPopup_DrawItemCheck_Offset{0};
	DWORD64 UxThemePatcher::g_CThemeMenuPopup_DrawClientArea_Offset{0};
	DWORD64 UxThemePatcher::g_CThemeMenuPopup_DrawNonClientArea_Offset{0};
	DWORD64 UxThemePatcher::g_CThemeMenu_DrawItemBitmap_Offset{0};
#pragma data_seg()
#pragma comment(linker,"/SECTION:uxthemeOffset,RWS")
}

TranslucentFlyouts::UxThemePatcher& TranslucentFlyouts::UxThemePatcher::GetInstance()
{
	static UxThemePatcher instance{};
	return instance;
}

TranslucentFlyouts::UxThemePatcher::UxThemePatcher()
{
	try
	{
		// uxtheme.dll
		PVOID uxthemeBase{reinterpret_cast<PVOID>(GetModuleHandleW(L"uxtheme.dll"))};
		THROW_LAST_ERROR_IF_NULL(uxthemeBase);

		// uxtheme.dll
		if (g_CThemeMenuPopup_DrawItem_Offset)
			m_actualCThemeMenuPopup_DrawItem = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenuPopup_DrawItem_Offset);
		if (g_CThemeMenuPopup_DrawItemCheck_Offset)
			m_actualCThemeMenuPopup_DrawItemCheck = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenuPopup_DrawItemCheck_Offset);
		if (g_CThemeMenuPopup_DrawClientArea_Offset)
			m_actualCThemeMenuPopup_DrawClientArea = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenuPopup_DrawClientArea_Offset);
		if (g_CThemeMenuPopup_DrawNonClientArea_Offset)
			m_actualCThemeMenuPopup_DrawNonClientArea = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenuPopup_DrawNonClientArea_Offset);
		if (g_CThemeMenu_DrawItemBitmap_Offset)
			m_actualCThemeMenu_DrawItemBitmap = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenu_DrawItemBitmap_Offset);

		m_actualDrawThemeText = reinterpret_cast<decltype(m_actualDrawThemeText)>(DetourFindFunction("uxtheme.dll", "DrawThemeText"));
		THROW_LAST_ERROR_IF_NULL(m_actualDrawThemeText);
		m_actualDrawThemeBackground = reinterpret_cast<decltype(m_actualDrawThemeBackground)>(DetourFindFunction("uxtheme.dll", "DrawThemeBackground"));
		THROW_LAST_ERROR_IF_NULL(m_actualDrawThemeBackground);
	}
	catch (...)
	{
		m_internalError = true;
		LOG_CAUGHT_EXCEPTION();
	}
}

TranslucentFlyouts::UxThemePatcher::~UxThemePatcher() noexcept
{
	ShutdownHook();
}

HRESULT WINAPI TranslucentFlyouts::UxThemePatcher::DrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	HRESULT hr{S_OK};

	hr = [&]()
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hTheme);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, pRect);
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(pRect));
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, IsRectEmpty(pRect) == TRUE);

		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentListviewDC() != hdc &&
			MenuHandler::GetCurrentMenuDC() != hdc
		);

		bool darkMode{ThemeHelper::DetermineThemeMode(hTheme, L"", L"Menu", MENU_POPUPITEM, 0, TMT_TEXTCOLOR)};

		MenuHandler::NotifyUxThemeRendering();
		MenuHandler::NotifyMenuDarkMode(darkMode);

		RECT clipRect{*pRect};
		if (pClipRect != nullptr)
		{
			IntersectRect(&clipRect, &clipRect, pClipRect);
		}

		auto& menuRendering{MenuRendering::GetInstance()};
		DWORD customRendering
		{
			RegHelper::GetDword(
				L"Menu",
				L"EnableCustomRendering",
				0,
				false
			)
		};
		// use ImmersiveContextMenu
		DWORD style
		{
			RegHelper::GetDword(
				L"Menu",
				L"EnableImmersiveStyle",
				1,
				false
			)
		};

		if (style)
		{
			// I don't know how this value come from
			constexpr int immersiveContextMenuSeparatorPadding{8};

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
				return S_OK;
			}
		}

		if (iPartId == MENU_POPUPBORDERS)
		{
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
				if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
				{
					return S_OK;
				}
			}
		}
		// Separator
		if (iPartId == MENU_POPUPSEPARATOR)
		{
			if (customRendering)
			{
				if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
				{
					return S_OK;
				}
			}
		}
		// Focusing
		if (iPartId == MENU_POPUPITEMKBFOCUS)
		{
			if (customRendering)
			{
				if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
				{
					return S_OK;
				}
			}
		}
		if ((iPartId == MENU_POPUPITEM || iPartId == MENU_POPUPITEM_FOCUSABLE))
		{
			if (iStateId == MPI_DISABLEDHOT)
			{
				if (customRendering)
				{
					if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
					{
						return S_OK;
					}
				}
			}
			if (iStateId == MPI_HOT)
			{
				if (customRendering)
				{
					if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
					{
						return S_OK;
					}
				}

				// use ImmersiveContextMenu
				if (style)
				{
					wstring_view themeClass
					{
						darkMode ?
						L"DarkMode_ImmersiveStart::Menu" :
						L"LightMode_ImmersiveStart::Menu"
					};
					wil::unique_htheme themeHandle{OpenThemeData(nullptr, themeClass.data())};
					RETURN_HR_IF_NULL(E_FAIL, themeHandle);

					return ::DrawThemeBackground(
							   themeHandle.get(),
							   hdc,
							   iPartId,
							   iStateId,
							   pRect,
							   pClipRect
						   );
				}

				return E_NOTIMPL;
			}
		}

		{
			RETURN_HR_IF_EXPECTED(
				E_NOTIMPL,
				iPartId != MENU_POPUPBACKGROUND &&
				iPartId != MENU_POPUPBORDERS &&
				iPartId != MENU_POPUPGUTTER &&
				iPartId != MENU_POPUPITEM &&
				iPartId != MENU_POPUPITEM_FOCUSABLE
			);

			RETURN_IF_WIN32_BOOL_FALSE(
				PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS)
			);
		}

		return S_OK;
	}();
	if (FAILED(hr))
	{
		hr = GetInstance().m_actualDrawThemeBackground(
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
HRESULT WINAPI TranslucentFlyouts::UxThemePatcher::DrawThemeText(
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
	HRESULT hr{S_OK};
	hr = [&]()
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hTheme);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, pRect);
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(pRect));
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, IsRectEmpty(pRect) == TRUE);

		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentListviewDC() != hdc &&
			MenuHandler::GetCurrentMenuDC() != hdc
		);

		DTTOPTS options{sizeof(DTTOPTS), DTT_COMPOSITED};

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
		RETURN_IF_FAILED(ThemeHelper::DrawThemeContent(hdc, *pRect, nullptr, nullptr, 0, f));

		return S_OK;
	}();
	if (FAILED(hr))
	{
		hr = GetInstance().m_actualDrawThemeText(
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

void WINAPI TranslucentFlyouts::UxThemePatcher::CThemeMenu::DrawItemBitmap(CThemeMenu* This, HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, int iStateId, const RECT* lprc)
{
#ifndef _WIN64
#error This function currently cannot work properly!
#endif // !_WIN64
	HRESULT hr{S_OK};

	hr = [&]()
	{
		RETURN_HR_IF_NULL(E_INVALIDARG, lprc);
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(lprc));
		RETURN_HR_IF_NULL(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL(E_INVALIDARG, hBitmap);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			!fromPopupMenu
		);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			ThemeHelper::IsOemBitmap(hBitmap)
		);

		DWORD style
		{
			RegHelper::GetDword(
				L"Menu",
				L"EnableImmersiveStyle",
				1,
				false
			)
		};
		if (style)
		{
			RECT clipRect{};
			GetClipBox(hdc, &clipRect);
			constexpr int immersiveContextMenuSeparatorPadding{8};
			OffsetRect(const_cast<LPRECT>(lprc), clipRect.left + immersiveContextMenuSeparatorPadding - lprc->left, 0);
		}

		auto bitmap{MenuRendering::GetInstance().PromiseAlpha(hBitmap)};
		RETURN_LAST_ERROR_IF(bitmap && !bitmap.value().get());
		reinterpret_cast<decltype(DrawItemBitmap)*>(GetInstance().m_actualCThemeMenu_DrawItemBitmap)(This, hWnd, hdc, bitmap ? bitmap.value().get() : hBitmap, fromPopupMenu, iStateId, lprc);

		return S_OK;
	}();
	if (FAILED(hr))
	{
		reinterpret_cast<decltype(DrawItemBitmap)*>(GetInstance().m_actualCThemeMenu_DrawItemBitmap)(This, hWnd, hdc, hBitmap, fromPopupMenu, iStateId, lprc);
	}

	return;
}

void WINAPI TranslucentFlyouts::UxThemePatcher::CThemeMenu::DrawItemBitmap2(CThemeMenu* This, HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, bool noStretch, int iStateId, const RECT* lprc)
{
	HRESULT hr{S_OK};

	hr = [&]()
	{
		RETURN_HR_IF_NULL(E_INVALIDARG, lprc);
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(lprc));
		RETURN_HR_IF_NULL(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL(E_INVALIDARG, hBitmap);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			!fromPopupMenu
		);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			ThemeHelper::IsOemBitmap(hBitmap)
		);

		auto bitmap{MenuRendering::GetInstance().PromiseAlpha(hBitmap)};
		RETURN_LAST_ERROR_IF(bitmap && !bitmap.value().get());
		reinterpret_cast<decltype(DrawItemBitmap2)*>(GetInstance().m_actualCThemeMenu_DrawItemBitmap)(This, hWnd, hdc, bitmap ? bitmap.value().get() : hBitmap, fromPopupMenu, noStretch, iStateId, lprc);

		return S_OK;
	}();
	if (FAILED(hr))
	{
		reinterpret_cast<decltype(DrawItemBitmap2)*>(GetInstance().m_actualCThemeMenu_DrawItemBitmap)(This, hWnd, hdc, hBitmap, fromPopupMenu, noStretch, iStateId, lprc);
	}

	return;
}

bool TranslucentFlyouts::UxThemePatcher::IsUxThemeAPIOffsetReady()
{
	if (
		g_CThemeMenuPopup_DrawItem_Offset != 0 &&
		g_CThemeMenuPopup_DrawItemCheck_Offset != 0 &&
		g_CThemeMenuPopup_DrawClientArea_Offset != 0 &&
		g_CThemeMenuPopup_DrawNonClientArea_Offset != 0 &&
		g_CThemeMenu_DrawItemBitmap_Offset != 0
	)
	{
		return true;
	}

	return false;
}

void TranslucentFlyouts::UxThemePatcher::InitUxThemeOffset()
{
	// TO-DO
	// Cache the offset information into the registry so that we don't need to calculate them every time

	while (!IsUxThemeAPIOffsetReady())
	{
		HRESULT hr{S_OK};
		SymbolResolver symbolResolver{};
		hr = symbolResolver.Walk(L"uxtheme.dll"sv, "*!*", [](PSYMBOL_INFO symInfo, ULONG symbolSize) -> bool
		{
			auto functionName{reinterpret_cast<const CHAR*>(symInfo->Name)};
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
			auto functionOffset{symInfo->Address - symInfo->ModBase};

			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenuPopup::DrawItem"))
			{
				g_CThemeMenuPopup_DrawItem_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenuPopup::DrawItemCheck"))
			{
				g_CThemeMenuPopup_DrawItemCheck_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenuPopup::DrawClientArea"))
			{
				g_CThemeMenuPopup_DrawClientArea_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenuPopup::DrawNonClientArea"))
			{
				g_CThemeMenuPopup_DrawNonClientArea_Offset = functionOffset;
			}
#ifdef _WIN64
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenu::DrawItemBitmap"))
			{
				g_uxthemeVersion = 0;
				g_CThemeMenu_DrawItemBitmap_Offset = functionOffset;
			}
			if (!strcmp(unDecoratedFunctionName, "void __cdecl CThemeMenu::DrawItemBitmap(struct HWND__ * __ptr64,struct HDC__ * __ptr64,struct HBITMAP__ * __ptr64,bool,int,struct tagRECT const * __ptr64) __ptr64"))
			{
				g_uxthemeVersion = 0;
				g_CThemeMenu_DrawItemBitmap_Offset = functionOffset;
			}
			if (!strcmp(unDecoratedFunctionName, "void __cdecl CThemeMenu::DrawItemBitmap(struct HWND__ * __ptr64,struct HDC__ * __ptr64,struct HBITMAP__ * __ptr64,bool,bool,int,struct tagRECT const * __ptr64) __ptr64"))
			{
				g_uxthemeVersion = 1;
				g_CThemeMenu_DrawItemBitmap_Offset = functionOffset;
			}
#else
			if (!strcmp(fullyUnDecoratedFunctionName, "CThemeMenu::DrawItemBitmap"))
			{
				g_uxthemeVersion = 0;
				g_CThemeMenu_DrawItemBitmap_Offset = functionOffset;
			}
			if (!strcmp(unDecoratedFunctionName, "void __thiscall CThemeMenu::DrawItemBitmap(struct HWND__ *,struct HDC__ *,struct HBITMAP__ *,bool,int,struct tagRECT const *)"))
			{
				g_uxthemeVersion = 0;
				g_CThemeMenu_DrawItemBitmap_Offset = functionOffset;
			}
			if (!strcmp(unDecoratedFunctionName, "void __thiscall CThemeMenu::DrawItemBitmap(struct HWND__ *,struct HDC__ *,struct HBITMAP__ *,bool,bool,int,struct tagRECT const *)"))
			{
				g_uxthemeVersion = 1;
				g_CThemeMenu_DrawItemBitmap_Offset = functionOffset;
			}
#endif // _WIN64

			if (IsUxThemeAPIOffsetReady())
			{
				return false;
			}

			return true;
		});
		THROW_IF_FAILED(hr);

		if (IsUxThemeAPIOffsetReady())
		{
			if (symbolResolver.GetLastSymbolSource())
			{
				Utils::OutputModuleString(IDS_STRING101);
			}
		}
		else if (GetConsoleWindow())
		{
			if (symbolResolver.GetLastSymbolSource())
			{
				Utils::OutputModuleString(IDS_STRING107);

				if (Utils::ConsoleGetConfirmation())
				{
					continue;
				}
				else
				{
					break;
				}
			}
			else
			{
				Utils::OutputModuleString(IDS_STRING102);

				if (Utils::ConsoleGetConfirmation())
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
			Utils::StartupConsole();
			Utils::OutputModuleString(IDS_STRING107);

			if (Utils::ConsoleGetConfirmation())
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}
}

void TranslucentFlyouts::UxThemePatcher::PrepareUxTheme() try
{
	InitUxThemeOffset();
}
catch (...)
{
	Utils::StartupConsole();
	wprintf_s(L"exception caught: 0x%x!\n", wil::ResultFromCaughtException());
	Utils::OutputModuleString(IDS_STRING103);

	LOG_CAUGHT_EXCEPTION();
	return;
}

void TranslucentFlyouts::UxThemePatcher::StartupHook()
{
	if (m_internalError)
	{
		return;
	}

	m_callHook.Attach(
		m_actualCThemeMenuPopup_DrawItem,
		m_actualDrawThemeText,
		UxThemePatcher::DrawThemeText,
		2
	);

	m_callHook.Attach(
		m_actualCThemeMenuPopup_DrawItem,
		m_actualDrawThemeBackground,
		UxThemePatcher::DrawThemeBackground,
		4
	);
	m_callHook.Attach(
		m_actualCThemeMenuPopup_DrawClientArea,
		m_actualDrawThemeBackground,
		UxThemePatcher::DrawThemeBackground,
		1
	);
	m_callHook.Attach(
		m_actualCThemeMenuPopup_DrawNonClientArea,
		m_actualDrawThemeBackground,
		UxThemePatcher::DrawThemeBackground,
		1
	);

	PVOID detourDestination{nullptr};
	if (g_uxthemeVersion == 0)
	{
		detourDestination = UxThemePatcher::CThemeMenu::DrawItemBitmap;
	}
	if (g_uxthemeVersion == 1)
	{
		detourDestination = UxThemePatcher::CThemeMenu::DrawItemBitmap2;
	}

	Hooking::Detours::Write([&]()
	{
		Hooking::Detours::Attach(&m_actualCThemeMenu_DrawItemBitmap, detourDestination);
	});
}

void TranslucentFlyouts::UxThemePatcher::ShutdownHook()
{
	if (m_internalError)
	{
		return;
	}

	m_callHook.Detach();

	PVOID detourDestination{nullptr};
	if (g_uxthemeVersion == 0)
	{
		detourDestination = UxThemePatcher::CThemeMenu::DrawItemBitmap;
	}
	if (g_uxthemeVersion == 1)
	{
		detourDestination = UxThemePatcher::CThemeMenu::DrawItemBitmap2;
	}

	Hooking::Detours::Write([&]()
	{
		Hooking::Detours::Detach(&m_actualCThemeMenu_DrawItemBitmap, detourDestination);
	});
}