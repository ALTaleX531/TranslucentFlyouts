#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	class MenuRendering
	{
	public:
		static MenuRendering& GetInstance();
		~MenuRendering() noexcept = default;
		MenuRendering(const MenuRendering&) = delete;
		MenuRendering& operator=(const MenuRendering&) = delete;

		HRESULT DoCustomThemeRendering(HDC hdc, bool darkMode, int partId, int stateId, const RECT& clipRect, const RECT& paintRect);
		std::optional<wil::shared_hbitmap> PromiseAlpha(HBITMAP bitmap);
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
	private:
		MenuRendering() = default;
	};
}