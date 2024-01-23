#include "pch.h"
#include "Utils.hpp"
#include "ApiEx.hpp"
#include "EffectHelper.hpp"
#include "SystemHelper.hpp"
#include "RegHelper.hpp"
#include "HookHelper.hpp"
#include "ThemeHelper.hpp"
#include "TooltipHooks.hpp"
#include "KbxLabelHooks.hpp"
#include "TooltipHandler.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::TooltipHandler
{
	bool ShouldTooltipUseDarkMode(HWND hWnd);
	bool IsTooltipAttachable(HWND hWnd);
	LRESULT CALLBACK TooltipSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK KbxLabelSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
}
namespace TranslucentFlyouts::TooltipHooks { extern HMODULE g_comctl32Module; }

bool TooltipHandler::ShouldTooltipUseDarkMode(HWND hWnd)
{
	if (
		GetWindowThreadProcessId(FindWindowW(L"Shell_TrayWnd", nullptr), nullptr) == GetWindowThreadProcessId(hWnd, nullptr) ||
		GetWindowThreadProcessId(FindWindowW(L"Shell_SecondaryTrayWnd", nullptr), nullptr) == GetWindowThreadProcessId(hWnd, nullptr)
	)
	{
		return ThemeHelper::ShouldSystemUseDarkMode();
	}

	return ThemeHelper::ShouldAppsUseDarkMode() && ThemeHelper::IsDarkModeAllowedForApp();
}

bool TooltipHandler::IsTooltipAttachable(HWND hWnd)
{
	if (!GetWindowTheme(hWnd))
	{
		return false;
	}
	if ((GetWindowStyle(hWnd) & TTS_BALLOON) == TTS_BALLOON)
	{
		return false;
	}

	return !HookHelper::Subclass::IsAlreadyAttached<TooltipSubclassProc>(hWnd);
}

void TooltipHandler::TooltipContext::Update(HWND hWnd, std::optional<bool> darkMode)
{
	g_tooltipContext.hwnd = hWnd;
	if (darkMode.has_value())
	{
		g_tooltipContext.useDarkMode = darkMode.value();
	}
	else
	{
		g_tooltipContext.useDarkMode = ShouldTooltipUseDarkMode(g_tooltipContext.hwnd);
	}
	// rendering context
	Api::QueryTooltipRenderingContext(g_tooltipContext.renderingContext, g_tooltipContext.useDarkMode);
	// backdrop effect
	g_tooltipContext.noSystemDropShadow = RegHelper::Get<DWORD>(
		{ L"Tooltip" },
		L"NoSystemDropShadow",
		0
	) != 0;
	Api::QueryBackdropEffectContext(L"Tooltip", g_tooltipContext.useDarkMode, g_tooltipContext.backdropEffect);
	// border part
	Api::QueryBorderContext(L"Tooltip", g_tooltipContext.useDarkMode, g_tooltipContext.border);
	// apply backdrop effect
	Api::ApplyEffect(g_tooltipContext.hwnd, g_tooltipContext.useDarkMode, g_tooltipContext.backdropEffect, g_tooltipContext.border);
}

LRESULT CALLBACK TooltipHandler::TooltipSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_WINDOWPOSCHANGED)
	{
		const auto& windowPos{ *reinterpret_cast<WINDOWPOS*>(lParam) };
		if (windowPos.flags & SWP_SHOWWINDOW)
		{
			auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };

			if (g_tooltipContext.noSystemDropShadow)
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
	if (uMsg == WM_PAINT || uMsg == WM_PRINTCLIENT)
	{
		TooltipHooks::EnableHooks(true);

		g_tooltipContext.Update(hWnd);
		SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

		auto result{DefSubclassProc(hWnd, uMsg, wParam, lParam)};

		TooltipHooks::EnableHooks(false);

		return result;
	}

	if (uMsg == HookHelper::TFM_ATTACH)
	{
		g_tooltipContext.Update(hWnd);
		SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

		return 0;
	}
	if (uMsg == HookHelper::TFM_DETACH)
	{
		if (wParam == 0)
		{
			Api::DropEffect(L"Tooltip", hWnd);

			TooltipHooks::EnableMarginHooks(false);
			SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);
			TooltipHooks::EnableMarginHooks(true);
		}

		return 0;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK TooltipHandler::KbxLabelSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (uMsg == WM_PAINT)
	{
		KbxLabelHooks::EnableHooks(true);
		auto result{ DefSubclassProc(hWnd, uMsg, wParam, lParam) };
		KbxLabelHooks::EnableHooks(false);

		return result;
	}

	if (uMsg == HookHelper::TFM_ATTACH)
	{
		g_tooltipContext.Update(hWnd, ThemeHelper::ShouldAppsUseDarkMode() && ThemeHelper::IsDarkModeAllowedForApp());
		return 0;
	}
	if (uMsg == HookHelper::TFM_DETACH)
	{
		if (wParam == 0)
		{
			Api::DropEffect(L"Tooltip", hWnd);
		}

		return 0;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CALLBACK TooltipHandler::HandleWinEvent(
	HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
	LONG idObject, LONG idChild,
	DWORD dwEventThread, DWORD dwmsEventTime
)
{
	if (Api::IsPartDisabled(L"Tooltip")) [[unlikely]]
	{
		return;
	}
	if (!TooltipHooks::g_comctl32Module) [[unlikely]]
	{
		return;
	}

	if (Utils::IsWindowClass(hWnd, TOOLTIPS_CLASSW) && IsTooltipAttachable(hWnd))
	{
		HookHelper::Subclass::Attach<TooltipSubclassProc>(hWnd, true);
	}
	if (dwEvent == EVENT_OBJECT_CREATE)
	{
		if (Utils::IsWindowClass(hWnd, L"KbxLabelClass"))
		{
			HookHelper::Subclass::Attach<KbxLabelSubclassProc>(hWnd, true);
		}
	}
}

void TooltipHandler::Prepare()
{
	TooltipHooks::Prepare();
	KbxLabelHooks::Startup();
}

void TooltipHandler::Startup()
{
	TooltipHooks::Startup();
	KbxLabelHooks::Shutdown();

	Update();
}

void TooltipHandler::Shutdown()
{
	HookHelper::Subclass::DetachAll<TooltipSubclassProc>();
	HookHelper::Subclass::DetachAll<KbxLabelSubclassProc>();

	TooltipHooks::Shutdown();
	KbxLabelHooks::Shutdown();
}

void TooltipHandler::Update()
{
	if (Api::IsPartDisabled(L"Tooltip")) [[unlikely]]
	{
		HookHelper::Subclass::DetachAll<TooltipSubclassProc>();
		TooltipHooks::DisableHooks();
		KbxLabelHooks::DisableHooks();

		return;
	}

	TooltipHooks::EnableMarginHooks(true);
}