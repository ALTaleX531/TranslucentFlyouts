#include "pch.h"
#include "tflapi.h"
#include "SettingsHelper.h"
#include "DebugHelper.h"
#include "ThemeHelper.h"
#include "AcrylicHelper.h"
#include "TranslucentFlyoutsLib.h"

using namespace TranslucentFlyoutsLib;

// 透明化处理
DetoursHook DrawThemeBackgroundHook("Uxtheme", "DrawThemeBackground", MyDrawThemeBackground);
// 文字渲染
DetoursHook DrawThemeTextExHook("Uxtheme", "DrawThemeTextEx", MyDrawThemeTextEx);
DetoursHook DrawThemeTextHook("Uxtheme", "DrawThemeText", MyDrawThemeText);
DetoursHook DrawTextWHook("User32", "DrawTextW", MyDrawTextW);
DetoursHook DrawTextExWHook("User32", "DrawTextExW", MyDrawTextExW);
DetoursHook ExtTextOutWHook("Gdi32", "ExtTextOutW", MyExtTextOutW);
// 图标修复
DetoursHook SetMenuInfoHook("User32", "SetMenuInfo", MySetMenuInfo);
DetoursHook SetMenuItemBitmapsHook("User32", "SetMenuItemBitmaps", MySetMenuItemBitmaps);
DetoursHook InsertMenuItemWHook("User32", "InsertMenuItemW", MyInsertMenuItemW);
DetoursHook SetMenuItemInfoWHook("User32", "SetMenuItemInfoW", MySetMenuItemInfoW);
// 窗口句柄的记录
DetoursHook CreateCompatibleDCHook("Gdi32", "CreateCompatibleDC", MyCreateCompatibleDC);
DetoursHook DeleteDCHook("Gdi32", "DeleteDC", MyDeleteDC);
DetoursHook DeleteObjectHook("Gdi32", "DeleteObject", MyDeleteObject);

thread_local HWND g_hWnd = nullptr;
#pragma data_seg("shared")
Settings g_settings = {};
#pragma data_seg()
#pragma comment(linker,"/SECTION:shared,RWS")

void SetCurrentMenuFlyout(HWND hWnd)
{
	g_hWnd = hWnd;
}

HWND GetCurrentMenuFlyout()
{
	return g_hWnd;
}

void TranslucentFlyoutsLib::Startup()
{
	Detours::Begin();
	Detours::Batch(
	    DrawThemeBackgroundHook,
	    DrawThemeTextExHook,
	    DrawThemeTextHook,
	    DrawTextWHook,
	    DrawTextExWHook,
	    ExtTextOutWHook,
	    SetMenuInfoHook,
	    SetMenuItemBitmapsHook,
	    InsertMenuItemWHook,
	    SetMenuItemInfoWHook,
	    CreateCompatibleDCHook,
	    DeleteDCHook,
	    DeleteObjectHook
	);
	Detours::Commit();
}

void TranslucentFlyoutsLib::Shutdown()
{
	Detours::Begin();
	Detours::Batch(
	    DrawThemeBackgroundHook,
	    DrawThemeTextExHook,
	    DrawThemeTextHook,
	    DrawTextWHook,
	    DrawTextExWHook,
	    ExtTextOutWHook,
	    SetMenuInfoHook,
	    SetMenuItemBitmapsHook,
	    InsertMenuItemWHook,
	    SetMenuItemInfoWHook,
	    CreateCompatibleDCHook,
	    DeleteDCHook,
	    DeleteObjectHook
	);
	Detours::Commit();
	ClearDeviceContextList();
}

