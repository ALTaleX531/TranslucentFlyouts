#include "pch.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "SystemHelper.hpp"
#include "ThemeHelper.hpp"
#include "ApiEx.hpp"
#include "EffectHelper.hpp"
#include "MenuAppearance.hpp"
#include "FlyoutAnimation.hpp"

using namespace TranslucentFlyouts;

void Api::ApplyEffect(HWND hWnd, bool darkMode, const WindowBackdropEffectContext& backdropContext, const BorderContext& border)
{
	ApplyBackdropEffect(hWnd, darkMode, backdropContext);
	ApplyBorderEffect(hWnd, darkMode, border);
}

void Api::ApplyBorderEffect(HWND hWnd, bool darkMode, const BorderContext& border)
{
	if (SystemHelper::g_buildNumber >= 22000)
	{
		if (border.cornerType != DWM_WINDOW_CORNER_PREFERENCE::DWMWCP_DEFAULT)
		{
			DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &border.cornerType, sizeof(border.cornerType));
		}

		COLORREF color{ Utils::MakeCOLORREF(border.color) };
		if (border.colorUseNone)
		{
			color = border.color;
		}
		if (!border.colorUseDefault || border.colorUseNone)
		{
			DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &color, sizeof(color));
		}
	}

	DwmTransitionOwnedWindow(hWnd, DWMTRANSITION_OWNEDWINDOW_REPOSITION);
}

void Api::ApplyBackdropEffect(HWND hWnd, bool darkMode, const WindowBackdropEffectContext& backdropContext)
{
	EffectHelper::EnableWindowDarkMode(hWnd, darkMode);
	EffectHelper::SetWindowBackdrop(hWnd, backdropContext.enableDropShadow, Utils::MakeCOLORREF(backdropContext.gradientColor) | (static_cast<DWORD>(Utils::GetAlphaFromARGB(backdropContext.gradientColor)) << 24), backdropContext.effectType);

	DwmTransitionOwnedWindow(hWnd, DWMTRANSITION_OWNEDWINDOW_REPOSITION);
}

void Api::DropEffect(std::wstring_view part, HWND hWnd)
{
	EffectHelper::SetWindowBackdrop(hWnd, FALSE, 0, static_cast<DWORD>(EffectHelper::EffectType::None));
	if (SystemHelper::g_buildNumber >= 22000)
	{
		auto cornerType{ DWM_WINDOW_CORNER_PREFERENCE::DWMWCP_DEFAULT };
		if (!_wcsicmp(part.data(), L"Menu"))
		{
			cornerType = DWM_WINDOW_CORNER_PREFERENCE::DWMWCP_ROUNDSMALL;
		}
		DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerType, sizeof(cornerType));

		COLORREF color{ DWMWA_COLOR_NONE };
		DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &color, sizeof(color));
	}
	InvalidateRect(hWnd, nullptr, TRUE);
}

void Api::QueryBackdropEffectContext(std::wstring_view part, bool darkMode, WindowBackdropEffectContext& context)
{
	constexpr DWORD lightMode_GradientColor{ 0x9EDDDDDD };
	constexpr DWORD darkMode_GradientColor{ 0x412B2B2B };
	RtlSecureZeroMemory(&context, sizeof(context));

	context.effectType = RegHelper::Get<DWORD>(
		{ part, L"" },
		L"EffectType",
		static_cast<DWORD>(EffectHelper::EffectType::ModernAcrylicBlur),
		1
	);
	context.enableDropShadow = RegHelper::Get<DWORD>(
		{ part, L"" },
		L"EnableDropShadow",
		0,
		1
	);

	DWORD gradientColor{};
	if (darkMode)
	{
		gradientColor = RegHelper::Get<DWORD>(
			{ part, L"" },
			L"DarkMode_GradientColor",
			darkMode_GradientColor,
			1
		);
	}
	else
	{
		gradientColor = RegHelper::Get<DWORD>(
			{ part, L"" },
			L"LightMode_GradientColor",
			lightMode_GradientColor,
			1
		);
	}
	context.gradientColor = gradientColor;
}

