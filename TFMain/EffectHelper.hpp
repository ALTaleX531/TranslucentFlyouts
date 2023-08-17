#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace EffectHelper
	{
		enum class WINDOWCOMPOSITIONATTRIBUTE
		{
			WCA_UNDEFINED,
			WCA_NCRENDERING_ENABLED,
			WCA_NCRENDERING_POLICY,
			WCA_TRANSITIONS_FORCEDISABLED,
			WCA_ALLOW_NCPAINT,
			WCA_CAPTION_BUTTON_BOUNDS,
			WCA_NONCLIENT_RTL_LAYOUT,
			WCA_FORCE_ICONIC_REPRESENTATION,
			WCA_EXTENDED_FRAME_BOUNDS,
			WCA_HAS_ICONIC_BITMAP,
			WCA_THEME_ATTRIBUTES,
			WCA_NCRENDERING_EXILED,
			WCA_NCADORNMENTINFO,
			WCA_EXCLUDED_FROM_LIVEPREVIEW,
			WCA_VIDEO_OVERLAY_ACTIVE,
			WCA_FORCE_ACTIVEWINDOW_APPEARANCE,
			WCA_DISALLOW_PEEK,
			WCA_CLOAK,
			WCA_CLOAKED,
			WCA_ACCENT_POLICY,
			WCA_FREEZE_REPRESENTATION,
			WCA_EVER_UNCLOAKED,
			WCA_VISUAL_OWNER,
			WCA_HOLOGRAPHIC,
			WCA_EXCLUDED_FROM_DDA,
			WCA_PASSIVEUPDATEMODE,
			WCA_USEDARKMODECOLORS,
			WCA_CORNER_STYLE,
			WCA_PART_COLOR,
			WCA_DISABLE_MOVESIZE_FEEDBACK,
			WCA_LAST
		};

		struct WINDOWCOMPOSITIONATTRIBUTEDATA
		{
			DWORD dwAttribute;
			PVOID pvData;
			SIZE_T cbData;
		};

		enum class ACCENT_STATE
		{
			ACCENT_DISABLED,
			ACCENT_ENABLE_GRADIENT,
			ACCENT_ENABLE_TRANSPARENTGRADIENT,
			ACCENT_ENABLE_BLURBEHIND,	// Removed in Windows 11 22H2+
			ACCENT_ENABLE_ACRYLICBLURBEHIND,
			ACCENT_ENABLE_HOSTBACKDROP,
			ACCENT_INVALID_STATE
		};

		enum class ACCENT_FLAG
		{
			ACCENT_NONE,
			ACCENT_ENABLE_MODERN_ACRYLIC_RECIPE = 1 << 1,	// Windows 11 22H2+
			ACCENT_ENABLE_GRADIENT_COLOR = 1 << 1, // ACCENT_ENABLE_BLURBEHIND
			ACCENT_ENABLE_FULLSCREEN = 1 << 2,
			ACCENT_ENABLE_BORDER_LEFT = 1 << 5,
			ACCENT_ENABLE_BORDER_TOP = 1 << 6,
			ACCENT_ENABLE_BORDER_RIGHT = 1 << 7,
			ACCENT_ENABLE_BORDER_BOTTOM = 1 << 8,
			ACCENT_ENABLE_BLUR_RECT = 1 << 9,	// DwmpUpdateAccentBlurRect, it is conflicted with ACCENT_ENABLE_GRADIENT_COLOR when using ACCENT_ENABLE_BLURBEHIND
			ACCENT_ENABLE_BORDER = ACCENT_ENABLE_BORDER_LEFT | ACCENT_ENABLE_BORDER_TOP | ACCENT_ENABLE_BORDER_RIGHT | ACCENT_ENABLE_BORDER_BOTTOM
		};

		struct ACCENT_POLICY
		{
			DWORD AccentState;
			DWORD AccentFlags;
			DWORD dwGradientColor;
			DWORD dwAnimationId;
		};

		static const auto g_actualSetWindowCompositionAttribute
		{
			reinterpret_cast<BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBUTEDATA*)>(
				DetourFindFunction("user32", "SetWindowCompositionAttribute")
			)
		};

		static void DwmMakeWindowTransparent(HWND hwnd, BOOL enable)
		{
			DWM_BLURBEHIND bb{DWM_BB_ENABLE | static_cast<DWORD>(enable ? (DWM_BB_BLURREGION | DWM_BB_TRANSITIONONMAXIMIZED) : 0), enable, CreateRectRgn(0, 0, -1, -1), TRUE};
			DwmEnableBlurBehindWindow(hwnd, &bb);
			DeleteObject(bb.hRgnBlur);
		}

		enum class EffectType
		{
			None,
			Transparent,
			Solid,
			Blur,	// Removed in Windows 11 22H2+
			AcrylicBlur,
			// Windows 11
			ModernAcrylicBlur,
			Acrylic,
			Mica,
			MicaAlt
		};

		static void EnableWindowDarkMode(HWND hwnd, BOOL darkMode)
		{
			DwmSetWindowAttribute(hwnd, 19, &darkMode, sizeof(darkMode));
			DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
		}

		static void EnableWindowNCRendering(HWND hwnd, BOOL ncRendering)
		{
			// NOTICE WINDOWS THAT WE HAVE ACTIVATED THE WINDOW
			DefWindowProcW(hwnd, WM_NCACTIVATE, TRUE, 0);
			SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_DRAWFRAME | SWP_NOACTIVATE);
		}

		// Set specific backdrop effect for a window,
		// please remember don't call this function when you received WM_NC** messages...(eg. WM_NCCREATE, WM_NCDESTROY)
		static void SetWindowBackdrop(HWND hwnd, BOOL dropshadow, DWORD tintColor, DWORD effectType)
		{
			ACCENT_POLICY accentPolicy
			{
				static_cast<DWORD>(ACCENT_STATE::ACCENT_DISABLED),
				static_cast<DWORD>(dropshadow ? ACCENT_FLAG::ACCENT_ENABLE_BORDER : ACCENT_FLAG::ACCENT_NONE),
				tintColor,
				0
			};
			WINDOWCOMPOSITIONATTRIBUTEDATA data
			{
				static_cast<DWORD>(WINDOWCOMPOSITIONATTRIBUTE::WCA_ACCENT_POLICY),
				&accentPolicy,
				sizeof(ACCENT_POLICY)
			};
			DWM_SYSTEMBACKDROP_TYPE backdropType{DWMSBT_NONE};

			BOOL mica{FALSE};
			BOOL ncRendering{FALSE};
			BOOL windowTransparent{FALSE};

			switch (static_cast<EffectType>(effectType))
			{
				case EffectType::None:
				{
					break;
				}
				case EffectType::Solid:
				{
					accentPolicy.AccentState = static_cast<DWORD>(ACCENT_STATE::ACCENT_ENABLE_GRADIENT);
					accentPolicy.AccentFlags |= static_cast<DWORD>(ACCENT_FLAG::ACCENT_ENABLE_GRADIENT_COLOR);
					break;
				}
				case EffectType::Transparent:
				{
					accentPolicy.AccentState = static_cast<DWORD>(ACCENT_STATE::ACCENT_ENABLE_TRANSPARENTGRADIENT);
					accentPolicy.AccentFlags |= static_cast<DWORD>(ACCENT_FLAG::ACCENT_ENABLE_GRADIENT_COLOR);
					break;
				}
				case EffectType::Blur:
				{
					accentPolicy.AccentState = static_cast<DWORD>(ACCENT_STATE::ACCENT_ENABLE_BLURBEHIND);
					accentPolicy.AccentFlags |= static_cast<DWORD>(ACCENT_FLAG::ACCENT_ENABLE_GRADIENT_COLOR);
					break;
				}
				case EffectType::AcrylicBlur:
				{
					accentPolicy.AccentState = static_cast<DWORD>(ACCENT_STATE::ACCENT_ENABLE_ACRYLICBLURBEHIND);
					break;
				}

				// Windows 11(+)
				case EffectType::ModernAcrylicBlur:
				{
					accentPolicy.AccentState = static_cast<DWORD>(ACCENT_STATE::ACCENT_ENABLE_ACRYLICBLURBEHIND);
					accentPolicy.AccentFlags |= static_cast<DWORD>(ACCENT_FLAG::ACCENT_ENABLE_MODERN_ACRYLIC_RECIPE);
					break;
				}

				case EffectType::Acrylic:
				{
					backdropType = DWMSBT_TRANSIENTWINDOW;
					ncRendering = TRUE;
					windowTransparent = TRUE;
					break;
				}
				case EffectType::Mica:
				{
					mica = TRUE;

					backdropType = DWMSBT_MAINWINDOW;
					ncRendering = TRUE;
					windowTransparent = TRUE;
					break;
				}
				case EffectType::MicaAlt:
				{
					backdropType = DWMSBT_TABBEDWINDOW;
					ncRendering = TRUE;
					windowTransparent = TRUE;
					break;
				}
			}

			DwmSetWindowAttribute(hwnd, 1029, &mica, sizeof(mica));
			DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(DWM_SYSTEMBACKDROP_TYPE));
			DwmMakeWindowTransparent(hwnd, windowTransparent);
			if (g_actualSetWindowCompositionAttribute)
			{
				g_actualSetWindowCompositionAttribute(hwnd, &data);
			}
			EnableWindowNCRendering(hwnd, ncRendering);
		}
	}
}