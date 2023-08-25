#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace MenuRendering
	{
		HRESULT DoCustomThemeRendering(HDC hdc, bool darkMode, int partId, int stateId, const RECT& clipRect, const RECT& paintRect);
		HRESULT BltWithAlpha(
			HDC   hdcDest,
			int   xDest,
			int   yDest,
			int   wDest,
			int   hDest,
			HDC   hdcSrc,
			int   xSrc,
			int   ySrc,
			int   wSrc,
			int   hSrc
		);
	}
}