void Api::QueryBorderContext(std::wstring_view part, bool darkMode, BorderContext& context)
{
	RtlSecureZeroMemory(&context, sizeof(context));

	context.colorUseDefault = true;
	context.colorUseNone = RegHelper::Get<DWORD>(
		{ part, L"" },
		L"NoBorderColor",
		0,
		1
	) != 0;
	if (!context.colorUseNone)
	{
		auto enableColorization
		{
			RegHelper::Get<DWORD>(
				{part, L""},
				L"EnableThemeColorization",
				0,
				1
			) != 0
		};
		if (enableColorization)
		{
			auto colorizationType
			{
				RegHelper::Get<std::wstring>(
					{part, L""},
					darkMode ? L"DarkMode_ThemeColorizationType" : L"LightMode_ThemeColorizationType",
					L"ImmersiveStartHoverBackground",
					1
				)
			};
			context.color = ThemeHelper::GetThemeColorizationColor(colorizationType);
		}

		std::optional<DWORD> borderColor{};
		if (darkMode)
		{
			borderColor = RegHelper::TryGet<DWORD>(
				{ part, L"" },
				L"DarkMode_BorderColor",
				1
			);
		}
		else
		{
			borderColor = RegHelper::TryGet<DWORD>(
				{ part, L"" },
				L"LightMode_BorderColor",
				1
			);
		}
		if (borderColor.has_value())
		{
			context.color = borderColor.value();
		}
		else
		{
			context.colorUseDefault = enableColorization ? false : true;
		}
	}
	else
	{
		context.color = DWMWA_COLOR_NONE;
		context.colorUseDefault = false;
	}

	context.cornerType = static_cast<DWM_WINDOW_CORNER_PREFERENCE>(
		RegHelper::Get<DWORD>(
			{ part, L"" },
			L"CornerType",
			3,
			1
		)
		);
	if (SystemHelper::g_buildNumber < 22000)
	{
		context.cornerType = DWM_WINDOW_CORNER_PREFERENCE::DWMWCP_DONOTROUND;
	}
}

