#pragma once
#include "Api.hpp"

namespace TranslucentFlyouts::Api
{
	struct WindowBackdropEffectContext
	{
		DWORD effectType;
		DWORD enableDropShadow;
		DWORD gradientColor;
	};
	struct BorderContext
	{
		bool colorUseNone;
		bool colorUseDefault;
		DWORD color;
		DWM_WINDOW_CORNER_PREFERENCE cornerType;
	};
	struct MenuCustomRenderingContext
	{
		bool enable;

		bool separator_disabled;
		DWORD separator_cornerRadius;
		DWORD separator_color;
		DWORD separator_width;
		//
		bool focusing_disabled;
		DWORD focusing_cornerRadius;
		DWORD focusing_color;
		DWORD focusing_width;
		//
		bool disabledHot_disabled;
		DWORD disabledHot_cornerRadius;
		DWORD disabledHot_color;
		//
		bool hot_disabled;
		DWORD hot_cornerRadius;
		DWORD hot_color;
	};
	struct MenuIconBackgroundColorRemovalContext
	{
		bool enable;
		DWORD colorTreatAsTransparent;
		DWORD colorTreatAsTransparentThreshold;
	};
	struct FlyoutAnimationContext
	{
		bool enable;
		DWORD fadeOutTime;

		bool immediateInterupting;
		DWORD startRatio;
		DWORD popInTime;
		DWORD fadeInTime;
		DWORD popInStyle;
	};
	struct TooltipRenderingContext
	{
		COLORREF color;
		DWORD marginsType;
		MARGINS margins;
	};

	void ApplyEffect(HWND hWnd, bool darkMode, const WindowBackdropEffectContext& backdropContext, const BorderContext& border);
	void ApplyBorderEffect(HWND hWnd, bool darkMode, const BorderContext& border);
	void ApplyBackdropEffect(HWND hWnd, bool darkMode, const WindowBackdropEffectContext& backdropContext);
	void DropEffect(std::wstring_view part, HWND hWnd);
	void QueryBackdropEffectContext(std::wstring_view part, bool darkMode, WindowBackdropEffectContext& context);
	void QueryBorderContext(std::wstring_view part, bool darkMode, BorderContext& context);
	void QueryMenuCustomRenderingContext(bool darkMode, MenuCustomRenderingContext& context);
	void QueryMenuIconBackgroundColorRemovalContext(MenuIconBackgroundColorRemovalContext& context);
	void QueryFlyoutAnimationContext(std::wstring_view part, FlyoutAnimationContext& context);
	void QueryTooltipRenderingContext(TooltipRenderingContext& context, bool darkMode);

}