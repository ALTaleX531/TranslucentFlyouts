#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace SharedUxTheme
	{
		void Startup();
		void Shutdown();

		inline HRESULT WINAPI RealDrawThemeBackground(
			HTHEME  hTheme,
			HDC     hdc,
			int     iPartId,
			int     iStateId,
			LPCRECT pRect,
			LPCRECT pClipRect
		);
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