void Api::QueryMenuCustomRenderingContext(bool darkMode, MenuCustomRenderingContext& context)
{
	RtlSecureZeroMemory(&context, sizeof(context));

	context.enable = RegHelper::Get<DWORD>(
		{ L"Menu" },
		L"EnableCustomRendering",
		0
	) != 0;
	if (context.enable)
	{
		context.separator_disabled = RegHelper::Get<DWORD>(
			{ L"Separator" },
			L"Disabled",
			0
		) != 0;
		if (!context.separator_disabled)
		{
			context.separator_width = RegHelper::Get<DWORD>(
				{ L"Separator" },
				L"Width",
				MenuAppearance::separatorWidth
			);
			if (darkMode)
			{
				context.separator_color = RegHelper::Get<DWORD>(
					{ L"Separator" },
					L"DarkMode_Color",
					MenuAppearance::darkMode_SeparatorColor
				);
			}
			else
			{
				context.separator_color = RegHelper::Get<DWORD>(
					{ L"Separator" },
					L"LightMode_Color",
					MenuAppearance::lightMode_SeparatorColor
				);
			}
			if (
				RegHelper::Get<DWORD>(
					{ L"Separator" },
					L"EnableThemeColorization",
					0
				) != 0
				)
			{
				auto colorizationType
				{
					RegHelper::Get<std::wstring>(
						{ L"Separator" },
						darkMode ? L"DarkMode_ThemeColorizationType" : L"LightMode_ThemeColorizationType",
						L"ImmersiveStartHoverBackground"
					)
				};
				context.separator_color = ThemeHelper::GetThemeColorizationColor(colorizationType);
			}
		}

		context.focusing_disabled = RegHelper::Get<DWORD>(
			{ L"Focusing" },
			L"Disabled",
			0
		) != 0;
		if (!context.focusing_disabled)
		{
			context.focusing_cornerRadius = RegHelper::Get<DWORD>(
				{ L"Focusing" },
				L"CornerRadius",
				MenuAppearance::cornerRadius
			);
			context.focusing_width = RegHelper::Get<DWORD>(
				{ L"Focusing" },
				L"Width",
				MenuAppearance::focusingWidth
			);
			if (darkMode)
			{
				context.focusing_color = RegHelper::Get<DWORD>(
					{ L"Focusing" },
					L"DarkMode_Color",
					MenuAppearance::darkMode_FocusingColor
				);
			}
			else
			{
				context.focusing_color = RegHelper::Get<DWORD>(
					{ L"Focusing" },
					L"LightMode_Color",
					MenuAppearance::lightMode_FocusingColor
				);
			}
			if (
				RegHelper::Get<DWORD>(
					{ L"Focusing" },
					L"EnableThemeColorization",
					0
				) != 0
				)
			{
				auto colorizationType
				{
					RegHelper::Get<std::wstring>(
						{ L"Focusing" },
						darkMode ? L"DarkMode_ThemeColorizationType" : L"LightMode_ThemeColorizationType",
						L"ImmersiveStartHoverBackground"
					)
				};
				context.focusing_color = ThemeHelper::GetThemeColorizationColor(colorizationType);
			}
		}

		context.disabledHot_disabled = RegHelper::Get<DWORD>(
			{ L"DisabledHot" },
			L"Disabled",
			0
		) != 0;
		if (!context.disabledHot_disabled)
		{
			context.disabledHot_cornerRadius = RegHelper::Get<DWORD>(
				{ L"DisabledHot" },
				L"CornerRadius",
				MenuAppearance::cornerRadius
			);
			if (darkMode)
			{
				context.disabledHot_color = RegHelper::Get<DWORD>(
					{ L"DisabledHot" },
					L"DarkMode_Color",
					MenuAppearance::darkMode_DisabledHotColor
				);
			}
			else
			{
				context.disabledHot_color = RegHelper::Get<DWORD>(
					{ L"DisabledHot" },
					L"LightMode_Color",
					MenuAppearance::lightMode_DisabledHotColor
				);
			}
			if (
				RegHelper::Get<DWORD>(
					{ L"DisabledHot" },
					L"EnableThemeColorization",
					0
				) != 0
				)
			{
				auto colorizationType
				{
					RegHelper::Get<std::wstring>(
						{ L"DisabledHot" },
						darkMode ? L"DarkMode_ThemeColorizationType" : L"LightMode_ThemeColorizationType",
						L"ImmersiveStartHoverBackground"
					)
				};
				context.disabledHot_color = ThemeHelper::GetThemeColorizationColor(colorizationType);
			}
		}

		context.hot_disabled = RegHelper::Get<DWORD>(
			{ L"Hot" },
			L"Disabled",
			0
		) != 0;
		if (!context.hot_disabled)
		{
			context.hot_cornerRadius = RegHelper::Get<DWORD>(
				{ L"Hot" },
				L"CornerRadius",
				MenuAppearance::cornerRadius
			);
			if (darkMode)
			{
				context.hot_color = RegHelper::Get<DWORD>(
					{ L"Hot" },
					L"DarkMode_Color",
					MenuAppearance::darkMode_HotColor
				);
			}
			else
			{
				context.hot_color = RegHelper::Get<DWORD>(
					{ L"Hot" },
					L"LightMode_Color",
					MenuAppearance::lightMode_HotColor
				);
			}
			if (
				RegHelper::Get<DWORD>(
					{ L"Hot" },
					L"EnableThemeColorization",
					0
				) != 0
				)
			{
				auto colorizationType
				{
					RegHelper::Get<std::wstring>(
						{ L"Hot" },
						darkMode ? L"DarkMode_ThemeColorizationType" : L"LightMode_ThemeColorizationType",
						L"ImmersiveStartHoverBackground"
					)
				};
				context.hot_color = ThemeHelper::GetThemeColorizationColor(colorizationType);
			}
		}
	}
}

