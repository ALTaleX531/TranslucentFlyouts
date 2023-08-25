#include "pch.h"
#include "RegHelper.hpp"
#include "resource.h"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "SharedUxTheme.hpp"
#include "MenuRendering.hpp"
#include "SymbolResolver.hpp"
#include "UxThemePatcher.hpp"

using namespace std;
using namespace wil;
using namespace TranslucentFlyouts;

namespace TranslucentFlyouts::UxThemePatcher
{
	HRESULT WINAPI DrawThemeBackground(
		HTHEME  hTheme,
		HDC     hdc,
		int     iPartId,
		int     iStateId,
		LPCRECT pRect,
		LPCRECT pClipRect
	);
	HRESULT WINAPI DrawThemeText(
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
	struct CThemeMenu
	{
		void __thiscall DrawItemBitmap(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, int iStateId, const RECT* lprc);
		void __thiscall DrawItemBitmap2(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, bool noStretch, int iStateId, const RECT* lprc);	// Windows 11
	};

	void CalcAPIAddress();
	bool IsAPIOffsetReady();

#pragma data_seg(".shared")
	static int g_uxthemeVersion { -1 };
	static DWORD64 g_CThemeMenuPopup_DrawItem_Offset{ 0 };
	static DWORD64 g_CThemeMenuPopup_DrawItemCheck_Offset{ 0 };
	static DWORD64 g_CThemeMenuPopup_DrawClientArea_Offset{ 0 };
	static DWORD64 g_CThemeMenuPopup_DrawNonClientArea_Offset{ 0 };
	static DWORD64 g_CThemeMenu_DrawItemBitmap_Offset{ 0 };
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")

	bool g_startup{ false };
	bool g_hooked{ false };

	PVOID g_actualCThemeMenuPopup_DrawItem{ nullptr };
	PVOID g_actualCThemeMenuPopup_DrawItemCheck{ nullptr };
	PVOID g_actualCThemeMenuPopup_DrawClientArea{ nullptr };
	PVOID g_actualCThemeMenuPopup_DrawNonClientArea{ nullptr };
	PVOID g_actualCThemeMenu_DrawItemBitmap{ nullptr };

	decltype(DrawThemeBackground)* g_actualDrawThemeBackground{ nullptr };
	decltype(DrawThemeText)* g_actualDrawThemeText{ nullptr };

	Hooking::FunctionCallHook g_callHook{};
}

HRESULT WINAPI UxThemePatcher::DrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	bool handled{ false };
	HRESULT hr{S_OK};

	hr = [hTheme, hdc, iPartId, iStateId, pRect, pClipRect, & handled]()
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hTheme);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, pRect);
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, Utils::IsBadReadPtr(pRect));
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, IsRectEmpty(pRect) == TRUE);

		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentListviewDC() != hdc &&
			MenuHandler::GetCurrentMenuDC() != hdc
		);

		bool darkMode{ThemeHelper::DetermineThemeMode(hTheme, L"", L"Menu", MENU_POPUPITEM, 0, TMT_TEXTCOLOR)};

		MenuHandler::NotifyUxThemeRendering();
		MenuHandler::NotifyMenuDarkMode(darkMode);
		MenuHandler::NotifyMenuStyle(false);

		COLORREF color{DWMWA_COLOR_NONE};
		if (SUCCEEDED(GetThemeColor(hTheme, MENU_POPUPBORDERS, 0, TMT_FILLCOLORHINT, &color)))
		{
			MenuHandler::NotifyMenuBorderColor(color);
		}

		RECT clipRect{*pRect};
		if (pClipRect)
		{
			IntersectRect(&clipRect, &clipRect, pClipRect);
		}

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
				handled = true;
				return S_OK;
			}

			if (
				(iPartId == MENU_POPUPITEM || iPartId == MENU_POPUPITEM_FOCUSABLE) &&
				iStateId == MPI_HOT &&
				!RegHelper::GetDword(
					L"Menu",
					L"EnableCustomRendering",
					0,
					false
				)
			)
			{
				wstring_view themeClass
				{
					darkMode ?
					L"DarkMode_ImmersiveStart::Menu" :
					L"LightMode_ImmersiveStart::Menu"
				};
				unique_htheme themeHandle{ OpenThemeData(nullptr, themeClass.data()) };
				RETURN_HR_IF_NULL(E_FAIL, themeHandle);

				handled = true;
				return ::DrawThemeBackground(
						   themeHandle.get(),
						   hdc,
						   iPartId,
						   iStateId,
						   pRect,
						   pClipRect
					   );
			}
		}

		if (iPartId == MENU_POPUPBORDERS)
		{
			RETURN_IF_WIN32_BOOL_FALSE(
				PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS)
			);
			if (!MenuHandler::HandlePopupMenuNCBorderColors(hdc, darkMode, clipRect))
			{
				Utils::unique_ext_hdc dc{hdc};

				ExcludeClipRect(dc.get(), clipRect.left + MenuHandler::systemOutlineSize, clipRect.top + MenuHandler::systemOutlineSize, clipRect.right - MenuHandler::systemOutlineSize, clipRect.bottom - MenuHandler::systemOutlineSize);

				handled = true;
				return g_actualDrawThemeBackground(
						   hTheme,
						   hdc,
						   iPartId,
						   iStateId,
						   pRect,
						   pClipRect
					   );
			}
			else
			{
				handled = true;
				return S_OK;
			}
		}

		return SharedUxTheme::DrawThemeBackgroundHelper(
				   hTheme,
				   hdc,
				   iPartId,
				   iStateId,
				   pRect,
				   pClipRect,
				   darkMode,
				   handled
			   );
	}
	();
	if (!handled)
	{
		hr = g_actualDrawThemeBackground(
				 hTheme,
				 hdc,
				 iPartId,
				 iStateId,
				 pRect,
				 pClipRect
			 );
	}
	else
	{
		LOG_IF_FAILED(hr);
	}

	return hr;
}
HRESULT WINAPI UxThemePatcher::DrawThemeText(
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
	bool handled{ false };
	HRESULT hr{S_OK};
	hr = [hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, & handled]()
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hTheme);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, pRect);
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, Utils::IsBadReadPtr(pRect));
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

		handled = true;
		return S_OK;
	}
	();
	if (!handled)
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
	else
	{
		LOG_IF_FAILED(hr);
	}

	return hr;
}