HRESULT WINAPI TranslucentFlyoutsLib::MyDrawThemeBackground(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCRECT pRect,
    LPCRECT pClipRect
)
{
	// 手动确认是否是透明的
	auto VerifyThemeBackgroundTransparency = [&](HTHEME hTheme, int iPartId, int iStateId)
	{
		RECT Rect = {0, 0, 1, 1};
		bool bResult = false;

		auto verify = [&](int y, int x, RGBQUAD * pRGBAInfo)
		{
			if (pRGBAInfo->rgbReserved != 0xFF)
			{
				bResult = true;
				return false;
			}
			return true;
		};

		auto f = [&](HDC hMemDC, HPAINTBUFFER hPaintBuffer)
		{
			HRESULT hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
			                 hTheme,
			                 hMemDC,
			                 iPartId,
			                 iStateId,
			                 &Rect,
			                 nullptr
			             );
			GdiFlush();

			if (SUCCEEDED(hr))
			{
				BufferedPaintWalkBits(hPaintBuffer, verify);
			}
		};

		DoBufferedPaint(hdc, &Rect, f, 0xFF, BPPF_ERASE | BPPF_NOCLIP, FALSE);

		return bResult;
	};

	HRESULT hr = S_OK;

	RECT Rect = *pRect;
	if (pClipRect)
	{
		::IntersectRect(&Rect, pRect, pClipRect);
	}

	auto f = [&](HDC hMemDC, HPAINTBUFFER hPaintBuffer)
	{
		Clear(hdc, &Rect);

		hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
		         hTheme,
		         hMemDC,
		         iPartId,
		         iStateId,
		         pRect,
		         pClipRect
		     );

		GdiFlush();
		BufferedPaintSetAlpha(hPaintBuffer, &Rect, 0xFF);
	};

	// 工具提示
	if (
	    IsAllowTransparent() and
	    VerifyThemeData(hTheme, TEXT("Tooltip")) and
	    (g_settings.GetPolicy() & Tooltip) and
	    (
	        iPartId == TTP_STANDARD or
	        iPartId == TTP_BALLOON or
	        iPartId == TTP_BALLOONSTEM
	    )
	)
	{
		if (
		    !IsThemeBackgroundPartiallyTransparent(hTheme, iPartId, iStateId) or
		    !VerifyThemeBackgroundTransparency(hTheme, iPartId, iStateId)
		)
		{
			if (!DoBufferedPaint(hdc, &Rect, f, (BYTE)g_settings.GetOpacity()))
			{
				goto Default;
			}
		}
		else
		{
			goto Default;
		}
	}
	// 视图控制
	else if
	(
	    IsAllowTransparent() and
	    VerifyThemeData(hTheme, TEXT("Toolbar")) and
	    (iPartId == 0 and iStateId == 0) and
	    (
	        (g_settings.GetPolicy() & ViewControl) and
	        (
	            IsViewControlFlyout(GetParent(GetWindowFromHDC(hdc))) or
	            !IsWindow(GetWindowFromHDC(hdc))
	        )
	    )
	)
	{
		if (!DoBufferedPaint(hdc, &Rect, f, (BYTE)g_settings.GetOpacity()))
		{
			goto Default;
		}
	}
	// 弹出菜单
	else if
	(
	    IsAllowTransparent() and
	    (
	        VerifyThemeData(hTheme, TEXT("Menu")) and
	        (
	            (g_settings.GetPolicy() & PopupMenu) and
	            !IsViewControlFlyout(GetWindowFromHDC(hdc)) and
	            !IsViewControlFlyout(GetParent(GetWindowFromHDC(hdc)))
	        ) or
	        (
	            (g_settings.GetPolicy() & ViewControl) and
	            (
	                IsViewControlFlyout(GetWindowFromHDC(hdc)) or
	                IsViewControlFlyout(GetParent(GetWindowFromHDC(hdc)))
	            )
	        )
	    )
	)
	{
		if (IsWindow(GetCurrentMenuFlyout()))
		{
			if (iPartId != MENU_POPUPBACKGROUND)
			{
				SetWindowEffect(
				    GetCurrentMenuFlyout(),
				    g_settings.GetEffect(),
				    g_settings.GetBorder()
				);
				SetCurrentMenuFlyout(nullptr);
			}
		}

		if (
		    iPartId == MENU_POPUPBACKGROUND or
		    iPartId == MENU_POPUPGUTTER or
		    (
		        g_settings.GetColorizeOption() == 0 ?
		        (iPartId == MENU_POPUPITEM and iStateId != MPI_HOT) :
		        (iPartId == MENU_POPUPITEM)
		    ) or
		    iPartId == MENU_POPUPBORDERS
		)
		{
			if (
			    iPartId == MENU_POPUPBACKGROUND or
			    (
			        !IsThemeBackgroundPartiallyTransparent(hTheme, iPartId, iStateId) or
			        !VerifyThemeBackgroundTransparency(hTheme, iPartId, iStateId)
			    )
			)
			{
				if (!DoBufferedPaint(hdc, &Rect, f, (BYTE)g_settings.GetOpacity(), BPPF_ERASE | (iPartId == MENU_POPUPBORDERS ? BPPF_NONCLIENT : 0UL)))
				{
					goto Default;
				}
			}
			else
			{
				MyDrawThemeBackground(
				    hTheme,
				    hdc,
				    MENU_POPUPBACKGROUND,
				    0,
				    pRect,
				    pClipRect
				);
				if (iPartId != MENU_POPUPITEM or (iPartId == MENU_POPUPITEM and iStateId != MPI_NORMAL))
				{
					MyDrawThemeBackground(
					    hTheme,
					    hdc,
					    MENU_POPUPITEM,
					    MPI_NORMAL,
					    pRect,
					    pClipRect
					);
				}
				goto Default;
			}
		}
		else
		{
			goto Default;
		}
	}
	else
	{
		goto Default;
	}
	return hr;
