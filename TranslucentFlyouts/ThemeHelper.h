#pragma once
#include "pch.h"

class ThemeHelper
{
public:
	typedef HRESULT(WINAPI*pfnGetThemeClass)(HTHEME hTheme, LPCTSTR pszClassName, int cchClassName);
	typedef BOOL(WINAPI*pfnIsThemeClassDefined)(HTHEME hTheme, LPCTSTR pszAppName, LPCTSTR pszClassName, BOOL bMatchClass);
	typedef BOOL(WINAPI*pfnIsTopLevelWindow)(HWND hWnd);

	static inline bool IsAllowTransparent()
	{
		DWORD dwResult = 0;
		DWORD dwSize = sizeof(DWORD);
		RegGetValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), TEXT("EnableTransparency"), RRF_RT_REG_DWORD, nullptr, &dwResult, &dwSize);
		return dwResult == 1;
	}

	static inline HRESULT WINAPI GetThemeClass(const HTHEME& hTheme, LPCTSTR pszClassName, const int cchClassName)
	{
		static const auto& GetThemeClass = (pfnGetThemeClass)GetProcAddress(GetModuleHandle(TEXT("Uxtheme")), MAKEINTRESOURCEA(74));
		if (GetThemeClass)
		{
			return GetThemeClass(hTheme, pszClassName, cchClassName);
		}
		else
		{
			return E_POINTER;
		}
	}

	static inline BOOL WINAPI IsThemeClassDefined(const HTHEME& hTheme, LPCTSTR pszAppName, LPCTSTR pszClassName, BOOL bMatchClass)
	{
		static const auto& IsThemeClassDefined = (pfnIsThemeClassDefined)GetProcAddress(GetModuleHandle(TEXT("Uxtheme")), MAKEINTRESOURCEA(50));
		if (IsThemeClassDefined)
		{
			return IsThemeClassDefined(hTheme, pszAppName, pszClassName, bMatchClass);
		}
		else
		{
			return FALSE;
		}
	}

	static inline bool VerifyThemeData(const HTHEME& hTheme, LPCTSTR pszThemeClassName)
	{
		TCHAR pszClassName[MAX_PATH + 1];
		GetThemeClass(hTheme, pszClassName, MAX_PATH);
		return !_wcsicmp(pszClassName, pszThemeClassName);
	}

	static inline BOOL IsTopLevelWindow(const HWND& hWnd)
	{
		static const auto& IsTopLevelWindow = (pfnIsTopLevelWindow)GetProcAddress(GetModuleHandle(TEXT("User32")), "IsTopLevelWindow");
		if (IsTopLevelWindow)
		{
			return IsTopLevelWindow(hWnd);
		}
		return FALSE;
	}

	static inline bool IsValidFlyout(const HWND& hWnd)
	{
		TCHAR szClass[MAX_PATH + 1] = {};
		// 微软内部判断窗口是否是弹出菜单的方法
		if (GetClassLong(hWnd, GCW_ATOM) == 32768)
		{
			return true;
		}
		GetClassName(hWnd, szClass, MAX_PATH);
		return
		    (
		        IsTopLevelWindow(hWnd) and
		        (
		            !_tcscmp(szClass, TEXT("#32768")) or
		            !_tcscmp(szClass, TEXT("ViewControlClass")) or
		            !_tcscmp(szClass, TEXT("DropDown"))
		        )
		    );
	}

	static inline bool IsViewControlClass(const HWND& hWnd)
	{
		TCHAR szClass[MAX_PATH + 1] = {};
		GetClassName(hWnd, szClass, MAX_PATH);
		return !_tcscmp(szClass, TEXT("ViewControlClass"));
	}

	static inline bool IsTooltipClass(const HWND& hWnd)
	{
		TCHAR szClass[MAX_PATH + 1] = {};
		GetClassName(hWnd, szClass, MAX_PATH);
		return !_tcscmp(szClass, TOOLTIPS_CLASS);
	}

	static inline bool IsToolbarWindow(const HWND& hWnd)
	{
		TCHAR szClass[MAX_PATH + 1] = {};
		GetClassName(hWnd, szClass, MAX_PATH);
		return !_tcscmp(szClass, TEXT("ToolbarWindow32"));
	}

	static inline void Clear(HDC hdc, LPCRECT lpRect)
	{
		PatBlt(
		    hdc,
		    lpRect->left,
		    lpRect->top,
		    lpRect->right - lpRect->left,
		    lpRect->bottom - lpRect->top,
		    BLACKNESS
		);
	}

	static inline HBITMAP CreateDIB(HDC hdc, LONG nWidth, LONG nHeight, PVOID* pvBits)
	{
		BITMAPINFO bitmapInfo = {};
		bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biWidth = nWidth;
		bitmapInfo.bmiHeader.biHeight = -nHeight;

		return CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, pvBits, nullptr, 0);
	}

	static inline COLORREF GetBrushColor(const HBRUSH& hBrush)
	{
		LOGBRUSH lbr;
		GetObject(hBrush, sizeof(lbr), &lbr);
		if (lbr.lbStyle != BS_SOLID)
		{
			return CLR_NONE;
		}
		return lbr.lbColor;
	}

	static inline void SetPixel(PBYTE pvBits, BYTE b, BYTE g, BYTE r, BYTE a)
	{
		// Alpha预乘
		pvBits[0] = (b * (a + 1)) >> 8;
		pvBits[1] = (g * (a + 1)) >> 8;
		pvBits[2] = (r * (a + 1)) >> 8;
		pvBits[3] = a;
	}

	static inline void HighlightBox(HDC hdc, LPCRECT lpRect, COLORREF dwColor)
	{
		HPEN hPen = CreatePen(PS_SOLID, 3, dwColor);
		HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
		Rectangle(hdc, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
	}

	static void PrepareAlpha(HBITMAP hBitmap)
	{
		HDC hdc = CreateCompatibleDC(nullptr);
		BITMAPINFO BitmapInfo = {sizeof(BitmapInfo.bmiHeader)};

		if (hdc)
		{
			if (GetDIBits(hdc, hBitmap, 0, 0, nullptr, &BitmapInfo, DIB_RGB_COLORS))
			{
				BYTE *pvBits = new BYTE[BitmapInfo.bmiHeader.biSizeImage];

				BitmapInfo.bmiHeader.biCompression = BI_RGB;
				BitmapInfo.bmiHeader.biBitCount = 32;

				if (pvBits and GetDIBits(hdc, hBitmap, 0, BitmapInfo.bmiHeader.biHeight, (LPVOID)pvBits, &BitmapInfo, DIB_RGB_COLORS))
				{
					bool bHasAlpha = false;

					for (UINT i = 0; i < BitmapInfo.bmiHeader.biSizeImage; i += 4)
					{
						if (pvBits[i + 3] != 0)
						{
							bHasAlpha = true;
							break;
						}
					}

					if (!bHasAlpha)
					{
						for (UINT i = 0; i < BitmapInfo.bmiHeader.biSizeImage; i += 4)
						{
							pvBits[i] = (pvBits[i] * 256) >> 8;
							pvBits[i + 1] = (pvBits[i + 1] * 256) >> 8;
							pvBits[i + 2] = (pvBits[i + 2] * 256) >> 8;
							pvBits[i + 3] = 255;
						}
					}

					SetDIBits(hdc, hBitmap, 0, BitmapInfo.bmiHeader.biHeight, pvBits, &BitmapInfo, DIB_RGB_COLORS);
				}
				delete[] pvBits;
			}
			DeleteDC(hdc);
		}
	}
};