void __thiscall UxThemePatcher::CThemeMenu::DrawItemBitmap(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, int iStateId, const RECT* lprc)
{
	bool handled{ false };
	HRESULT hr {S_OK};

	auto ptr
	{
#ifdef _WIN64
		Utils::member_function_pointer_cast<void(__fastcall*)(CThemeMenu*, HWND, HDC, HBITMAP, bool, const RECT*, const RECT*)>((g_actualCThemeMenu_DrawItemBitmap))
#else
		Utils::member_function_pointer_cast<void(__fastcall*)(CThemeMenu*, void* edx, HWND, HDC, HBITMAP, bool, const RECT*, const RECT*)>((g_actualCThemeMenu_DrawItemBitmap))
#endif
	};

	hr = [this, hWnd, hdc, hBitmap, fromPopupMenu, iStateId, lprc, ptr, & handled]()
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, lprc);
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, Utils::IsBadReadPtr(lprc));
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hBitmap);
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

		unique_hbitmap bitmap{ Utils::Promise32BPP(hBitmap) };
		RETURN_LAST_ERROR_IF_NULL(bitmap);
		auto color{ RegHelper::TryGetDword(L"Menu", L"ColorTreatAsTransparent", false) };
		if (color && !Utils::IsBitmapSupportAlpha(hBitmap))
		{
			Utils::BitmapApplyEffect(
				bitmap.get(),
				{
					std::make_shared<Utils::SpriteEffect>(
						color.value(),
						RegHelper::GetDword(L"Menu", L"ColorTreatAsTransparentThreshold", 50, false)
					)
				}
			);
		}

#ifdef _WIN64
		ptr(this, hWnd, hdc, bitmap.get(), fromPopupMenu, lprc, lprc);
#else
		ptr(this, nullptr, hWnd, hdc, bitmap.get(), fromPopupMenu, lprc, lprc);
#endif
		handled = true;
		return S_OK;
	}
	();
	if (!handled)
	{
#ifdef _WIN64
		ptr(this, hWnd, hdc, hBitmap, fromPopupMenu, lprc, lprc);
#else
		ptr(this, nullptr, hWnd, hdc, hBitmap, fromPopupMenu, lprc, lprc);
#endif
	}
	else
	{
		LOG_IF_FAILED(hr);
	}

	return;
}