void Api::QueryMenuIconBackgroundColorRemovalContext(MenuIconBackgroundColorRemovalContext& context)
{
	RtlSecureZeroMemory(&context, sizeof(context));

	auto color{ RegHelper::TryGet<DWORD>({L"Menu"}, L"ColorTreatAsTransparent") };
	context.enable = color.has_value();
	if (context.enable)
	{
		context.colorTreatAsTransparent = color.value();
		context.colorTreatAsTransparentThreshold = RegHelper::Get<DWORD>({ L"Menu" }, L"ColorTreatAsTransparentThreshold", 50);
	}
}

void Api::QueryFlyoutAnimationContext(std::wstring_view part, FlyoutAnimationContext& context)
{
	RtlSecureZeroMemory(&context, sizeof(context));
	auto key{ std::format(L"{}\\Animation", part) };

	context.enable = RegHelper::Get<DWORD>(
		{ part, L"" },
		L"EnableFluentAnimation",
		0,
		1
	);
	context.fadeOutTime = RegHelper::Get<DWORD>(
		{ part, L"" },
		L"FadeOutTime",
		static_cast<DWORD>(FlyoutAnimation::standardFadeoutDuration.count()),
		1
	);
	if (context.enable)
	{
		context.startRatio = RegHelper::Get<DWORD>(
			{ part, L"" },
			L"StartRatio",
			lround(FlyoutAnimation::standardStartPosRatio * 100.f),
			1
		);
		context.popInTime = RegHelper::Get<DWORD>(
			{ part, L"" },
			L"PopInTime",
			static_cast<DWORD>(FlyoutAnimation::standardPopupInDuration.count()),
			1
		);
		context.fadeInTime = RegHelper::Get<DWORD>(
			{ part, L"" },
			L"FadeInTime",
			static_cast<DWORD>(FlyoutAnimation::standardFadeInDuration.count()),
			1
		);
		context.popInStyle = RegHelper::Get<DWORD>(
			{ part, L"" },
			L"PopInStyle",
			0,
			1
		);
		context.immediateInterupting = RegHelper::Get<DWORD>(
			{ part, L"" },
			L"EnableImmediateInterupting",
			0,
			1
		);
	}
}

void Api::QueryTooltipRenderingContext(TooltipRenderingContext& context, bool darkMode)
{
	constexpr DWORD darkMode_Color{ 0xFFFFFFFF };
	constexpr DWORD lightMode_Color{ 0xFF1A1A1A };

	RtlSecureZeroMemory(&context, sizeof(context));

	context.color = Utils::MakeCOLORREF(
		darkMode ?
		RegHelper::Get<DWORD>(
			{ L"Tooltip" }, L"DarkMode_Color", darkMode_Color
		) :
		RegHelper::Get<DWORD>(
			{ L"Tooltip" }, L"LightMode_Color", lightMode_Color
		)
	);

	context.marginsType = RegHelper::Get<DWORD>(
		{ L"Tooltip" }, L"MarginsType", 0, false
	);

	context.margins.cxLeftWidth = RegHelper::Get<DWORD>(
		{ L"Tooltip" }, L"Margins_cxLeftWidth", 6
	);
	context.margins.cxRightWidth = RegHelper::Get<DWORD>(
		{ L"Tooltip" }, L"Margins_cxRightWidth", 6
	);
	context.margins.cyTopHeight = RegHelper::Get<DWORD>(
		{ L"Tooltip" }, L"Margins_cyTopHeight", 6
	);
	context.margins.cyBottomHeight = RegHelper::Get<DWORD>(
		{ L"Tooltip" }, L"Margins_cyBottomHeight", 6
	);
}