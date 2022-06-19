#include "pch.h"
#include "tflapi.h"
#include "ThemeHelper.h"
#include "AcrylicHelper.h"
#include "TranslucentFlyoutsLib.h"
#include <memory>

using std::nothrow;
using namespace TranslucentFlyoutsLib;

// 透明化处理
DetoursHook DrawThemeBackgroundHook("Uxtheme", "DrawThemeBackground", MyDrawThemeBackground);
// 文字渲染
DetoursHook DrawThemeTextExHook("Uxtheme", "DrawThemeTextEx", MyDrawThemeTextEx);
DetoursHook DrawThemeTextHook("Uxtheme", "DrawThemeText", MyDrawThemeText);
DetoursHook DrawTextWHook("User32", "DrawTextW", MyDrawTextW);
// 图标修复
DetoursHook SetMenuInfoHook("User32", "SetMenuInfo", MySetMenuInfo);
DetoursHook SetMenuItemBitmapsHook("User32", "SetMenuItemBitmaps", MySetMenuItemBitmaps);
DetoursHook InsertMenuItemWHook("User32", "InsertMenuItemW", MyInsertMenuItemW);
DetoursHook SetMenuItemInfoWHook("User32", "SetMenuItemInfoW", MySetMenuItemInfoW);

thread_local HWND hWnd = nullptr;

bool VerifyCaller(PVOID pvCaller, LPCTSTR pszCallerModuleName)
{
	HMODULE hModule = DetourGetContainingModule(pvCaller);
	return hModule == GetModuleHandle(pszCallerModuleName);
}

void SetFlyout(HWND hWnd)
{
	::hWnd = hWnd;
}

void TranslucentFlyoutsLib::Startup()
{
	Detours::Begin();
	Detours::Batch(
	    TRUE,
	    DrawThemeBackgroundHook,
	    DrawThemeTextExHook,
	    DrawThemeTextHook,
	    DrawTextWHook,
	    SetMenuInfoHook,
	    SetMenuItemBitmapsHook,
	    InsertMenuItemWHook,
	    SetMenuItemInfoWHook
	);
	Detours::Commit();
}