void __thiscall UxThemePatcher::CThemeMenu::DrawItemBitmap2(HWND hWnd, HDC hdc, HBITMAP hBitmap, bool fromPopupMenu, bool noStretch, int iStateId, const RECT* lprc)
{
	bool handled{ false };
	HRESULT hr{S_OK};
	auto ptr
	{
#ifdef _WIN64
		Utils::member_function_pointer_cast<void(__fastcall*)(CThemeMenu*, HWND, HDC, HBITMAP, bool, bool, const RECT*, const RECT*)>((g_actualCThemeMenu_DrawItemBitmap))
#else
		Utils::member_function_pointer_cast<void(__fastcall*)(CThemeMenu*, void* edx, HWND, HDC, HBITMAP, bool, bool, const RECT*, const RECT*)>((g_actualCThemeMenu_DrawItemBitmap))
#endif
	};

	hr = [this, hWnd, hdc, hBitmap, fromPopupMenu, noStretch, iStateId, lprc, ptr, & handled]()
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, lprc);
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, Utils::IsBadReadPtr(lprc));
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hBitmap);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			!fromPopupMenu
		);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			ThemeHelper::IsOemBitmap(hBitmap)
		);

		unique_hbitmap bitmap{ Utils::Promise32BPP(hBitmap) };
		RETURN_LAST_ERROR_IF_NULL(bitmap);
		auto color{ RegHelper::TryGetDword(L"Menu", L"ColorTreatAsTransparent", false) };
		if (color && !Utils::IsBitmapSupportAlpha(hBitmap))
		{
			Utils::BitmapApplyEffect(
				bitmap.get(),
				{
					std::make_shared<Utils::SpriteEffect>(
						color.value(),
						RegHelper::GetDword(L"Menu", L"ColorTreatAsTransparentThreshold", 50, false)
					)
				}
			);
		}

#ifdef _WIN64
		ptr(this, hWnd, hdc, bitmap.get(), fromPopupMenu, noStretch, lprc, lprc);
#else
		ptr(this, nullptr, hWnd, hdc, bitmap.get(), fromPopupMenu, noStretch, lprc, lprc);
#endif

		handled = true;
		return S_OK;
	}
	();
	if (!handled)
	{
#ifdef _WIN64
		ptr(this, hWnd, hdc, hBitmap, fromPopupMenu, noStretch, lprc, lprc);
#else
		ptr(this, nullptr, hWnd, hdc, hBitmap, fromPopupMenu, noStretch, lprc, lprc);
#endif
	}
	else
	{
		LOG_IF_FAILED(hr);
	}

	return;
}

bool UxThemePatcher::IsAPIOffsetReady()
{
	if (
		g_CThemeMenuPopup_DrawItem_Offset &&
		g_CThemeMenuPopup_DrawItemCheck_Offset &&
		g_CThemeMenuPopup_DrawClientArea_Offset &&
		g_CThemeMenuPopup_DrawNonClientArea_Offset &&
		g_CThemeMenu_DrawItemBitmap_Offset
	)
	{
		return true;
	}

	return false;
}

void UxThemePatcher::Prepare(const TFMain::InteractiveIO& io)
{
	using TFMain::InteractiveIO;
	// TO-DO
	// Cache the offset information into the registry so that we don't need to calculate them every time

	while (!IsAPIOffsetReady())
	{
		HRESULT hr{ S_OK };
		SymbolResolver symbolResolver{L"UxThemePatcher", io};
		hr = symbolResolver.Walk(L"uxtheme.dll"sv, "*!*", [](PSYMBOL_INFO symInfo, ULONG symbolSize) -> bool
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
				std::format(L"[UxThemePatcher] "),
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
					std::format(L"[UxThemePatcher] "),
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
					std::format(L"[UxThemePatcher] "),
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
						std::format(L"[UxThemePatcher] "),
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
				std::format(L"[UxThemePatcher] "),
				L"\n"sv
			);
			break;
		}
	}

	io.OutputString(
		TFMain::InteractiveIO::StringType::Notification,
		TFMain::InteractiveIO::WaitType::NoWait,
		0,
		std::format(L"[UxThemePatcher] "),
		std::format(L"Done. \n"),
		true
	);
}