Default:
	hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
	         hTheme,
	         hdc,
	         iPartId,
	         iStateId,
	         pRect,
	         pClipRect
	     );
	return hr;
}

HRESULT WINAPI TranslucentFlyoutsLib::MyDrawThemeTextEx(
    HTHEME        hTheme,
    HDC           hdc,
    int           iPartId,
    int           iStateId,
    LPCTSTR       pszText,
    int           cchText,
    DWORD         dwTextFlags,
    LPRECT        pRect,
    const DTTOPTS *pOptions
)
{
	// pOptions可以为NULL，使用NULL时与DrawThemeText效果无异
	HRESULT hr = S_OK;

	if (
	    IsAllowTransparent() and
	    (
	        (VerifyThemeData(hTheme, TEXT("Menu")) and g_settings.GetPolicy() & PopupMenu) or
	        (VerifyThemeData(hTheme, TEXT("Toolbar")) and g_settings.GetPolicy() & ViewControl) or
	        (VerifyThemeData(hTheme, TEXT("Tooltip")) and g_settings.GetPolicy() & Tooltip)
	    ) and
	    pOptions and
	    (
	        !pOptions or
	        (
	            !(pOptions->dwFlags & DTT_CALCRECT) and
	            !(pOptions->dwFlags & DTT_COMPOSITED)
	        )
	    )
	)
	{
		DTTOPTS Options = *pOptions;
		Options.dwFlags |= DTT_COMPOSITED;

		auto f = [&](HDC hMemDC, HPAINTBUFFER hPaintBuffer)
		{
			hr = DrawThemeTextExHook.OldFunction<decltype(MyDrawThemeTextEx)>(
			         hTheme,
			         hMemDC,
			         iPartId,
			         iStateId,
			         pszText,
			         cchText,
			         dwTextFlags,
			         pRect,
			         &Options
			     );

			GdiFlush();

		};

		if (!DoBufferedPaint(hdc, pRect, f))
		{
			goto Default;
		}

		return hr;
	}
	else
	{
		goto Default;
	}
	return hr;
Default:
	hr = DrawThemeTextExHook.OldFunction<decltype(MyDrawThemeTextEx)>(
	         hTheme,
	         hdc,
	         iPartId,
	         iStateId,
	         pszText,
	         cchText,
	         dwTextFlags,
	         pRect,
	         pOptions
	     );
	return hr;
}