void TranslucentFlyoutsLib::Shutdown()
{
	Detours::Begin();
	Detours::Batch(
	    FALSE,
	    DrawThemeBackgroundHook,
	    DrawThemeTextExHook,
	    DrawThemeTextHook,
	    DrawTextWHook,
	    SetMenuInfoHook,
	    SetMenuItemBitmapsHook,
	    InsertMenuItemWHook,
	    SetMenuItemInfoWHook
	);
	Detours::Commit();
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
	auto IsThemeBackgroundPartiallyTransparent = [&](const HTHEME & hTheme, const int& iPartId, const int& iStateId) -> bool
	{
		bool bResult = false;
		PBYTE pvBits = nullptr;
		RECT Rect = {0, 0, 1, 1};
		HDC hMemDC = CreateCompatibleDC(nullptr);
		if (hMemDC)
		{
			HBITMAP hBitmap = CreateDIB(nullptr, Rect.right, Rect.bottom, (PVOID*)&pvBits);
			if (hBitmap)
			{
				SelectObject(hMemDC, hBitmap);
				if (
				    SUCCEEDED(
				        DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
				            hTheme,
				            hMemDC,
				            iPartId,
				            iStateId,
				            &Rect,
				            nullptr
				        )
				    )
				)
				{
					if (pvBits[3] != 255)
					{
						bResult = true;
					}
				}
				DeleteObject(hBitmap);
			}

			DeleteDC(hMemDC);
		}
		return bResult;
	};
	HRESULT hr = S_OK;

	/*if (VerifyCaller(_ReturnAddress(), TEXT("Explorerframe")) or VerifyCaller(_ReturnAddress(), TEXT("dui70")))
	{
		TCHAR pszClassName[MAX_PATH + 1];
		ThemeHelper::GetThemeClass(hTheme, pszClassName, MAX_PATH);
		OutputDebugString(pszClassName);
	}*/
	/*if (ThemeHelper::VerifyThemeData(hTheme, TEXT("Menu")))
	{
		HMODULE hModule = DetourGetContainingModule(_ReturnAddress());
		TCHAR pszClassName[MAX_PATH + 1];
		GetModuleFileName(hModule, pszClassName, MAX_PATH);
		OutputDebugString(pszClassName);
	}*/
	if (IsAllowTransparent() and VerifyThemeData(hTheme, TEXT("Tooltip")) and iPartId == TTP_STANDARD and GetCurrentFlyoutPolicy() & Tooltip)
	{
		if (hWnd and IsWindow(hWnd))
		{
			SetWindowEffect(
			    hWnd,
			    GetCurrentFlyoutEffect(),
			    GetCurrentFlyoutBorder()
			);
			SetFlyout(nullptr);
		}

		if (
		    !::IsThemeBackgroundPartiallyTransparent(hTheme, iPartId, iStateId) or
		    !IsThemeBackgroundPartiallyTransparent(hTheme, iPartId, iStateId)
		)
		{
			RECT Rect = *pRect;
			if (pClipRect)
			{
				::IntersectRect(&Rect, pRect, pClipRect);
			}

			HDC hMemDC = nullptr;
			BLENDFUNCTION BlendFunction = {AC_SRC_OVER, 0, (BYTE)GetCurrentFlyoutOpacity(), AC_SRC_ALPHA};
			BP_PAINTPARAMS PaintParams = {sizeof(BP_PAINTPARAMS), BPPF_ERASE, nullptr, &BlendFunction};
			HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdc, &Rect, BPBF_TOPDOWNDIB, &PaintParams, &hMemDC);

			if (hPaintBuffer and hMemDC)
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

				BufferedPaintSetAlpha(hPaintBuffer, &Rect, 255);
				EndBufferedPaint(hPaintBuffer, TRUE);
			}
			else
			{
				hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
				         hTheme,
				         hdc,
				         iPartId,
				         iStateId,
				         pRect,
				         pClipRect
				     );
			}
		}
		else
		{
			hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
			         hTheme,
			         hdc,
			         iPartId,
			         iStateId,
			         pRect,
			         pClipRect
			     );
		}
	}
	else if (IsAllowTransparent() and VerifyThemeData(hTheme, TEXT("Toolbar")) and !VerifyCaller(_ReturnAddress(), TEXT("comctrl32")) and GetCurrentFlyoutPolicy() & ViewControl and iPartId == 0 and iStateId == 0)
	{
		RECT Rect = *pRect;
		if (pClipRect)
		{
			::IntersectRect(&Rect, pRect, pClipRect);
		}

		HDC hMemDC = nullptr;
		BLENDFUNCTION BlendFunction = {AC_SRC_OVER, 0, (BYTE)GetCurrentFlyoutOpacity(), AC_SRC_ALPHA};
		BP_PAINTPARAMS PaintParams = {sizeof(BP_PAINTPARAMS), BPPF_ERASE, nullptr, &BlendFunction};
		HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdc, &Rect, BPBF_TOPDOWNDIB, &PaintParams, &hMemDC);

		if (hPaintBuffer and hMemDC)
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

			BufferedPaintSetAlpha(hPaintBuffer, &Rect, 255);
			EndBufferedPaint(hPaintBuffer, TRUE);
		}
		else
		{
			hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
			         hTheme,
			         hdc,
			         iPartId,
			         iStateId,
			         pRect,
			         pClipRect
			     );
		}
	}
	else if (IsAllowTransparent() and VerifyThemeData(hTheme, TEXT("Menu")) and GetCurrentFlyoutPolicy() & PopupMenu)
	{
		if (hWnd and IsWindow(hWnd))
		{
			if (iPartId != MENU_POPUPBACKGROUND)
			{
				SetWindowEffect(
				    hWnd,
				    GetCurrentFlyoutEffect(),
				    GetCurrentFlyoutBorder()
				);
				SetFlyout(nullptr);
			}
		}

		if (
		    !::IsThemeBackgroundPartiallyTransparent(hTheme, iPartId, iStateId) or
		    !IsThemeBackgroundPartiallyTransparent(hTheme, iPartId, iStateId)
		)
		{
			RECT Rect = *pRect;
			if (pClipRect)
			{
				::IntersectRect(&Rect, pRect, pClipRect);
			}

			if (
			    iPartId == MENU_POPUPBACKGROUND or
			    iPartId == MENU_POPUPGUTTER or
			    (
			        GetCurrentFlyoutColorizeOption() == 0 ?
			        (iPartId == MENU_POPUPITEM and iStateId != MPI_HOT) :
			        (iPartId == MENU_POPUPITEM)
			    ) or
			    iPartId == MENU_POPUPBORDERS
			)
			{
				HDC hMemDC = nullptr;
				BLENDFUNCTION BlendFunction = {AC_SRC_OVER, 0, (BYTE)GetCurrentFlyoutOpacity(), AC_SRC_ALPHA};
				BP_PAINTPARAMS PaintParams = {sizeof(BP_PAINTPARAMS), BPPF_ERASE, nullptr, &BlendFunction};
				HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdc, &Rect, BPBF_TOPDOWNDIB, &PaintParams, &hMemDC);

				if (hPaintBuffer and hMemDC)
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

					BufferedPaintSetAlpha(hPaintBuffer, &Rect, 255);
					EndBufferedPaint(hPaintBuffer, TRUE);
				}
				else
				{
					hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
					         hTheme,
					         hdc,
					         iPartId,
					         iStateId,
					         pRect,
					         pClipRect
					     );
				}
			}
			else
			{
				hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
				         hTheme,
				         hdc,
				         iPartId,
				         iStateId,
				         pRect,
				         pClipRect
				     );
			}
		}
		else
		{
			if (
			    (
			        iPartId == MENU_POPUPITEM and
			        (iStateId != MPI_HOT and iStateId != MPI_DISABLEDHOT)
			    ) and
			    (
			        !::IsThemeBackgroundPartiallyTransparent(hTheme, MENU_POPUPBACKGROUND, 0) or
			        !IsThemeBackgroundPartiallyTransparent(hTheme, MENU_POPUPBACKGROUND, 0)
			    )
			)
			{
				DrawThemeBackground(
				    hTheme,
				    hdc,
				    MENU_POPUPBACKGROUND,
				    0,
				    pRect,
				    pClipRect
				);
			}

			hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
			         hTheme,
			         hdc,
			         iPartId,
			         iStateId,
			         pRect,
			         pClipRect
			     );
		}
	}
	else
	{
		hr = DrawThemeBackgroundHook.OldFunction<decltype(MyDrawThemeBackground)>(
		         hTheme,
		         hdc,
		         iPartId,
		         iStateId,
		         pRect,
		         pClipRect
		     );
	}
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
	HRESULT hr = S_OK;
	if (
	    IsAllowTransparent() and
	    (
	        (VerifyThemeData(hTheme, TEXT("Menu")) and GetCurrentFlyoutPolicy() & PopupMenu) or
	        (VerifyThemeData(hTheme, TEXT("Toolbar")) and GetCurrentFlyoutPolicy() & ViewControl) or
	        (VerifyThemeData(hTheme, TEXT("Tooltip")) and GetCurrentFlyoutPolicy() & Tooltip)
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
		HDC hMemDC = nullptr;
		DTTOPTS Options = *pOptions;
		Options.dwFlags |= DTT_COMPOSITED;


		BLENDFUNCTION BlendFunction = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		BP_PAINTPARAMS PaintParams = {sizeof(BP_PAINTPARAMS), BPPF_ERASE, nullptr, &BlendFunction};
		HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdc, pRect, BPBF_TOPDOWNDIB, &PaintParams, &hMemDC);

		if (hPaintBuffer and hMemDC)
		{
			SelectObject(hMemDC, GetCurrentObject(hdc, OBJ_FONT));
			SetBkMode(hMemDC, GetBkMode(hdc));
			SetBkColor(hMemDC, GetBkColor(hdc));
			SetTextAlign(hMemDC, GetTextAlign(hdc));
			SetTextCharacterExtra(hMemDC, GetTextCharacterExtra(hdc));

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

			EndBufferedPaint(hPaintBuffer, TRUE);
		}
		else
		{
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
		}

		return hr;
	}
	else
	{
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
	}
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
	// dwTextFlags2 从未被使用，始终为0
	// dwTextFlags 不支持DT_CALCRECT
	HRESULT hr = S_OK;

	if (
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
    LPCTSTR lpchText,
    int     cchText,
    LPRECT  lprc,
    UINT    format
)
{
	int nResult = 0;
	thread_local static int nLastResult = 0;

	if (!IsAllowTransparent() or GetBkMode(hdc) != TRANSPARENT)
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
	else if ((VerifyCaller(_ReturnAddress(), TEXT("Uxtheme")) or (format & DT_CALCRECT)))
	{
		nLastResult =
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

BOOL WINAPI TranslucentFlyoutsLib::MySetMenuInfo(
    HMENU hMenu,
    LPCMENUINFO lpMenuInfo
)
{
	BOOL bResult = FALSE;
	thread_local COLORREF dwLastColor = 0xFFFFFF;

	if (
	    (lpMenuInfo->fMask & MIM_BACKGROUND) and
	    lpMenuInfo->hbrBack and
	    IsAllowTransparent() and
	    GetCurrentFlyoutPolicy() & PopupMenu
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

			if (hBrush)
			{
				// 此画刷会被菜单绘制者自动释放
				MenuInfo.hbrBack = hBrush;
				// 我们替换了调用者提供的画刷，所以必须手动释放资源
				DeleteObject(lpMenuInfo->hbrBack);
			}

			DeleteObject(hBitmap);
			bResult = SetMenuInfoHook.OldFunction<decltype(MySetMenuInfo)>(
			              hMenu,
			              &MenuInfo
			          );
		}
		else
		{
			bResult = SetMenuInfoHook.OldFunction<decltype(MySetMenuInfo)>(
			              hMenu,
			              lpMenuInfo
			          );
		}
	}
	else
	{
		bResult = SetMenuInfoHook.OldFunction<decltype(MySetMenuInfo)>(
		              hMenu,
		              lpMenuInfo
		          );
	}

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

	if (IsAllowTransparent() and lpmii and (lpmii->fMask & MIIM_CHECKMARKS or lpmii->fMask & MIIM_BITMAP) and GetCurrentFlyoutPolicy() & PopupMenu)
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

	if (IsAllowTransparent() and lpmii and (lpmii->fMask & MIIM_CHECKMARKS or lpmii->fMask & MIIM_BITMAP) and GetCurrentFlyoutPolicy() & PopupMenu)
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
			if (IsAllowTransparent() and IsValidFlyout(hWnd))
			{
				SetFlyout(hWnd);
				BufferedPaintInit();
			}
		}

		if (dwEvent == EVENT_OBJECT_DESTROY)
		{
			if (IsAllowTransparent() and IsValidFlyout(hWnd))
			{
				BufferedPaintUnInit();
			}
		}

		if (dwEvent == EVENT_OBJECT_SHOW)
		{
			if (IsAllowTransparent() and IsTooltipClass(hWnd) and GetCurrentFlyoutPolicy() & Tooltip)
			{
				SetFlyout(hWnd);
			}
		}
	}
}