#include "pch.h"
#include "tflapi.h"
#include "ThemeHelper.h"
#include "AcrylicHelper.h"
#include "TranslucentFlyoutsLib.h"
#include <memory>

using std::nothrow;
// 透明化处理
Detours TranslucentFlyoutsLib::DrawThemeBackgroundHook("Uxtheme", "DrawThemeBackground", MyDrawThemeBackground);
// 文字渲染
Detours TranslucentFlyoutsLib::DrawThemeTextExHook("Uxtheme", "DrawThemeTextEx", MyDrawThemeTextEx);
Detours TranslucentFlyoutsLib::DrawThemeTextHook("Uxtheme", "DrawThemeText", MyDrawThemeText);
Detours TranslucentFlyoutsLib::DrawTextWHook("User32", "DrawTextW", MyDrawTextW);
// 图标修复
Detours TranslucentFlyoutsLib::SetMenuInfoHook("User32", "SetMenuInfo", MySetMenuInfo);
Detours TranslucentFlyoutsLib::SetMenuItemBitmapsHook("User32", "SetMenuItemBitmaps", MySetMenuItemBitmaps);
Detours TranslucentFlyoutsLib::InsertMenuItemWHook("User32", "InsertMenuItemW", MyInsertMenuItemW);
Detours TranslucentFlyoutsLib::SetMenuItemInfoWHook("User32", "SetMenuItemInfoW", MySetMenuItemInfoW);

thread_local HWND TranslucentFlyoutsLib::hWnd = nullptr;

void TranslucentFlyoutsLib::Startup()
{
	Detours::BeginHook();
	DrawThemeBackgroundHook.SetHookState(TRUE);
	DrawThemeTextExHook.SetHookState(TRUE);
	DrawThemeTextHook.SetHookState(TRUE);
	DrawTextWHook.SetHookState(TRUE);
	SetMenuInfoHook.SetHookState(TRUE);
	SetMenuItemBitmapsHook.SetHookState(TRUE);
	InsertMenuItemWHook.SetHookState(TRUE);
	SetMenuItemInfoWHook.SetHookState(TRUE);
	Detours::EndHook();
}

