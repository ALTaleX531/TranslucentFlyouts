#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace ToolTipHandler
	{
		HRESULT WINAPI DrawThemeBackground(
			HTHEME  hTheme,
			HDC     hdc,
			int     iPartId,
			int     iStateId,
			LPCRECT pRect,
			LPCRECT pClipRect
		);

		void Startup();
		void Shutdown();
	}
}