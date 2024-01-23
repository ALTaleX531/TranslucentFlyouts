#pragma once
#include "pch.h"

namespace TranslucentFlyouts::TooltipHandler
{
	inline thread_local struct TooltipContext
	{
		bool useDarkMode;
		HWND hwnd;

		bool noSystemDropShadow;
		// rendering context
		Api::TooltipRenderingContext renderingContext;
		// backdrop effect
		Api::WindowBackdropEffectContext backdropEffect;
		// border
		Api::BorderContext border;

		void Update(HWND hWnd, std::optional<bool> darkMode = std::nullopt);
	} g_tooltipContext;

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