HRESULT WINAPI TranslucentFlyoutsLib::MyDrawThemeText(
    HTHEME  hTheme,
    HDC     hdc,
    int     iPartId,
    int     iStateId,
    LPCTSTR pszText,
    int     cchText,
    DWORD   dwTextFlags,
    DWORD   dwTextFlags2,
    LPCRECT pRect
)
{
	// dwTextFlags 不支持DT_CALCRECT
	// dwTextFlags2 从未被使用，始终为0
	HRESULT hr = S_OK;

	if (
	    IsAllowTransparent() and
	    pRect and
	    (
	        (VerifyThemeData(hTheme, TEXT("Menu")) and g_settings.GetPolicy() & PopupMenu) or
	        (VerifyThemeData(hTheme, TEXT("Toolbar")) and g_settings.GetPolicy() & ViewControl) or
	        (VerifyThemeData(hTheme, TEXT("Tooltip")) and g_settings.GetPolicy() & Tooltip)
	    )
	)
	{
		DTTOPTS Options = {sizeof(DTTOPTS)};
		RECT Rect = *pRect;
		hr = DrawThemeTextEx(
		         hTheme,
		         hdc,
		         iPartId,
		         iStateId,
		         pszText,
		         cchText,
		         dwTextFlags,
		         &Rect,
		         &Options
		     );
	}
	else
	{
		hr = DrawThemeTextHook.OldFunction<decltype(MyDrawThemeText)>(
		         hTheme,
		         hdc,
		         iPartId,
		         iStateId,
		         pszText,
		         cchText,
		         dwTextFlags,
		         dwTextFlags2,
		         pRect
		     );
	}

	return hr;
}

int WINAPI TranslucentFlyoutsLib::MyDrawTextW(
    HDC     hdc,
    LPCTSTR lpchText,
    int     cchText,
    LPRECT  lprc,
    UINT    format
)
{
	int nResult = 0;
	thread_local int nLastResult = 0;

	if (
	    !IsAllowTransparent() or
	    GetBkMode(hdc) != TRANSPARENT or
	    (VerifyCaller(TEXT("Uxtheme")) or
	     (format & DT_CALCRECT)) or
	    !IsEnableTextRenderRedirection()
	)
	{
		nResult =
		    DrawTextWHook.OldFunction<decltype(MyDrawTextW)>(
		        hdc,
		        lpchText,
		        cchText,
		        lprc,
		        format
		    );
	}
	else
	{
		DTTOPTS Options = {sizeof(DTTOPTS)};
		HTHEME hTheme = OpenThemeData(nullptr, !VerifyWindowClass(GetWindowFromHDC(hdc), TOOLTIPS_CLASS, TRUE) ? TEXT("Menu") : TEXT("Tooltip"));
		Options.dwFlags = DTT_TEXTCOLOR;
		Options.crText = GetTextColor(hdc);

		if (hTheme)
		{
			DrawThemeTextEx(hTheme, hdc, 0, 0, lpchText, cchText, format, lprc, &Options);
			CloseThemeData(hTheme);
		}

		nResult = nLastResult;
	}

	return nResult;
}

