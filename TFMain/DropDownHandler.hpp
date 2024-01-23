#pragma once
#include "pch.h"
#include "ApiEx.hpp"

namespace TranslucentFlyouts::DropDownHandler
{
	inline thread_local LPDRAWITEMSTRUCT g_drawItemStruct{};
	inline thread_local struct DropDownContext
	{
		bool useDarkMode;

		// backdrop effect
		Api::WindowBackdropEffectContext backdropEffect;
		// border
		Api::BorderContext border;
		// animation
		Api::FlyoutAnimationContext animation;
	} g_dropDownContext;

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