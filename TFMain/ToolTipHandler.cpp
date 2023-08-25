#include "pch.h"
#include "Hooking.hpp"
#include "TFMain.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "SharedUxTheme.hpp"
#include "ToolTipHandler.hpp"

using namespace TranslucentFlyouts;
using namespace std;
using namespace wil;

namespace TranslucentFlyouts::ToolTipHandler
{
	int WINAPI DrawTextW(
		HDC     hdc,
		LPCWSTR lpchText,
		int     cchText,
		LPRECT  lprc,
		UINT    format
	);
	void WinEventCallback(HWND hWnd, DWORD event);
	LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	
	void AttachTooltip(HWND hWnd);
	void DetachTooltip(HWND hWnd);

	constexpr int tooltipSubclassId{ 0 };
	const UINT WM_THATTACH{ RegisterWindowMessageW(L"TranslucentFlyouts.TooltipHandler.Attach") };
	const UINT WM_THDETACH{RegisterWindowMessageW(L"TranslucentFlyouts.TooltipHandler.Detach")};

	decltype(DrawTextW)* g_actualDrawTextW{ nullptr };
	decltype(DrawThemeBackground)* g_actualDrawThemeBackground{nullptr};

	bool g_startup{false};
	thread_local bool g_darkMode{ false };
	thread_local bool g_useUxTheme{ false };

	list<HWND> g_tooltipList{};
}

HRESULT WINAPI TranslucentFlyouts::ToolTipHandler::DrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	bool handled{ false };
	HRESULT hr{ S_OK };

	hr = [hTheme, hdc, iPartId, iStateId, pRect, pClipRect, &handled]()
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hTheme);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, pRect);
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(pRect));
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, IsRectEmpty(pRect) == TRUE);

		WCHAR themeClassName[MAX_PATH + 1] {};
		RETURN_IF_FAILED(
			ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)
		);

		if (!_wcsicmp(themeClassName, L"Tooltip"))
		{
			DWORD itemDisabled
			{
				RegHelper::GetDword(
					L"Tooltip",
					L"Disabled",
					0
				)
			};
			RETURN_HR_IF_EXPECTED(
				E_NOTIMPL, (itemDisabled != 0)
			);
			
			g_darkMode = ThemeHelper::DetermineThemeMode(hTheme, L"Explorer"sv, L"Tooltip"sv, 0, 0, TMT_TEXTCOLOR);
			
			HWND hWnd{WindowFromDC(hdc)};
			RETURN_LAST_ERROR_IF_NULL_EXPECTED(hWnd);
			RETURN_HR_IF_EXPECTED(
				E_NOTIMPL, !Utils::IsWindowClass(hWnd, TOOLTIPS_CLASSW)
			);
			RETURN_HR_IF_EXPECTED(
				E_NOTIMPL, (GetWindowStyle(hWnd) & TTS_BALLOON) == TTS_BALLOON
			);
			RETURN_HR_IF_EXPECTED(
				E_NOTIMPL,
				iPartId != TTP_STANDARD
			);

			RECT paintRect{*pRect};
			if (pClipRect != nullptr)
			{
				IntersectRect(&paintRect, &paintRect, pClipRect);
			}

			THROW_IF_WIN32_BOOL_FALSE(PatBlt(hdc, paintRect.left, paintRect.top, paintRect.right - paintRect.left, paintRect.bottom - paintRect.top, BLACKNESS));
			
			handled = true;
			g_useUxTheme = true;
			return S_OK;
		}

		return E_NOTIMPL;
	}();
	if (!handled)
	{
		hr = SharedUxTheme::RealDrawThemeBackground(
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

int WINAPI ToolTipHandler::DrawTextW(
	HDC     hdc,
	LPCWSTR lpchText,
	int     cchText,
	LPRECT  lprc,
	UINT    format
)
{
	bool handled{ false };
	HRESULT hr{ S_OK };
	int result{ 0 };

	hr = [hdc, lpchText, cchText, lprc, format, &result, &handled]()
	{
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(lprc));
		RETURN_HR_IF_EXPECTED(E_NOTIMPL, ((format & DT_CALCRECT) || (format & DT_INTERNAL) || (format & DT_NOCLIP)));
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL, !g_useUxTheme
		);
		DWORD itemDisabled
		{
			RegHelper::GetDword(
				L"Tooltip",
				L"Disabled",
				0
			)
		};
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL, (itemDisabled != 0)
		);
		HWND hWnd{ WindowFromDC(hdc) };
		RETURN_LAST_ERROR_IF_NULL_EXPECTED(hWnd);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL, !Utils::IsWindowClass(hWnd, TOOLTIPS_CLASSW)
		);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL, (GetWindowStyle(hWnd) & TTS_BALLOON) == TTS_BALLOON
		);

		handled = true;
		SetTextColor(hdc, g_darkMode ? RGB(255, 255, 255) : RGB(0, 0, 0));
		return ThemeHelper::DrawTextWithAlpha(
				   hdc,
				   lpchText,
				   cchText,
				   lprc,
				   format,
				   result
			   );
	}();
	if (!handled)
	{
		result = g_actualDrawTextW(hdc, lpchText, cchText, lprc, format);
	}
	else
	{
		LOG_IF_FAILED(hr);
	}

	return result;
}