int WINAPI TranslucentFlyoutsLib::MyDrawTextExW(
    HDC              hdc,
    LPWSTR           lpchText,
    int              cchText,
    LPRECT           lprc,
    UINT             format,
    LPDRAWTEXTPARAMS lpdtp
)
{
	int nResult = 0;
	thread_local int nLastResult = 0;

	if (
	    !IsAllowTransparent() or
	    GetBkMode(hdc) != TRANSPARENT or
	    (VerifyCaller(TEXT("Uxtheme")) or
	     (format & DT_CALCRECT)) or
	    !IsEnableTextRenderRedirection()
	)
	{
		nResult =
		    DrawTextExWHook.OldFunction<decltype(MyDrawTextExW)>(
		        hdc,
		        lpchText,
		        cchText,
		        lprc,
		        format,
		        lpdtp
		    );
	}
	else
	{
		DTTOPTS Options = {sizeof(DTTOPTS)};
		HTHEME hTheme = OpenThemeData(nullptr, TEXT("Menu"));
		Options.dwFlags = DTT_TEXTCOLOR;
		Options.crText = GetTextColor(hdc);

		if (hTheme)
		{
			DrawThemeTextEx(hTheme, hdc, 0, 0, lpchText, cchText, format, lprc, &Options);
			CloseThemeData(hTheme);
		}

		nResult = nLastResult;
	}

	return nResult;
}

BOOL WINAPI TranslucentFlyoutsLib::MyExtTextOutW(
    HDC        hdc,
    int        x,
    int        y,
    UINT       options,
    const RECT *lprect,
    LPCWSTR    lpString,
    UINT       c,
    const INT  *lpDx
)
{
	BOOL bResult = FALSE;

	if (
	    lpDx == nullptr and
	    !(options | ETO_GLYPH_INDEX) and
	    IsAllowTransparent() and
	    IsEnableTextRenderRedirection() and
	    (
	        !VerifyCaller(TEXT("Gdi32")) and
	        !VerifyCaller(TEXT("Gdi32full"))
	    )
	)
	{
		RECT Rect = {};
		if ((options | ETO_OPAQUE or options | ETO_CLIPPED) and lprect)
		{
			Rect = *lprect;
		}
		else
		{
			DrawText(
			    hdc,
			    lpString,
			    c,
			    &Rect,
			    DT_LEFT | DT_TOP | DT_SINGLELINE | DT_CALCRECT
			);
		}

		DTTOPTS Options = {sizeof(DTTOPTS)};
		HTHEME hTheme = OpenThemeData(nullptr, TEXT("Menu"));
		Options.dwFlags = DTT_TEXTCOLOR;
		Options.crText = GetTextColor(hdc);

		if (hTheme)
		{
			DrawThemeTextEx(hTheme, hdc, 0, 0, lpString, c, DT_LEFT | DT_TOP | DT_SINGLELINE, &Rect, &Options);
			CloseThemeData(hTheme);
		}
	}
	else
	{
		goto Default;
	}
	return bResult;
Default:
	bResult = ExtTextOutWHook.OldFunction<decltype(MyExtTextOutW)>(
	              hdc,
	              x,
	              y,
	              options,
	              lprect,
	              lpString,
	              c,
	              lpDx
	          );
	return bResult;
}

BOOL WINAPI TranslucentFlyoutsLib::MySetMenuInfo(
    HMENU hMenu,
    LPCMENUINFO lpMenuInfo
)
{
	BOOL bResult = TRUE;
	// 上次留下的画刷
	thread_local HBRUSH hBrush = nullptr;

	if (
	    (lpMenuInfo->fMask & MIM_BACKGROUND) and
	    lpMenuInfo->hbrBack and
	    IsAllowTransparent() and
	    (g_settings.GetPolicy() & PopupMenu)
	)
	{
		PBYTE pvBits = nullptr;
		MENUINFO MenuInfo = *lpMenuInfo;
		HBITMAP hBitmap = CreateDIB(nullptr, 1, 1, (PVOID*)&pvBits);
		if (hBitmap and pvBits)
		{
			COLORREF dwColor = GetBrushColor(lpMenuInfo->hbrBack);

			// 获取提供的画刷颜色，设置位图画刷的像素
			if (dwColor != CLR_NONE)
			{
				SetPixel(
				    pvBits,
				    GetBValue(dwColor),
				    GetGValue(dwColor),
				    GetRValue(dwColor),
				    (BYTE)g_settings.GetOpacity()
				);
				// 创建位图画刷
				// 只有位图画刷才有Alpha值

				/// 需要注意
				hBrush = CreatePatternBrush(hBitmap);
			}

			DeleteObject(hBitmap);

			if (hBrush)
			{
				// 此画刷会被内核自动释放
				MenuInfo.hbrBack = hBrush;
				// 我们替换了调用者提供的画刷，要帮它释放
				DeleteObject(lpMenuInfo->hbrBack);
			}
			else
			{
				goto Default;
			}

			bResult = SetMenuInfoHook.OldFunction<decltype(MySetMenuInfo)>(
			              hMenu,
			              &MenuInfo
			          );
		}
		else
		{
			goto Default;
		}
	}
	else
	{
		goto Default;
	}

	return bResult;
Default:
	bResult = SetMenuInfoHook.OldFunction<decltype(MySetMenuInfo)>(
	              hMenu,
	              lpMenuInfo
	          );
	return bResult;
}

