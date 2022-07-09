#pragma once
#include "pch.h"

namespace TranslucentFlyoutsLib
{
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

	static inline HRESULT WINAPI GetThemeClass(HTHEME hTheme, LPCTSTR pszClassName, const int cchClassName)
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

	static inline BOOL WINAPI IsThemeClassDefined(HTHEME hTheme, LPCTSTR pszAppName, LPCTSTR pszClassName, BOOL bMatchClass)
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

	static inline bool VerifyThemeData(HTHEME hTheme, LPCTSTR pszThemeClassName)
	{
		TCHAR pszClassName[MAX_PATH + 1];
		GetThemeClass(hTheme, pszClassName, MAX_PATH);
		return !_wcsicmp(pszClassName, pszThemeClassName);
	}

	static inline BOOL IsTopLevelWindow(HWND hWnd)
	{
		static const auto& IsTopLevelWindow = (pfnIsTopLevelWindow)GetProcAddress(GetModuleHandle(TEXT("User32")), "IsTopLevelWindow");
		if (IsTopLevelWindow)
		{
			return IsTopLevelWindow(hWnd);
		}
		return FALSE;
	}

	static inline bool VerifyWindowClass(HWND hWnd, LPCTSTR pszClassName, BOOL bRequireTopLevel = FALSE)
	{
		TCHAR pszClass[MAX_PATH + 1] = {};
		GetClassName(hWnd, pszClass, MAX_PATH);
		return (!_tcscmp(pszClass, pszClassName) and (bRequireTopLevel ? IsTopLevelWindow(hWnd) : TRUE));
	}

	static inline bool IsPopupMenuFlyout(HWND hWnd)
	{
		TCHAR pszClass[MAX_PATH + 1] = {};
		// ΢���ڲ��жϴ����Ƿ��ǵ����˵��ķ���
		if (GetClassLong(hWnd, GCW_ATOM) == 32768)
		{
			return true;
		}
		GetClassName(hWnd, pszClass, MAX_PATH);
		return
		    (
		        IsTopLevelWindow(hWnd) and
		        (
		            !_tcscmp(pszClass, TEXT("#32768")) or
		            !_tcscmp(pszClass, TEXT("DropDown"))
		        )
		    );
	}

	static inline bool IsViewControlFlyout(HWND hWnd)
	{
		return VerifyWindowClass(hWnd, TEXT("ViewControlClass"), TRUE);
	}