void ToolTipHandler::AttachTooltip(HWND hWnd)
{
	DWORD itemDisabled
	{
		RegHelper::GetDword(
			L"Tooltip",
			L"Disabled",
			0
		)
	};

	if (itemDisabled)
	{
		return;
	}

	g_tooltipList.push_back(hWnd);
	SetWindowSubclass(hWnd, SubclassProc, tooltipSubclassId, 0);

	TFMain::ApplyBackdropEffect(L"Tooltip", hWnd, g_darkMode, TFMain::darkMode_GradientColor, TFMain::lightMode_GradientColor);
	TFMain::ApplyRoundCorners(L"Tooltip", hWnd);
	TFMain::ApplySysBorderColors(L"Tooltip", hWnd, g_darkMode, DWMWA_COLOR_NONE, DWMWA_COLOR_NONE);
}
void ToolTipHandler::DetachTooltip(HWND hWnd)
{
	RemoveWindowSubclass(hWnd, SubclassProc, tooltipSubclassId);
	g_tooltipList.remove(hWnd);
}

LRESULT CALLBACK ToolTipHandler::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	bool handled{ false };
	LRESULT result{ 0 };

	// Tooltip
	if (uIdSubclass == tooltipSubclassId)
	{
		if (uMsg == WM_DESTROY || uMsg == WM_THDETACH)
		{
			if (uMsg == WM_THDETACH)
			{
				EffectHelper::SetWindowBackdrop(hWnd, FALSE, 0, static_cast<DWORD>(EffectHelper::EffectType::None));
				InvalidateRect(hWnd, nullptr, TRUE);
			}

			DetachTooltip(hWnd);
		}
	}

	if (!handled)
	{
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	return result;
}

void ToolTipHandler::WinEventCallback(HWND hWnd, DWORD event)
{
	if (event == EVENT_OBJECT_SHOW)
	{
		if (
			Utils::IsWindowClass(hWnd, TOOLTIPS_CLASSW) &&
			!(GetWindowStyle(hWnd) & TTS_BALLOON)
		)
		{
			g_useUxTheme = false;
			{
				unique_hdc memoryDC{ CreateCompatibleDC(nullptr) };
				THROW_LAST_ERROR_IF_NULL(memoryDC);
				unique_hbitmap bitmap{ CreateCompatibleBitmap(memoryDC.get(), 1, 1) };
				THROW_LAST_ERROR_IF_NULL(bitmap);

				{
					auto selectedObject{ wil::SelectObject(memoryDC.get(), bitmap.get()) };
					THROW_IF_WIN32_BOOL_FALSE(PrintWindow(hWnd, memoryDC.get(), 0));
				}
			}

			if (g_useUxTheme)
			{
				AttachTooltip(hWnd);
			}
		}
	}
}

void TranslucentFlyouts::ToolTipHandler::Startup() try
{
	THROW_HR_IF(E_ILLEGAL_METHOD_CALL, g_startup);

	g_actualDrawTextW = reinterpret_cast<decltype(g_actualDrawTextW)>(DetourFindFunction("user32.dll", "DrawTextW"));
	THROW_LAST_ERROR_IF_NULL(g_actualDrawTextW);

	g_actualDrawThemeBackground = reinterpret_cast<decltype(g_actualDrawThemeBackground)>(DetourFindFunction("uxtheme.dll", "DrawThemeBackground"));
	THROW_LAST_ERROR_IF_NULL(g_actualDrawThemeBackground);

	HMODULE moduleHandle{ GetModuleHandleW(L"comctl32.dll") };
	if (moduleHandle)
	{
		Hooking::WriteDelayloadIAT(moduleHandle, "uxtheme.dll", { {"DrawThemeBackground", ToolTipHandler::DrawThemeBackground} });
		Hooking::WriteIAT(moduleHandle, "uxtheme.dll", { {"DrawThemeBackground", ToolTipHandler::DrawThemeBackground} });
		Hooking::WriteDelayloadIAT(moduleHandle, "user32.dll", { {"DrawTextW", ToolTipHandler::DrawTextW} });
		Hooking::WriteIAT(moduleHandle, "user32.dll", { {"DrawTextW", ToolTipHandler::DrawTextW} });
	}
	
	TFMain::AddCallback(WinEventCallback);

	g_startup = true;
}
CATCH_LOG_RETURN()

void TranslucentFlyouts::ToolTipHandler::Shutdown()
{
	if (!g_startup)
	{
		return;
	}

	TFMain::DeleteCallback(WinEventCallback);

	// Remove subclass for all existing tool tip
	if (!g_tooltipList.empty())
	{
		auto tooltipList{ g_tooltipList };
		for (auto tooltip : tooltipList)
		{
			SendMessage(tooltip, WM_THDETACH, 0, 0);
		}
		g_tooltipList.clear();
	}

	Hooking::WriteDelayloadIAT(GetModuleHandleW(L"comctl32.dll"), "uxtheme.dll", { {"DrawThemeBackground", g_actualDrawThemeBackground} });
	Hooking::WriteIAT(GetModuleHandleW(L"comctl32.dll"), "uxtheme.dll", { {"DrawThemeBackground", g_actualDrawThemeBackground} });
	Hooking::WriteDelayloadIAT(GetModuleHandleW(L"comctl32.dll"), "user32.dll", { {"DrawTextW", g_actualDrawTextW} });
	Hooking::WriteIAT(GetModuleHandleW(L"comctl32.dll"), "user32.dll", { {"DrawTextW", g_actualDrawTextW} });

	g_startup = false;
}