BOOL WINAPI TranslucentFlyoutsLib::MySetMenuItemBitmaps(
    HMENU   hMenu,
    UINT    uPosition,
    UINT    uFlags,
    HBITMAP hBitmapUnchecked,
    HBITMAP hBitmapChecked
)
{
	BOOL bResult = FALSE;

	if (IsAllowTransparent() and g_settings.GetPolicy() & PopupMenu)
	{
		PrepareAlpha(hBitmapUnchecked);
		PrepareAlpha(hBitmapChecked);
	}

	bResult = SetMenuItemBitmapsHook.OldFunction<decltype(MySetMenuItemBitmaps)>(
	              hMenu,
	              uPosition,
	              uFlags,
	              hBitmapUnchecked,
	              hBitmapChecked
	          );
	return bResult;
}

BOOL WINAPI TranslucentFlyoutsLib::MyInsertMenuItemW(
    HMENU            hMenu,
    UINT             item,
    BOOL             fByPosition,
    LPCMENUITEMINFOW lpmii
)
{
	BOOL bResult = FALSE;

	if (IsAllowTransparent() and lpmii and (lpmii->fMask & MIIM_CHECKMARKS or lpmii->fMask & MIIM_BITMAP) and g_settings.GetPolicy() & PopupMenu)
	{
		PrepareAlpha(lpmii->hbmpItem);
		PrepareAlpha(lpmii->hbmpUnchecked);
		PrepareAlpha(lpmii->hbmpChecked);
	}
	bResult = InsertMenuItemWHook.OldFunction<decltype(MyInsertMenuItemW)>(
	              hMenu,
	              item,
	              fByPosition,
	              lpmii
	          );
	return bResult;
}

BOOL WINAPI TranslucentFlyoutsLib::MySetMenuItemInfoW(
    HMENU            hMenu,
    UINT             item,
    BOOL             fByPositon,
    LPCMENUITEMINFOW lpmii
)
{
	BOOL bResult = FALSE;

	if (IsAllowTransparent() and lpmii and (lpmii->fMask & MIIM_CHECKMARKS or lpmii->fMask & MIIM_BITMAP) and g_settings.GetPolicy() & PopupMenu)
	{
		PrepareAlpha(lpmii->hbmpItem);
		PrepareAlpha(lpmii->hbmpUnchecked);
		PrepareAlpha(lpmii->hbmpChecked);
	}
	bResult = SetMenuItemInfoWHook.OldFunction<decltype(MySetMenuItemInfoW)>(
	              hMenu,
	              item,
	              fByPositon,
	              lpmii
	          );

	return bResult;
}

HDC WINAPI TranslucentFlyoutsLib::MyCreateCompatibleDC(HDC hdc)
{
	HDC hMemDC = CreateCompatibleDCHook.OldFunction<decltype(MyCreateCompatibleDC)>(
	                 hdc
	             );
	if (hMemDC)
	{
		AssociateMemoryDC(hdc, hMemDC);
	}
	return hMemDC;
}

