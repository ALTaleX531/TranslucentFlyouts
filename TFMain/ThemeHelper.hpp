#pragma once
#include "pch.h"
#include "Utils.hpp"

namespace TranslucentFlyouts
{
	namespace ThemeHelper
	{
		static inline bool IsHighContrast()
		{
			HIGHCONTRASTW hc{sizeof(hc)};
			LOG_IF_WIN32_BOOL_FALSE(SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0));
			return hc.dwFlags & HCF_HIGHCONTRASTON;
		}

		static inline bool IsThemeAvailable(HWND hWnd = nullptr)
		{
			bool bThemeAvailable{false};
			wil::unique_htheme hTheme{OpenThemeData(hWnd, L"Composited::Window")};

			if (hTheme)
			{
				bThemeAvailable = true;
			}

			return IsAppThemed() &&
				   (GetThemeAppProperties() & STAP_ALLOW_CONTROLS) &&
				   bThemeAvailable;
		}

		static HRESULT GetThemeClass(HTHEME hTheme, LPCWSTR pszClassIdList, int cchClass)
		{
			static const auto actualGetThemeClass{reinterpret_cast<HRESULT(WINAPI*)(HTHEME, LPCWSTR, int)>(GetProcAddress(GetModuleHandleW(L"UxTheme"), MAKEINTRESOURCEA(74)))};

			if (actualGetThemeClass)
			{
				return actualGetThemeClass(hTheme, pszClassIdList, cchClass);
			}
			else
			{
				LOG_HR_MSG(E_POINTER, "actualGetThemeClass is invalid!");
			}

			return E_FAIL;
		}

		// Return true if hTheme is a dark mode theme handle
		static bool DetermineThemeMode(HTHEME hTheme, std::wstring_view appName, std::wstring_view themeClass, int iPartId, int iStateId, int iPropId) try
		{
			using namespace std;

			bool result{false};
			wstring darkModeThemeClass
			{
				appName.empty() ?
				format(L"DarkMode::{}", themeClass) :
				format(L"DarkMode_{}::{}", appName, themeClass)
			};
			wil::unique_htheme themeHandle{OpenThemeData(nullptr, darkModeThemeClass.c_str())};

			THROW_HR_IF_NULL_MSG(E_FAIL, themeHandle, "OpenThemeData(nullptr, darkModeThemeClass.c_str()");

			COLORREF darkModeColor{0};
			THROW_IF_FAILED(GetThemeColor(themeHandle.get(), iPartId, iStateId, iPropId, &darkModeColor));

			COLORREF color{0};
			THROW_IF_FAILED(GetThemeColor(hTheme, iPartId, iStateId, iPropId, &color));

			return (darkModeColor == color);
		}
		catch (...)
		{
			LOG_CAUGHT_EXCEPTION();
			return false;
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
			static const auto actualDrawTextWithGlow{reinterpret_cast<HRESULT(WINAPI*)(HDC, LPCWSTR, int, LPRECT, UINT, UINT, UINT, UINT, UINT, BOOL, DTT_CALLBACK_PROC, LPARAM)>(GetProcAddress(GetModuleHandle(TEXT("UxTheme")), MAKEINTRESOURCEA(126)))};

			if (actualDrawTextWithGlow)
			{
				return actualDrawTextWithGlow(hdc, pszText, cchText, prc, dwFlags, crText, crGlow, nGlowRadius, nGlowIntensity, bPreMultiply, actualDrawTextCallback, lParam);
			}
			else
			{
				LOG_HR_MSG(E_POINTER, "actualDrawTextWithGlow is invalid!");
			}

			return E_FAIL;
		}

		static HRESULT DrawThemeContent(
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
		CATCH_LOG_RETURN_HR(wil::ResultFromCaughtException());

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

		static inline bool IsOemBitmap(HBITMAP bitmap)
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
	}
}