void UxThemePatcher::CalcAPIAddress() try
{
	PVOID uxthemeBase{ reinterpret_cast<PVOID>(GetModuleHandleW(L"uxtheme.dll")) };
	THROW_LAST_ERROR_IF_NULL(uxthemeBase);

	if (g_CThemeMenuPopup_DrawItem_Offset)
		g_actualCThemeMenuPopup_DrawItem = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenuPopup_DrawItem_Offset);
	if (g_CThemeMenuPopup_DrawItemCheck_Offset)
		g_actualCThemeMenuPopup_DrawItemCheck = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenuPopup_DrawItemCheck_Offset);
	if (g_CThemeMenuPopup_DrawClientArea_Offset)
		g_actualCThemeMenuPopup_DrawClientArea = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenuPopup_DrawClientArea_Offset);
	if (g_CThemeMenuPopup_DrawNonClientArea_Offset)
		g_actualCThemeMenuPopup_DrawNonClientArea = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenuPopup_DrawNonClientArea_Offset);
	if (g_CThemeMenu_DrawItemBitmap_Offset)
		g_actualCThemeMenu_DrawItemBitmap = reinterpret_cast<PVOID>(reinterpret_cast<DWORD64>(uxthemeBase) + g_CThemeMenu_DrawItemBitmap_Offset);

	g_actualDrawThemeText = reinterpret_cast<decltype(g_actualDrawThemeText)>(DetourFindFunction("uxtheme.dll", "DrawThemeText"));
	THROW_LAST_ERROR_IF_NULL(g_actualDrawThemeText);
	g_actualDrawThemeBackground = reinterpret_cast<decltype(g_actualDrawThemeBackground)>(DetourFindFunction("uxtheme.dll", "DrawThemeBackground"));
	THROW_LAST_ERROR_IF_NULL(g_actualDrawThemeBackground);
}
CATCH_LOG_RETURN()

void UxThemePatcher::Startup()
{
	if (g_startup)
	{
		return;
	}

	CalcAPIAddress();

	LOG_HR_IF(
		E_FAIL,
		g_callHook.Attach(
			g_actualCThemeMenuPopup_DrawItem,
			g_actualDrawThemeText,
			UxThemePatcher::DrawThemeText,
			2
		) != 0
	);

	LOG_HR_IF(
		E_FAIL,
		g_callHook.Attach(
			g_actualCThemeMenuPopup_DrawItem,
			g_actualDrawThemeBackground,
			UxThemePatcher::DrawThemeBackground,
			4
		) != 0
	);

	LOG_HR_IF(
		E_FAIL,
		g_callHook.Attach(
			g_actualCThemeMenuPopup_DrawItemCheck,
			g_actualDrawThemeBackground,
			UxThemePatcher::DrawThemeBackground,
			2
		) != 0
	);

	LOG_HR_IF(
		E_FAIL,
		g_callHook.Attach(
			g_actualCThemeMenuPopup_DrawClientArea,
			g_actualDrawThemeBackground,
			UxThemePatcher::DrawThemeBackground,
			1
		) != 0
	);

	LOG_HR_IF(
		E_FAIL,
		g_callHook.Attach(
			g_actualCThemeMenuPopup_DrawNonClientArea,
			g_actualDrawThemeBackground,
			UxThemePatcher::DrawThemeBackground,
			1
		) != 0
	);

	g_startup = true;

	if (g_hooked)
	{
		return;
	}

	PVOID detourDestination{nullptr};
	if (g_uxthemeVersion == 0)
	{
		detourDestination = Utils::member_function_pointer_cast<PVOID>(&UxThemePatcher::CThemeMenu::DrawItemBitmap);
	}
	if (g_uxthemeVersion == 1)
	{
		detourDestination = Utils::member_function_pointer_cast<PVOID>(&UxThemePatcher::CThemeMenu::DrawItemBitmap2);
	}

	if (detourDestination && g_actualCThemeMenu_DrawItemBitmap)
	{
		HRESULT hr
		{
			Hooking::Detours::Write([&]()
			{
				Hooking::Detours::Attach(&g_actualCThemeMenu_DrawItemBitmap, detourDestination);
			})
		};

		if (SUCCEEDED(hr))
		{
			g_hooked = true;
		}
	}
}

void UxThemePatcher::Shutdown()
{
	if (!g_startup)
	{
		return;
	}
	g_callHook.Detach();

	if (!g_hooked)
	{
		return;
	}

	PVOID detourDestination{nullptr};
	if (g_uxthemeVersion == 0)
	{
		detourDestination = Utils::member_function_pointer_cast<PVOID>(&UxThemePatcher::CThemeMenu::DrawItemBitmap);
	}
	if (g_uxthemeVersion == 1)
	{
		detourDestination = Utils::member_function_pointer_cast<PVOID>(&UxThemePatcher::CThemeMenu::DrawItemBitmap2);
	}

	if (detourDestination && g_actualCThemeMenu_DrawItemBitmap)
	{
		HRESULT hr
		{
			Hooking::Detours::Write([&]()
			{
				Hooking::Detours::Detach(&g_actualCThemeMenu_DrawItemBitmap, detourDestination);
			})
		};

		LOG_IF_FAILED(hr);
	}

	g_startup = false;
	g_hooked = false;
}