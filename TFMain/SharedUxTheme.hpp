#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace SharedUxTheme
	{
		void DelayStartup();
		void Startup();
		void Shutdown();

		HRESULT WINAPI DrawThemeBackgroundHelper(
			HTHEME  hTheme,
			HDC     hdc,
			int     iPartId,
			int     iStateId,
			LPCRECT pRect,
			LPCRECT pClipRect,
			bool	darkMode,
			bool&	handled
		);
	}
}