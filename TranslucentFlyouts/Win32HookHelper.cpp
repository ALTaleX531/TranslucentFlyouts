#include "pch.h"
#include "tflapi.h"
#include "DebugHelper.h"
#include "ThemeHelper.h"
#include "DetoursHelper.h"
#include "AcrylicHelper.h"
#include "Win32HookHelper.h"

using namespace TranslucentFlyoutsLib;
extern HMODULE g_hModule;

thread_local HWND g_hWnd = nullptr;
thread_local bool g_alphaFixedState = false;

// 透明化处理
DetoursHook TranslucentFlyoutsLib::DrawThemeBackgroundHook("Uxtheme", "DrawThemeBackground", MyDrawThemeBackground);
// 文字渲染
DetoursHook TranslucentFlyoutsLib::DrawThemeTextExHook("Uxtheme", "DrawThemeTextEx", MyDrawThemeTextEx);
DetoursHook TranslucentFlyoutsLib::DrawThemeTextHook("Uxtheme", "DrawThemeText", MyDrawThemeText);
DetoursHook TranslucentFlyoutsLib::DrawTextWHook("User32", "DrawTextW", MyDrawTextW);
DetoursHook TranslucentFlyoutsLib::DrawTextExWHook("User32", "DrawTextExW", MyDrawTextExW);
// 图标修复
DetoursHook TranslucentFlyoutsLib::SetMenuInfoHook("User32", "SetMenuInfo", MySetMenuInfo);
DetoursHook TranslucentFlyoutsLib::SetMenuItemBitmapsHook("User32", "SetMenuItemBitmaps", MySetMenuItemBitmaps);
DetoursHook TranslucentFlyoutsLib::InsertMenuItemWHook("User32", "InsertMenuItemW", MyInsertMenuItemW);
DetoursHook TranslucentFlyoutsLib::SetMenuItemInfoWHook("User32", "SetMenuItemInfoW", MySetMenuItemInfoW);
DetoursHook TranslucentFlyoutsLib::ModifyMenuWHook("User32", "ModifyMenuW", MyModifyMenuW);
// 窗口句柄的记录
DetoursHook TranslucentFlyoutsLib::CreateCompatibleDCHook("Gdi32", "CreateCompatibleDC", MyCreateCompatibleDC);
DetoursHook TranslucentFlyoutsLib::DeleteDCHook("Gdi32", "DeleteDC", MyDeleteDC);
DetoursHook TranslucentFlyoutsLib::DeleteObjectHook("Gdi32", "DeleteObject", MyDeleteObject);

void TranslucentFlyoutsLib::Win32HookStartup()
{
	Detours::Batch(
	    TRUE,
	    DrawThemeBackgroundHook,
	    //
	    DrawThemeTextExHook,
	    DrawThemeTextHook,
	    DrawTextWHook,
	    DrawTextExWHook,
	    //
	    SetMenuInfoHook,
	    SetMenuItemBitmapsHook,
	    InsertMenuItemWHook,
	    SetMenuItemInfoWHook,
	    ModifyMenuWHook,
	    //
	    CreateCompatibleDCHook,
	    DeleteDCHook,
	    DeleteObjectHook
	);
}