void TranslucentFlyoutsLib::Shutdown()
{
	Detours::BeginHook();
	DrawThemeBackgroundHook.SetHookState(FALSE);
	DrawThemeTextExHook.SetHookState(FALSE);
	DrawThemeTextHook.SetHookState(FALSE);
	DrawTextWHook.SetHookState(FALSE);
	SetMenuInfoHook.SetHookState(FALSE);
	SetMenuItemBitmapsHook.SetHookState(FALSE);
	InsertMenuItemWHook.SetHookState(FALSE);
	SetMenuItemInfoWHook.SetHookState(FALSE);
	Detours::EndHook();
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
			HBITMAP hBitmap = ThemeHelper::CreateDIB(nullptr, Rect.right, Rect.bottom, (PVOID*)&pvBits);
			if (hBitmap)
			{
				SelectObject(hMemDC, hBitmap);
				if (
				    SUCCEEDED(
				        CallOldFunction(
				            DrawThemeBackgroundHook,
				            MyDrawThemeBackground,
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

	if (ThemeHelper::IsAllowTransparent() and ThemeHelper::VerifyThemeData(hTheme, TEXT("Tooltip")) and iPartId == TTP_STANDARD)
	{
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
				ThemeHelper::Clear(hdc, &Rect);

				hr = CallOldFunction(
				         DrawThemeBackgroundHook,
				         MyDrawThemeBackground,
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
				hr = CallOldFunction(
				         DrawThemeBackgroundHook,
				         MyDrawThemeBackground,
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
			hr = CallOldFunction(
			         DrawThemeBackgroundHook,
			         MyDrawThemeBackground,
			         hTheme,
			         hdc,
			         iPartId,
			         iStateId,
			         pRect,
			         pClipRect
			     );
		}
	}
	else if (ThemeHelper::IsAllowTransparent() and ThemeHelper::VerifyThemeData(hTheme, TEXT("Toolbar")) and iPartId == 0 and iStateId == 0)
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
			ThemeHelper::Clear(hdc, &Rect);

			hr = CallOldFunction(
			         DrawThemeBackgroundHook,
			         MyDrawThemeBackground,
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
			hr = CallOldFunction(
			         DrawThemeBackgroundHook,
			         MyDrawThemeBackground,
			         hTheme,
			         hdc,
			         iPartId,
			         iStateId,
			         pRect,
			         pClipRect
			     );
		}
	}
	else if (ThemeHelper::IsAllowTransparent() and ThemeHelper::VerifyThemeData(hTheme, TEXT("Menu")))
	{
		if (hWnd and IsWindow(hWnd))
		{
			if (iPartId != MENU_POPUPBACKGROUND)
			{
				AcrylicHelper::SetEffect(
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
					ThemeHelper::Clear(hdc, &Rect);

					hr = CallOldFunction(
					         DrawThemeBackgroundHook,
					         MyDrawThemeBackground,
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
					hr = CallOldFunction(
					         DrawThemeBackgroundHook,
					         MyDrawThemeBackground,
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
				hr = CallOldFunction(
				         DrawThemeBackgroundHook,
				         MyDrawThemeBackground,
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

			hr = CallOldFunction(
			         DrawThemeBackgroundHook,
			         MyDrawThemeBackground,
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
		hr = CallOldFunction(
		         DrawThemeBackgroundHook,
		         MyDrawThemeBackground,
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
	    ThemeHelper::IsAllowTransparent() and
	    (ThemeHelper::VerifyThemeData(hTheme, TEXT("Menu")) or ThemeHelper::VerifyThemeData(hTheme, TEXT("Toolbar")) or ThemeHelper::VerifyThemeData(hTheme, TEXT("Tooltip"))) and
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

			hr = CallOldFunction(
			         DrawThemeTextExHook,
			         MyDrawThemeTextEx,
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
			hr = CallOldFunction(
			         DrawThemeTextExHook,
			         MyDrawThemeTextEx,
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
		hr = CallOldFunction(
		         DrawThemeTextExHook,
		         MyDrawThemeTextEx,
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

	if (ThemeHelper::IsAllowTransparent() and (ThemeHelper::VerifyThemeData(hTheme, TEXT("Menu")) or ThemeHelper::VerifyThemeData(hTheme, TEXT("Toolbar")) or ThemeHelper::VerifyThemeData(hTheme, TEXT("Tooltip"))))
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
		hr = CallOldFunction(
		         DrawThemeTextHook,
		         MyDrawThemeText,
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

	if (!ThemeHelper::IsAllowTransparent() or GetBkMode(hdc) != TRANSPARENT)
	{
		nResult =
		    CallOldFunction(
		        DrawTextWHook,
		        MyDrawTextW,
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
		        CallOldFunction(
		            DrawTextWHook,
		            MyDrawTextW,
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

	if ((lpMenuInfo->fMask & MIM_BACKGROUND) and lpMenuInfo->hbrBack and ThemeHelper::IsAllowTransparent())
	{
		PBYTE pvBits = nullptr;
		MENUINFO MenuInfo = *lpMenuInfo;
		HBITMAP hBitmap = ThemeHelper::CreateDIB(nullptr, 1, 1, (PVOID*)&pvBits);
		if (hBitmap and pvBits)
		{
			COLORREF dwColor = ThemeHelper::GetBrushColor(lpMenuInfo->hbrBack);

			// 获取提供的画刷颜色，设置位图画刷的像素
			if (dwColor != CLR_NONE)
			{
				dwLastColor = dwColor;
				ThemeHelper::SetPixel(
				    pvBits,
				    GetBValue(dwColor),
				    GetGValue(dwColor),
				    GetRValue(dwColor),
				    (BYTE)GetCurrentFlyoutOpacity()
				);
			}
			else
			{
				ThemeHelper::SetPixel(
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
			bResult = CallOldFunction(
			              SetMenuInfoHook,
			              MySetMenuInfo,
			              hMenu,
			              &MenuInfo
			          );
		}
		else
		{
			bResult = CallOldFunction(
			              SetMenuInfoHook,
			              MySetMenuInfo,
			              hMenu,
			              lpMenuInfo
			          );
		}
	}
	else
	{
		bResult = CallOldFunction(
		              SetMenuInfoHook,
		              MySetMenuInfo,
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

	if (ThemeHelper::IsAllowTransparent())
	{
		ThemeHelper::PrepareAlpha(hBitmapUnchecked);
		ThemeHelper::PrepareAlpha(hBitmapChecked);
	}

	bResult = CallOldFunction(
	              SetMenuItemBitmapsHook,
	              MySetMenuItemBitmaps,
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

	if (ThemeHelper::IsAllowTransparent() and lpmii and (lpmii->fMask & MIIM_CHECKMARKS or lpmii->fMask & MIIM_BITMAP))
	{
		ThemeHelper::PrepareAlpha(lpmii->hbmpItem);
		ThemeHelper::PrepareAlpha(lpmii->hbmpUnchecked);
		ThemeHelper::PrepareAlpha(lpmii->hbmpChecked);
	}
	bResult = CallOldFunction(
	              InsertMenuItemWHook,
	              MyInsertMenuItemW,
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

	if (ThemeHelper::IsAllowTransparent() and lpmii and (lpmii->fMask & MIIM_CHECKMARKS or lpmii->fMask & MIIM_BITMAP))
	{
		ThemeHelper::PrepareAlpha(lpmii->hbmpItem);
		ThemeHelper::PrepareAlpha(lpmii->hbmpUnchecked);
		ThemeHelper::PrepareAlpha(lpmii->hbmpChecked);
	}
	bResult = CallOldFunction(
	              SetMenuItemInfoWHook,
	              MySetMenuItemInfoW,
	              hMenu,
	              item,
	              fByPositon,
	              lpmii
	          );

	return bResult;
}

bool TranslucentFlyoutsLib::VerifyCaller(PVOID pvCaller, LPCTSTR pszCallerModuleName)
{
	HMODULE hModule = DetourGetContainingModule(pvCaller);
	return hModule == GetModuleHandle(pszCallerModuleName);
}

void TranslucentFlyoutsLib::SetFlyout(HWND hWnd)
{
	TranslucentFlyoutsLib::hWnd = hWnd;
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
	if (hWnd)
	{
		if (dwEvent == EVENT_OBJECT_CREATE)
		{
			if (ThemeHelper::IsAllowTransparent() and ThemeHelper::IsValidFlyout(hWnd))
			{
				SetFlyout(hWnd);
				BufferedPaintInit();
			}

			if (ThemeHelper::IsViewControlClass(GetParent(hWnd)) and ThemeHelper::IsToolbarWindow(hWnd))
			{
				//SetWindowSubclass(hWnd, SubclassProc, 0, 0);
			}
		}

		if (dwEvent == EVENT_OBJECT_DESTROY)
		{
			if (ThemeHelper::IsAllowTransparent() and ThemeHelper::IsValidFlyout(hWnd))
			{
				BufferedPaintUnInit();
			}

			if (ThemeHelper::IsViewControlClass(GetParent(hWnd)) and ThemeHelper::IsToolbarWindow(hWnd))
			{
				//RemoveWindowSubclass(hWnd, SubclassProc, 0);
			}
		}

		if (dwEvent == EVENT_OBJECT_SHOW)
		{
			if (ThemeHelper::IsAllowTransparent() and ThemeHelper::IsTooltipClass(hWnd))
			{
				AcrylicHelper::SetEffect(
				    hWnd,
				    GetCurrentFlyoutEffect(),
				    GetCurrentFlyoutBorder()
				);
			}
		}
	}
	/*case WM_NCHITTEST:
		{
			TCHAR p[261] = {'n','u','l','l'};
			ATOM prop = (ATOM)GetProp(hWnd, (LPCTSTR)0xA910);
			GetAtomName(prop, p, 260);
			OutputDebugString(p);
			prop = (ATOM)GetProp(hWnd, (LPCTSTR)0xA911);
			GetAtomName(prop, p, 260);
			OutputDebugString(p);
			break;
		}*/
}