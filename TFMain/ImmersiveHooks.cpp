﻿#include "pch.h"
#include "Utils.hpp"
#include "HookHelper.hpp"
#include "SystemHelper.hpp"
#include "ImmersiveHooks.hpp"
#include "ThemeHelper.hpp"
#include "MenuHooks.hpp"
#include "MenuHandler.hpp"
#include "MenuRendering.hpp"
#include "HookDispatcher.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::ImmersiveHooks
{
	using namespace std::literals;
	int WINAPI MyDrawTextW(
		HDC     hdc,
		LPCWSTR lpchText,
		int     cchText,
		LPRECT  lprc,
		UINT    format
	);
	HRESULT WINAPI MyDrawThemeBackground(
		HTHEME  hTheme,
		HDC     hdc,
		int     iPartId,
		int     iStateId,
		LPCRECT pRect,
		LPCRECT pClipRect
	);
	HRESULT WINAPI MyDwmSetWindowAttribute(
		HWND    hwnd,
		DWORD   dwAttribute,
		LPCVOID pvAttribute,
		DWORD   cbAttribute
	);

	wil::srwlock g_lock{};

	HookHelper::HookDispatcherDependency g_hookDependency
	{
		std::tuple
		{
			std::array
			{
				"user32.dll"sv,
				"ext-ms-win-ntuser-draw-l1-1-0.dll"sv,
				"ext-ms-win-ntuser-misc-l1-1-0.dll"sv
			},
			"DrawTextW",
			reinterpret_cast<PVOID>(MyDrawTextW)
		},
		std::tuple
		{
			std::array
			{
				"uxtheme.dll"sv,
				"ext-ms-win-uxtheme-themes-l1-1-0.dll"sv,
				""sv
			},
			"DrawThemeBackground",
			reinterpret_cast<PVOID>(MyDrawThemeBackground)
		},
		std::tuple
		{
			std::array
			{
				"dwmapi.dll"sv,
				""sv,
				""sv
			},
			"DwmSetWindowAttribute",
			reinterpret_cast<PVOID>(MyDwmSetWindowAttribute)
		}
	};
	std::unordered_map<PVOID, HookHelper::THookDispatcher<decltype(g_hookDependency)>::type> g_hookDispatcherMap{};

	bool EnableHooksInternal(PVOID baseAddress, bool enable);
}

int WINAPI ImmersiveHooks::MyDrawTextW(
	HDC     hdc,
	LPCWSTR lpchText,
	int     cchText,
	LPRECT  lprc,
	UINT    format
)
{
	int result{ 0 };
	auto handler = [&]() -> bool
	{
		if (MenuHandler::g_drawItemStruct == nullptr)
		{
			return false;
		}
		if ((format & DT_CALCRECT) || (format & DT_INTERNAL) || (format & DT_NOCLIP))
		{
			return false;
		}
		if (
			FAILED(
				ThemeHelper::DrawTextWithAlpha(
					hdc,
					lpchText,
					cchText,
					lprc,
					format,
					result
				)
			)
		)
		{
			return false;
		}

		return true;
	};
	if (!handler())
	{
		auto& dispatcher{ g_hookDispatcherMap.at(DetourGetContainingModule(_ReturnAddress())) };
		result = dispatcher.GetOrg<0, decltype(&ImmersiveHooks::MyDrawTextW)>()(hdc, lpchText, cchText, lprc, format);
	}

	return result;
}

HRESULT WINAPI ImmersiveHooks::MyDrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	HRESULT hr{S_OK};
	auto& dispatcher{ g_hookDispatcherMap.at(DetourGetContainingModule(_ReturnAddress())) };
	auto actualDrawThemeBackground{ dispatcher.GetOrg<1, decltype(&ImmersiveHooks::MyDrawThemeBackground)>() };
	

	auto handler = [&]() -> bool
	{
		if (MenuHandler::g_drawItemStruct == nullptr)
		{
			return false;
		}

		return MenuRendering::HandleDrawThemeBackground(
			hTheme, hdc, iPartId, iStateId, pRect, pClipRect,
			actualDrawThemeBackground
		);
	};
	if (!handler())
	{
		hr = actualDrawThemeBackground(
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

HRESULT WINAPI ImmersiveHooks::MyDwmSetWindowAttribute(
	HWND    hwnd,
	DWORD   dwAttribute,
	LPCVOID pvAttribute,
	DWORD   cbAttribute
)
{
	COLORREF color{ Utils::MakeCOLORREF(MenuHandler::g_menuContext.border.color) };
	if (Utils::IsPopupMenu(hwnd))
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

	return g_hookDispatcherMap.at(DetourGetContainingModule(_ReturnAddress())).GetOrg<2, decltype(&ImmersiveHooks::MyDwmSetWindowAttribute)>()(
		hwnd,
		dwAttribute,
		pvAttribute,
		cbAttribute
	);
}

bool ImmersiveHooks::EnableHooksInternal(PVOID baseAddress, bool enable)
{
	if (!baseAddress)
	{
		return false;
	}

	if (g_hookDispatcherMap.find(baseAddress) == g_hookDispatcherMap.end())
	{
		g_hookDispatcherMap.emplace(
			baseAddress, 
			g_hookDependency
		);
	}

	auto& hookDispatcher{ g_hookDispatcherMap.at(baseAddress) };
	hookDispatcher.moduleAddress = baseAddress;
	hookDispatcher.EnableHook(0, enable);
	hookDispatcher.EnableHook(1, enable);
	if (SystemHelper::g_buildNumber >= 22000)
	{
		hookDispatcher.EnableHook(2, enable);
	}
	return true;
}

void ImmersiveHooks::Prepare()
{

}
void ImmersiveHooks::Startup()
{

}
void ImmersiveHooks::Shutdown()
{
	DisableHooks();
}

void ImmersiveHooks::EnableHooks(PVOID baseAddress, bool enable)
{
	auto lock{ g_lock.lock_exclusive() };

	if (GetModuleHandleW(L"explorerframe.dll") == baseAddress)
	{
		if (GetModuleHandleW(L"explorer.exe"))
		{
			EnableHooksInternal(GetModuleHandleW(L"shell32.dll"), enable);
			EnableHooksInternal(GetModuleHandleW(L"explorer.exe"), enable);
			return;
		}

		baseAddress = GetModuleHandleW(L"shell32.dll");
	}
	if (GetModuleHandleW(L"twinui.pcshell.dll") == baseAddress)
	{
		if (GetModuleHandleW(L"explorer.exe") && SystemHelper::g_buildNumber >= 22000)
		{
			EnableHooksInternal(GetModuleHandleW(L"Taskbar.dll"), enable);
		}
	}

	EnableHooksInternal(baseAddress, enable);
}

void ImmersiveHooks::DisableHooks()
{
	auto lock{ g_lock.lock_exclusive() };

	for (auto& [baseAddress, hookDispatcher] : g_hookDispatcherMap)
	{
		hookDispatcher.DisableAllHooks();
		hookDispatcher.moduleAddress = nullptr;
	}
}