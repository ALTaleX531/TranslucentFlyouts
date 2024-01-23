#pragma once
#include "pch.h"
#include "MenuHandler.hpp"

namespace TranslucentFlyouts::MenuRendering
{
	bool HandlePopupMenuNonClientBorderColors(HDC hdc, const RECT& paintRect);
	bool HandleCustomRendering(HDC hdc, int partId, int stateId, const RECT& clipRect, const RECT& paintRect);
	bool HandleMenuBitmap(HBITMAP& source, wil::unique_hbitmap& target);
	bool HandleDrawThemeBackground(
		HTHEME  hTheme,
		HDC     hdc,
		int     iPartId,
		int     iStateId,
		LPCRECT pRect,
		LPCRECT pClipRect,
		decltype(&DrawThemeBackground) actualDrawThemeBackground
	);
}