void TranslucentFlyoutsLib::Win32HookShutdown()
{
	Detours::Batch(
	    FALSE,
	    DrawThemeBackgroundHook,
	    //
	    DrawThemeTextExHook,
	    DrawThemeTextHook,
	    DrawTextWHook,
	    DrawTextExWHook,
	    //
	    SetMenuInfoHook,
	    SetMenuItemBitmapsHook,
	    InsertMenuItemWHook,
	    SetMenuItemInfoWHook,
	    ModifyMenuWHook,
	    //
	    CreateCompatibleDCHook,
	    DeleteDCHook,
	    DeleteObjectHook
	);
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

		BufferedPaintSetAlpha(hPaintBuffer, &Rect, 0xFF);
	};

	// 工具提示
	if (
	    IsAllowTransparent() and
	    VerifyThemeData(hTheme, TEXT("Tooltip")) and
	    (GetCurrentFlyoutPolicy() & Tooltip) and
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
			if (!DoBufferedPaint(hdc, &Rect, f, (BYTE)GetCurrentFlyoutOpacity()))
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
	        (GetCurrentFlyoutPolicy() & ViewControl) and
	        (
	            IsViewControlFlyout(GetParent(GetWindowFromHDC(hdc))) or
	            !IsWindow(GetWindowFromHDC(hdc))
	        )
	    )
	)
	{
		if (!DoBufferedPaint(hdc, &Rect, f, (BYTE)GetCurrentFlyoutOpacity()))
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
	            (GetCurrentFlyoutPolicy() & PopupMenu) and
	            !IsViewControlFlyout(GetWindowFromHDC(hdc)) and
	            !IsViewControlFlyout(GetParent(GetWindowFromHDC(hdc)))
	        ) or
	        (
	            (GetCurrentFlyoutPolicy() & ViewControl) and
	            (
	                IsViewControlFlyout(GetWindowFromHDC(hdc)) or
	                IsViewControlFlyout(GetParent(GetWindowFromHDC(hdc)))
	            )
	        )
	    )
	)
	{
		if (IsWindow(g_hWnd))
		{
			if (iPartId != MENU_POPUPBACKGROUND)
			{
				SetWindowEffect(
				    g_hWnd,
				    GetCurrentFlyoutEffect(),
				    GetCurrentFlyoutBorder()
				);
				g_hWnd = nullptr;
			}
		}

		// 在Windows 11 22H2中，Immersive弹出菜单使用的iPartId发生了改变
		// MENU_POPUPBACKGROUND -> 26 (MENU_INTERNAL_POPUPBACKGROUND)
		// MENU_POPUPITEM -> 27 (MENU_INTERNAL_POPUPITEM)
		if (
		    (
		        iPartId == MENU_POPUPBACKGROUND or
		        iPartId == MENU_INTERNAL_POPUPBACKGROUND
		    ) or
		    iPartId == MENU_POPUPGUTTER or
		    (
		        iPartId == MENU_POPUPITEM or
		        iPartId == MENU_INTERNAL_POPUPITEM
		    ) or
		    iPartId == MENU_POPUPBORDERS
		)
		{
			if (
			    (
			        iPartId == MENU_POPUPBACKGROUND or
			        iPartId == MENU_INTERNAL_POPUPBACKGROUND
			    ) or
			    (
			        !IsThemeBackgroundPartiallyTransparent(hTheme, iPartId, iStateId) or
			        !VerifyThemeBackgroundTransparency(hTheme, iPartId, iStateId)
			    )
			)
			{
				BYTE bOpacity = 255;
				switch (GetCurrentFlyoutColorizeOption())
				{
					case Opacity:
					{
						bOpacity = (BYTE)GetCurrentFlyoutOpacity();
						break;
					}
					case Opaque:
					{
						bOpacity = ((iPartId == MENU_POPUPITEM or iPartId == MENU_INTERNAL_POPUPITEM) and iStateId == MPI_HOT) ? 255 : (BYTE)GetCurrentFlyoutOpacity();
						break;
					}
					case Auto:
					{
						bOpacity = ((iPartId == MENU_POPUPITEM or iPartId == MENU_INTERNAL_POPUPITEM) and iStateId == MPI_HOT) ? BYTE(min(255 - ((BYTE)GetCurrentFlyoutOpacity() - 204), 255)) : (BYTE)GetCurrentFlyoutOpacity();
						break;
					}
				}
				if (!DoBufferedPaint(hdc, &Rect, f, bOpacity, BPPF_ERASE | (iPartId == MENU_POPUPBORDERS ? BPPF_NONCLIENT : 0UL)))
				{
					goto Default;
				}
			}
			else
			{
				// 对Windows 11 22H2的支持
				// 先检查是否定义了MENU_INTERNAL_POPUPITEM和MENU_INTERNAL_POPUPBACKGROUND
				if (
				    IsThemePartDefined(hTheme, MENU_INTERNAL_POPUPBACKGROUND, 0) and
				    IsThemePartDefined(hTheme, MENU_INTERNAL_POPUPITEM, 0)
				)
				{
					MyDrawThemeBackground(
					    hTheme,
					    hdc,
					    MENU_INTERNAL_POPUPBACKGROUND,
					    0,
					    pRect,
					    pClipRect
					);
					if (iPartId != MENU_INTERNAL_POPUPITEM or (iPartId == MENU_INTERNAL_POPUPITEM and iStateId != MPI_NORMAL))
					{
						MyDrawThemeBackground(
						    hTheme,
						    hdc,
						    MENU_INTERNAL_POPUPITEM,
						    MPI_NORMAL,
						    pRect,
						    pClipRect
						);
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
				}
				// 对Windows 11 22H2的支持
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
	    pOptions and
	    (
	        !pOptions or
	        (
	            !(pOptions->dwFlags & DTT_CALCRECT) and
	            !(pOptions->dwFlags & DTT_COMPOSITED)
	        )
	    ) and
	    IsAllowTransparent() and
	    (
	        (VerifyThemeData(hTheme, TEXT("Menu")) and GetCurrentFlyoutPolicy() & PopupMenu) or
	        (VerifyThemeData(hTheme, TEXT("Toolbar")) and GetCurrentFlyoutPolicy() & ViewControl) or
	        (VerifyThemeData(hTheme, TEXT("Tooltip")) and GetCurrentFlyoutPolicy() & Tooltip)
	    )
	)
	{
		DTTOPTS Options = *pOptions;
		Options.dwFlags |= DTT_COMPOSITED;

		auto f = [&](HDC hMemDC, HPAINTBUFFER hPaintBuffer)
		{
			g_alphaFixedState = true;

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

			g_alphaFixedState = false;
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
	g_alphaFixedState = true;

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

	g_alphaFixedState = false;
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
	    pRect and
	    IsAllowTransparent() and
	    (
	        (VerifyThemeData(hTheme, TEXT("Menu")) and GetCurrentFlyoutPolicy() & PopupMenu) or
	        (VerifyThemeData(hTheme, TEXT("Toolbar")) and GetCurrentFlyoutPolicy() & ViewControl) or
	        (VerifyThemeData(hTheme, TEXT("Tooltip")) and GetCurrentFlyoutPolicy() & Tooltip)
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
    LPCWSTR lpchText,
    int     cchText,
    LPRECT  lprc,
    UINT    format
)
{
	int nResult = 0;
	thread_local int nLastResult = 0;

	if (
	    (format & DT_CALCRECT) or
	    GetBkMode(hdc) != TRANSPARENT or
	    g_alphaFixedState == true or
	    !IsAllowTransparent() or
	    !(GetCurrentFlyoutPolicy() != Null)
	)
	{
		goto Default;
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
Default:
	nResult =
	    DrawTextWHook.OldFunction<decltype(MyDrawTextW)>(
	        hdc,
	        lpchText,
	        cchText,
	        lprc,
	        format
	    );
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
	    lpdtp or
	    (format & DT_CALCRECT) or
	    GetBkMode(hdc) != TRANSPARENT or
	    g_alphaFixedState == true or
	    !IsAllowTransparent() or
	    !(GetCurrentFlyoutPolicy() != Null)
	)
	{
		goto Default;
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
Default:
	nResult =
	    DrawTextExWHook.OldFunction<decltype(MyDrawTextExW)>(
	        hdc,
	        lpchText,
	        cchText,
	        lprc,
	        format,
	        lpdtp
	    );
	return nResult;
}

BOOL WINAPI TranslucentFlyoutsLib::MySetMenuInfo(
    HMENU hMenu,
    LPCMENUINFO lpMenuInfo
)
{
	BOOL bResult = TRUE;
	// 上次留下的画刷
	thread_local COLORREF dwLastColor = 0xFFFFFF;

	if (
	    (lpMenuInfo->fMask & MIM_BACKGROUND) and
	    lpMenuInfo->hbrBack and
	    IsAllowTransparent() and
	    (GetCurrentFlyoutPolicy() & PopupMenu)
	)
	{
		PBYTE pvBits = nullptr;
		MENUINFO MenuInfo = *lpMenuInfo;
		HBITMAP hBitmap = CreateDIB(nullptr, 1, 1, (PVOID *)&pvBits);

		if (hBitmap and pvBits)
		{
			COLORREF dwColor = GetBrushColor(lpMenuInfo->hbrBack);

			// 获取提供的画刷颜色，设置位图画刷的像素
			if (dwColor != CLR_NONE)
			{
				dwLastColor = dwColor;

				SetPixel(
				    pvBits,
				    GetBValue(dwColor),
				    GetGValue(dwColor),
				    GetRValue(dwColor),
				    (BYTE)GetCurrentFlyoutOpacity()
				);
			}
			else
			{
				SetPixel(
				    pvBits,
				    GetBValue(dwLastColor),
				    GetGValue(dwLastColor),
				    GetRValue(dwLastColor),
				    (BYTE)GetCurrentFlyoutOpacity()
				);
			}

			// 创建位图画刷
			// 只有位图画刷才有Alpha值
			HBRUSH hBrush = CreatePatternBrush(hBitmap);

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

	if (IsAllowTransparent() and GetCurrentFlyoutPolicy() & PopupMenu)
	{
		if (hBitmapUnchecked != hBitmapChecked)
		{
			PrepareAlpha(hBitmapUnchecked);
			PrepareAlpha(hBitmapChecked);
		}
		else
		{
			PrepareAlpha(hBitmapChecked);
		}
	}

	goto Default;
	return bResult;
Default:
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

	if (IsAllowTransparent() and lpmii and GetCurrentFlyoutPolicy() & PopupMenu)
	{
		if (lpmii->fMask & MIIM_BITMAP)
		{
			PrepareAlpha(lpmii->hbmpItem);
		}
		if (lpmii->fMask & MIIM_CHECKMARKS)
		{
			if (lpmii->hbmpUnchecked != lpmii->hbmpChecked)
			{
				PrepareAlpha(lpmii->hbmpUnchecked);
				PrepareAlpha(lpmii->hbmpChecked);
			}
			else
			{
				PrepareAlpha(lpmii->hbmpChecked);
			}
		}
	}

	goto Default;
	return bResult;
Default:
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

	if (IsAllowTransparent() and lpmii and GetCurrentFlyoutPolicy() & PopupMenu)
	{
		if (lpmii->fMask & MIIM_BITMAP)
		{
			PrepareAlpha(lpmii->hbmpItem);
		}
		if (lpmii->fMask & MIIM_CHECKMARKS)
		{
			if (lpmii->hbmpUnchecked != lpmii->hbmpChecked)
			{
				PrepareAlpha(lpmii->hbmpUnchecked);
				PrepareAlpha(lpmii->hbmpChecked);
			}
			else
			{
				PrepareAlpha(lpmii->hbmpChecked);
			}
		}
	}

	goto Default;
	return bResult;
Default:
	bResult = SetMenuItemInfoWHook.OldFunction<decltype(MySetMenuItemInfoW)>(
	              hMenu,
	              item,
	              fByPositon,
	              lpmii
	          );

	return bResult;
}

BOOL WINAPI TranslucentFlyoutsLib::MyModifyMenuW(
    HMENU    hMnu,
    UINT     uPosition,
    UINT     uFlags,
    UINT_PTR uIDNewItem,
    LPCWSTR  lpNewItem
)
{
	BOOL bResult = FALSE;

	if (IsAllowTransparent() and GetCurrentFlyoutPolicy() & PopupMenu)
	{
		if (uFlags & MF_BITMAP)
		{
			PrepareAlpha((HBITMAP)lpNewItem);
		}
	}

	goto Default;
	return bResult;
Default:
	bResult = ModifyMenuWHook.OldFunction<decltype(MyModifyMenuW)>(
	              hMnu,
	              uPosition,
	              uFlags,
	              uIDNewItem,
	              lpNewItem
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