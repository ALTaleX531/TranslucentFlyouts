#pragma once
#include "Utils.hpp"

namespace TranslucentFlyouts
{
	namespace ThemeHelper
	{
		inline void RefreshTheme()
		{
			static const auto s_actualSetSystemVisualStyle{reinterpret_cast<HRESULT(WINAPI*)(LPCWSTR, LPCWSTR, LPCWSTR, int)>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(65)))};
			
			if (!s_actualSetSystemVisualStyle) [[unlikely]]
			{
				return;
			}

			WCHAR themeFileName[MAX_PATH + 1];
			WCHAR colorScheme[MAX_PATH + 1];
			WCHAR canonicalName[MAX_PATH + 1];
			GetCurrentThemeName(themeFileName, MAX_PATH, colorScheme, MAX_PATH, canonicalName, MAX_PATH);
			s_actualSetSystemVisualStyle(themeFileName, colorScheme, canonicalName, 0);
		}

		static bool IsHighContrast()
		{
			HIGHCONTRASTW hc{sizeof(hc)};
			SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0);
			return hc.dwFlags & HCF_HIGHCONTRASTON;
		}

		static bool ShouldAppsUseDarkMode()
		{
			static const auto s_actualShouldAppsUseDarkMode{reinterpret_cast<bool(WINAPI*)()>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(132)))};

			if (s_actualShouldAppsUseDarkMode) [[likely]]
			{
				return s_actualShouldAppsUseDarkMode();
			}

			return false;
		}

		static bool ShouldSystemUseDarkMode()
		{
			static const auto s_actualShouldSystemUseDarkMode{ reinterpret_cast<bool(WINAPI*)()>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(138))) };

			if (s_actualShouldSystemUseDarkMode) [[likely]]
			{
				return s_actualShouldSystemUseDarkMode();
			}

			return false;
		}

		static bool IsDarkModeAllowedForApp()
		{
			static const auto s_actualIsDarkModeAllowedForApp{ reinterpret_cast<bool(WINAPI*)()>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(139))) };

			if (s_actualIsDarkModeAllowedForApp) [[likely]]
			{
				return s_actualIsDarkModeAllowedForApp();
			}

			return false;
		}

		static bool IsDarkModeAllowedForWindow(HWND hWnd)
		{
			static const auto s_actualIsDarkModeAllowedForWindow{ reinterpret_cast<bool(WINAPI*)(HWND)>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(137))) };

			if (s_actualIsDarkModeAllowedForWindow) [[likely]]
			{
				return s_actualIsDarkModeAllowedForWindow(hWnd);
			}

			return false;
		}

		static HRESULT GetThemeClass(HTHEME hTheme, LPCWSTR pszClassIdList, int cchClass)
		{
			static const auto s_actualGetThemeClass{reinterpret_cast<HRESULT(WINAPI*)(HTHEME, LPCWSTR, int)>(GetProcAddress(GetModuleHandleW(L"UxTheme"), MAKEINTRESOURCEA(74)))};

			if (s_actualGetThemeClass) [[likely]]
			{
				return s_actualGetThemeClass(hTheme, pszClassIdList, cchClass);
			}

			return E_FAIL;
		}

		static HRESULT DrawTextWithGlow(
			HDC hdc,
			LPCWSTR pszText,
			int cchText,
			LPRECT prc,
			UINT dwFlags,
			UINT crText,
			UINT crGlow,
			UINT nGlowRadius,
			UINT nGlowIntensity,
			BOOL bPreMultiply,
			DTT_CALLBACK_PROC actualDrawTextCallback,
			LPARAM lParam
		)
		{
			static const auto s_actualDrawTextWithGlow{reinterpret_cast<HRESULT(WINAPI*)(HDC, LPCWSTR, int, LPRECT, UINT, UINT, UINT, UINT, UINT, BOOL, DTT_CALLBACK_PROC, LPARAM)>(GetProcAddress(GetModuleHandle(TEXT("UxTheme")), MAKEINTRESOURCEA(126)))};

			if (s_actualDrawTextWithGlow) [[likely]]
			{
				return s_actualDrawTextWithGlow(hdc, pszText, cchText, prc, dwFlags, crText, crGlow, nGlowRadius, nGlowIntensity, bPreMultiply, actualDrawTextCallback, lParam);
			}

			return E_FAIL;
		}

		inline HRESULT DrawThemeContent(
			HDC hdc,
			const RECT& paintRect,
			LPCRECT clipRect,
			LPCRECT excludeRect,
			DWORD additionalFlags,
			std::function<void(HDC, HPAINTBUFFER, RGBQUAD*, int)> callback,
			std::byte alpha = std::byte{0xFF},
			bool useBlt = false,
			bool update = true
		) try
		{
			BOOL updateTarget{FALSE};
			HDC memoryDC{nullptr};
			HPAINTBUFFER bufferedPaint{nullptr};
			BLENDFUNCTION blendFunction{AC_SRC_OVER, 0, std::to_integer<BYTE>(alpha), AC_SRC_ALPHA};
			BP_PAINTPARAMS paintParams{sizeof(BP_PAINTPARAMS), BPPF_ERASE | additionalFlags, excludeRect, !useBlt ? &blendFunction : nullptr};

			THROW_HR_IF_NULL(E_INVALIDARG, callback);

			Utils::unique_ext_hdc dc{hdc};
			{
				auto cleanUp = wil::scope_exit([&]
				{
					if (bufferedPaint != nullptr)
					{
						EndBufferedPaint(bufferedPaint, updateTarget);
						bufferedPaint = nullptr;
					}
				});

				if (clipRect)
				{
					IntersectClipRect(dc.get(), clipRect->left, clipRect->top, clipRect->right, clipRect->bottom);
				}
				if (excludeRect)
				{
					ExcludeClipRect(dc.get(), excludeRect->left, excludeRect->top, excludeRect->right, excludeRect->bottom);
				}

				bufferedPaint = BeginBufferedPaint(dc.get(), &paintRect, BPBF_TOPDOWNDIB, &paintParams, &memoryDC);
				THROW_HR_IF_NULL(E_FAIL, bufferedPaint);

				{
					auto selectedFont{wil::SelectObject(memoryDC, GetCurrentObject(dc.get(), OBJ_FONT))};
					auto selectedBrush{wil::SelectObject(memoryDC, GetCurrentObject(dc.get(), OBJ_BRUSH))};
					auto selectedPen{wil::SelectObject(memoryDC, GetCurrentObject(dc.get(), OBJ_PEN))};

					int cxRow{0};
					RGBQUAD* buffer{nullptr};
					THROW_IF_FAILED(GetBufferedPaintBits(bufferedPaint, &buffer, &cxRow));

					callback(memoryDC, bufferedPaint, buffer, cxRow);
				}

				updateTarget = TRUE;
			}

			return S_OK;
		}
		CATCH_RETURN()

		static HRESULT DrawTextWithAlpha(
			HDC     hdc,
			LPCWSTR lpchText,
			int     cchText,
			LPRECT  lprc,
			UINT    format,
			int&	result
		)
		{
			auto drawTextCallback = [](HDC hdc, LPWSTR pszText, int cchText, LPRECT prc, UINT dwFlags, LPARAM lParam) -> int
			{
				return *reinterpret_cast<int*>(lParam) = DrawTextW(hdc, pszText, cchText, prc, dwFlags);
			};

			auto callback = [&](HDC memoryDC, HPAINTBUFFER, RGBQUAD*, int)
			{
				THROW_IF_FAILED(
					ThemeHelper::DrawTextWithGlow(
						memoryDC,
						lpchText,
						cchText,
						lprc,
						format,
						GetTextColor(hdc),
						0,
						0,
						0,
						TRUE,
						drawTextCallback,
						(LPARAM)&result
					)
				);
			};

			auto callbackAsFallback = [&](HDC memoryDC, HPAINTBUFFER, RGBQUAD*, int)
			{
				DTTOPTS Options =
				{
					sizeof(DTTOPTS),
					DTT_TEXTCOLOR | DTT_COMPOSITED | DTT_CALLBACK,
					GetTextColor(hdc),
					0,
					0,
					0,
					{},
					0,
					0,
					0,
					0,
					FALSE,
					0,
					drawTextCallback,
					(LPARAM)&result
				};
				wil::unique_htheme hTheme{OpenThemeData(nullptr, TEXT("Composited::Window"))};

				THROW_HR_IF_NULL(E_FAIL, hTheme);
				THROW_IF_FAILED(DrawThemeTextEx(hTheme.get(), memoryDC, 0, 0, lpchText, cchText, format, lprc, &Options));
			};
			
			RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
			
			if (FAILED(ThemeHelper::DrawThemeContent(hdc, *lprc, nullptr, nullptr, 0, callback)))
			{
				RETURN_IF_FAILED(ThemeHelper::DrawThemeContent(hdc, *lprc, nullptr, nullptr, 0, callbackAsFallback));
			}

			return S_OK;
		}

		static bool IsOemBitmap(HBITMAP bitmap)
		{
			bool result{false};

			switch (reinterpret_cast<DWORD64>(bitmap))
			{
				case -1:	// HBMMENU_CALLBACK
				case 1:		// HBMMENU_SYSTEM
				case 2:		// HBMMENU_MBAR_RESTORE
				case 3:		// HBMMENU_MBAR_MINIMIZE
				case 5:		// HBMMENU_MBAR_CLOSE
				case 6:		// HBMMENU_MBAR_CLOSE_D
				case 7:		// HBMMENU_MBAR_MINIMIZE_D
				case 8:		// HBMMENU_POPUP_CLOSE
				case 9:		// HBMMENU_POPUP_RESTORE
				case 10:	// HBMMENU_POPUP_MAXIMIZE
				case 11:	// HBMMENU_POPUP_MINIMIZE
				{
					result = true;
					break;
				}
				default:
					break;
			}

			return result;
		}

		inline DWORD GetThemeColorizationColor(std::wstring_view type)
		{
			static const auto s_GetImmersiveColorFromColorSetEx{ reinterpret_cast<DWORD(WINAPI*)(DWORD dwImmersiveColorSet, DWORD dwImmersiveColorType, bool bIgnoreHighContrast, DWORD dwHighContrastCacheMode)>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(95))) };
			static const auto s_GetImmersiveColorTypeFromName{ reinterpret_cast<DWORD(WINAPI*)(LPCWSTR name)>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(96))) };
			static const auto s_GetImmersiveUserColorSetPreference{ reinterpret_cast<DWORD(WINAPI*)(bool bForceCheckRegistry, bool bSkipCheckOnFail)>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(98))) };

			DWORD argb{ 0 };
			if (s_GetImmersiveColorFromColorSetEx && s_GetImmersiveColorTypeFromName && s_GetImmersiveUserColorSetPreference) [[likely]]
				{
					DWORD abgr
					{
						s_GetImmersiveColorFromColorSetEx(
							s_GetImmersiveUserColorSetPreference(0, 0),
							s_GetImmersiveColorTypeFromName(type.data()),
							true,
							0
						)
					};
					argb = Utils::MakeArgb(
						abgr >> 24,
						abgr & 0xff,
						abgr >> 8 & 0xff,
						abgr >> 16 & 0xff
					);
				}
			else
			{
				BOOL opaque{ FALSE };
				DwmGetColorizationColor(&argb, &opaque);
			}

			return argb;
		}
	}
}