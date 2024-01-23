#include "pch.h"
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
				"ext-ms-win-ntuser-draw-l1-1-0.dll"sv
			},
			"DrawTextW",
			reinterpret_cast<PVOID>(MyDrawTextW)
		},
		std::tuple
		{
			std::array
			{
				"uxtheme.dll"sv,
				"ext-ms-win-uxtheme-themes-l1-1-0.dll"sv
			},
			"DrawThemeBackground",
			reinterpret_cast<PVOID>(MyDrawThemeBackground)
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
	std::unordered_map<PVOID, HookHelper::HookDispatcher<3, 2>> g_hookDispatcherMap{};

	std::array<PVOID, 3> g_cachedOriginalFunction{};
	template <typename T, size_t index>
	__forceinline auto GetOrg(PVOID callerModuleAddress = reinterpret_cast<PVOID>(DetourGetContainingModule(_ReturnAddress()))) 
	{
		if (g_cachedOriginalFunction[index]) [[likely]]
		{
			return reinterpret_cast<T>(g_cachedOriginalFunction[index]);
		}
		return g_hookDispatcherMap.at(callerModuleAddress).GetOrg<T, index>();
	}

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
		result = GetOrg<decltype(&ImmersiveHooks::MyDrawTextW), 0>()(hdc, lpchText, cchText, lprc, format);
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
	auto actualDrawThemeBackground{ GetOrg<decltype(&ImmersiveHooks::MyDrawThemeBackground), 1>() };
	

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

	return GetOrg<decltype(&ImmersiveHooks::MyDwmSetWindowAttribute), 2>()(
		hwnd,
		dwAttribute,
		pvAttribute,
		cbAttribute
	);
}

bool ImmersiveHooks::EnableHooksInternal(PVOID baseAddress, bool enable)
{
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
	return hookDispatcher.EnableHook(2, enable);
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
			g_cachedOriginalFunction.fill(nullptr);
			return;
		}

		baseAddress = GetModuleHandleW(L"shell32.dll");
	}

	bool hookChanged{ EnableHooksInternal(baseAddress, enable) };
	if (hookChanged)
	{
		if (enable)
		{
			const auto& hookDispatcher{ g_hookDispatcherMap.at(baseAddress) };
			g_cachedOriginalFunction[0] = hookDispatcher.GetOrg<0>();
			g_cachedOriginalFunction[1] = hookDispatcher.GetOrg<1>();
			g_cachedOriginalFunction[2] = hookDispatcher.GetOrg<2>();
		}
		else
		{
			g_cachedOriginalFunction.fill(nullptr);
		}
	}
}

void ImmersiveHooks::DisableHooks()
{
	auto lock{ g_lock.lock_exclusive() };

	for (auto& [baseAddress, hookDispatcher] : g_hookDispatcherMap)
	{
		hookDispatcher.DisableAllHooks();
	}
}