#pragma once
#include "pch.h"
#include "ApiEx.hpp"

namespace TranslucentFlyouts::MenuHandler
{
	inline constexpr std::wstring_view menuOwnerAttached{ L"MenuOwnerAttached" };
	inline constexpr std::wstring_view menuOriginalBackgroundBrush{ L"MenuOriginalBackgroundBrush" };
	inline constexpr std::wstring_view menuSubclassProc{ L"MenuSubclassProc" };
	inline constexpr std::wstring_view uxHooksAttached{ L"UxHooksAttached" };

	constexpr UINT MN_SIZEWINDOW{ 0x01E2 };
	constexpr UINT MN_SELECTITEM{ 0x01E5 };
	constexpr UINT MN_BUTTONUP{ 0x01EF };

	inline thread_local LPDRAWITEMSTRUCT g_drawItemStruct{};
	inline thread_local bool g_uxhooksEnabled{};
	inline thread_local struct MenuContext
	{
		enum class Type
		{
			ClassicOrNativeTheme = -1,	// placeholder
			Classic,
			ThirdParty,
			NativeTheme,
			Immersive,
			TraverseLog
		};
		Type type;
		bool useDarkMode;
		bool useUxTheme;

		bool noSystemDropShadow;
		bool immersiveStyle;
		// backdrop effect
		Api::WindowBackdropEffectContext backdropEffect;
		// border
		Api::BorderContext border;
		// custom rendering
		Api::MenuCustomRenderingContext customRendering;
		// remove icon background color
		Api::MenuIconBackgroundColorRemovalContext iconBackgroundColorRemoval;
		// animation
		Api::FlyoutAnimationContext animation;
	} g_menuContext;

	void Prepare();
	void Startup();
	void Shutdown();
	void Update();
	void CALLBACK HandleWinEvent(
		HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
		LONG idObject, LONG idChild,
		DWORD dwEventThread, DWORD dwmsEventTime
	);
}