	static inline bool IsTooltipFlyout(HWND hWnd)
	{
		return VerifyWindowClass(hWnd, TOOLTIPS_CLASS, TRUE);
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

	static inline COLORREF GetBrushColor(HBRUSH hBrush)
	{
		LOGBRUSH lbr = {};
		GetObject(hBrush, sizeof(lbr), &lbr);
		if (lbr.lbStyle != BS_SOLID)
		{
			return CLR_NONE;
		}
		return lbr.lbColor;
	}

	static inline int GetBrushType(HBRUSH hBrush)
	{
		LOGBRUSH lbr = {};
		GetObject(hBrush, sizeof(lbr), &lbr);
		return lbr.lbStyle;
	}

	static inline void SetPixel(PBYTE pvBits, BYTE b, BYTE g, BYTE r, BYTE a)
	{
		// AlphaԤ��
		pvBits[0] = (b * (a + 1)) >> 8;
		pvBits[1] = (g * (a + 1)) >> 8;
		pvBits[2] = (r * (a + 1)) >> 8;
		pvBits[3] = a;
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

	template <typename T>
	static BOOL DoBufferedPaint(
	    HDC hdc,
	    LPCRECT Rect,
	    T&& t,
	    BYTE dwOpacity = 255,
	    DWORD dwFlag = BPPF_ERASE,
	    BOOL bUpdateTarget = TRUE
	)
	{
		HDC hMemDC = nullptr;
		BLENDFUNCTION BlendFunction = {AC_SRC_OVER, 0, dwOpacity, AC_SRC_ALPHA};
		BP_PAINTPARAMS PaintParams = {sizeof(BP_PAINTPARAMS), dwFlag, nullptr, &BlendFunction};
		HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdc, Rect, BPBF_TOPDOWNDIB, &PaintParams, &hMemDC);
		if (hPaintBuffer and hMemDC)
		{
			SetLayout(hMemDC, GetLayout(hdc));
			SelectObject(hMemDC, GetCurrentObject(hdc, OBJ_FONT));
			SelectObject(hMemDC, GetCurrentObject(hdc, OBJ_BRUSH));
			SelectObject(hMemDC, GetCurrentObject(hdc, OBJ_PEN));
			SetTextAlign(hMemDC, GetTextAlign(hdc));

			t(hMemDC, hPaintBuffer);
			EndBufferedPaint(hPaintBuffer, TRUE);
		}
		else
		{
			return FALSE;
		}
		return TRUE;
	}

	// �˺���Ӧ����EndBufferedPaint֮ǰ����
	template <typename T>
	static BOOL BufferedPaintWalkBits(
	    HPAINTBUFFER hPaintBuffer,
	    T&& t
	)
	{
		int cxRow = 0;
		RGBQUAD* pbBuffer = nullptr;
		RECT targetRect = {};
		if (SUCCEEDED(GetBufferedPaintTargetRect(hPaintBuffer, &targetRect)))
		{
			int cx = targetRect.right - targetRect.left;
			int cy = targetRect.bottom - targetRect.top;
			if (SUCCEEDED(GetBufferedPaintBits(hPaintBuffer, &pbBuffer, &cxRow)))
			{
				for (int y = 0; y < cy; y++)
				{
					for (int x = 0; x < cx; x++)
					{
						RGBQUAD *pRGBAInfo = &pbBuffer[y * cxRow + x];
						if (!t(y, x, pRGBAInfo))
						{
							break;
						}
					}
				}
				return TRUE;
			}
		}
		return FALSE;
	}

	// Special thanks to MapleSpe!
	// ÿһ���̶߳���һ���豸�����Ķ��ձ���hMemDC -> hWindowDC��hMemDC
	static std::unordered_map<DWORD, std::unordered_map<HDC, HDC>> g_deviceContextList;
	static Microsoft::WRL::Wrappers::SRWLock g_SRWdcLock;
	//
	static void ClearDeviceContextList()
	{
		auto lock = g_SRWdcLock.LockExclusive();
		g_deviceContextList.clear();
	}
	// �˺������Ի�ȡ�󲿷�HDC�����Ĵ��ھ����������������絯���˵�����ʱ���ȡʧ��
	// ���ʱ��һ����WM_ERASEBKGND�ṩ���ڴ�DC
	// ��BeginPaint����
	static HWND GetWindowFromHDC(HDC hMemDC)
	{
		HWND hwnd = WindowFromDC(hMemDC);
		auto it = g_deviceContextList.find(GetCurrentThreadId());
		auto lock = g_SRWdcLock.LockShared();
		// ��ȡ��ǰ�̵߳��豸�����Ķ��ձ�
		if (it != g_deviceContextList.end() and (!hwnd and !IsWindow(hwnd)))
		{
			// Ѱ�ұ�־
			bool bfirstTime = true;
			// �豸�����Ķ��ձ�
			auto item = it->second.find(hMemDC);
			// ��һ���ҵ��� �豸�����Ķ��ձ�
			auto lastItem = item;
			do
			{
				// �������һ���������жϣ������ɹ�
				if (item != it->second.end())
				{
					// ��һ�������ɹ�
					if (bfirstTime)
					{
						bfirstTime = false;
					}
					// �洢��һ�εĽ��
					lastItem = item;
				}
				else
				{
					// ��ԭΪ��һ�εĽ��
					item = lastItem;
					break;
				}
				// item->second ���Ǵ���DC���ڴ�DC
				// ���������������һ��
				item = it->second.find(item->second);
			}
			while (true);

			if (!bfirstTime)
			{
				// ���豸�����ģ����־Ϳ���ֱ���õ����ھ��
				hwnd = WindowFromDC(item->second);
			}
			// else
			// �Ҳ�����������豸�����ģ��п�����δ��¼����
		}
		return hwnd;
	}

	static void AssociateMemoryDC(HDC hdc, HDC hMemDC)
	{
		auto lock = g_SRWdcLock.LockExclusive();
		g_deviceContextList[GetCurrentThreadId()][hMemDC] = hdc;
	}
	static void DisassociateMemoryDC(HDC hMemDC)
	{
		auto it = g_deviceContextList.find(GetCurrentThreadId());
		auto lock = g_SRWdcLock.LockExclusive();
		// ��ȡ��ǰ�̵߳��豸�����Ķ��ձ�
		if (it != g_deviceContextList.end())
		{
			// �ڶ���Ϊ�豸�����Ķ��ձ�
			auto item = it->second.find(hMemDC);
			if (item != it->second.end())
			{
				// item->second ���Ǵ���DC���ڴ�DC
				if (item->first == hMemDC)
				{
					it->second.erase(item);
				}
			}
		}
	}
};