BOOL WINAPI TranslucentFlyoutsLib::MyDeleteDC(HDC hdc)
{
	BOOL bResult = DeleteDCHook.OldFunction<decltype(MyDeleteDC)>(
	                   hdc
	               );
	if (bResult)
	{
		DisassociateMemoryDC(hdc);
	}
	return bResult;
}

BOOL WINAPI TranslucentFlyoutsLib::MyDeleteObject(HGDIOBJ ho)
{
	BOOL bResult = DeleteObjectHook.OldFunction<decltype(MyDeleteObject)>(
	                   ho
	               );
	if (bResult)
	{
		if (GetObjectType(ho) == OBJ_MEMDC)
		{
			DisassociateMemoryDC((HDC)ho);
		}
	}
	return bResult;
}

LRESULT CALLBACK TranslucentFlyoutsLib::SubclassProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (Message)
	{
		case WM_NOTIFY:
		{
			//if (((LPNMHDR)lParam)->code == NM_CUSTOMDRAW and ThemeHelper::IsToolbarWindow(((LPNMHDR)lParam)->hwndFrom))
			{
				LPNMCUSTOMDRAW lpNMCustomDraw = (LPNMCUSTOMDRAW)lParam;
				/*OutputDebugString(L"NM_CUSTOMDRAW");
				{
					TCHAR p[261];
					swprintf_s(p, L"lpNMCustomDraw->dwDrawStage: %d", lpNMCustomDraw->dwDrawStage);
					OutputDebugString(p);
				}*/
				/*if (lpNMCustomDraw->dwDrawStage == CDDS_PREPAINT)
				{
					HTHEME hTheme = OpenThemeData(hWnd, TEXT("Menu"));
					if (hTheme)
					{
						MyDrawThemeBackground(
							hTheme,
							lpNMCustomDraw->hdc,
							MENU_POPUPBACKGROUND,
							0,
							&lpNMCustomDraw->rc,
							nullptr
						);
						CloseThemeData(hTheme);
					}
					return TBCDRF_NOBACKGROUND;
				}*/
			}
		}
		default:
			return DefSubclassProc(hWnd, Message, wParam, lParam);
	}
	return 0;
}

void TranslucentFlyoutsLib::OnWindowsCreated(HWND hWnd)
{
	if (IsAllowTransparent())
	{
		if (IsPopupMenuFlyout(hWnd) and g_settings.GetPolicy() & PopupMenu)
		{
			SetCurrentMenuFlyout(hWnd);
		}
		if (IsViewControlFlyout(hWnd) and g_settings.GetPolicy() & ViewControl)
		{
			SetWindowEffect(
			    hWnd,
			    g_settings.GetEffect(),
			    g_settings.GetBorder()
			);
		}
	}
}

void TranslucentFlyoutsLib::OnWindowsDestroyed(HWND hWnd)
{

}

void TranslucentFlyoutsLib::OnWindowShowed(HWND hWnd)
{
	if (IsAllowTransparent())
	{
		if (IsTooltipFlyout(hWnd) and g_settings.GetPolicy() & Tooltip)
		{
			SetWindowEffect(
			    hWnd,
			    g_settings.GetEffect(),
			    g_settings.GetBorder()
			);
		}
	}
}

void CALLBACK TranslucentFlyoutsLib::HandleWinEvent(
    HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
    LONG idObject, LONG idChild,
    DWORD dwEventThread, DWORD dwmsEventTime
)
{
	if (hWnd and IsWindow(hWnd))
	{
		if (dwEvent == EVENT_OBJECT_CREATE)
		{
			OnWindowsCreated(hWnd);
		}

		if (dwEvent == EVENT_OBJECT_DESTROY)
		{
			OnWindowsDestroyed(hWnd);
		}

		if (dwEvent == EVENT_OBJECT_SHOW)
		{
			OnWindowShowed(hWnd);
		}
	}
}