#include "pch.h"
#include "tfapi.h"
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

__declspec(thread) HWND TranslucentFlyoutsLib::hWnd = NULL;

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
		HDC hMemDC = nullptr;
		RECT Rect = {0, 0, 1, 1};
		BP_PAINTPARAMS PaintParams = {sizeof(BP_PAINTPARAMS), BPPF_ERASE | BPPF_NOCLIP, nullptr, nullptr};
		HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hdc, &Rect, BPBF_TOPDOWNDIB, &PaintParams, &hMemDC);
		if (hPaintBuffer and hMemDC)
		{
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
				int cxRow = 0;
				RGBQUAD *pvPixel;

				if (SUCCEEDED(GetBufferedPaintBits(hPaintBuffer, &pvPixel, &cxRow)))
				{
					if (pvPixel->rgbReserved != 0xff)
					{
						bResult = true;
					}
				}
			}
			EndBufferedPaint(hPaintBuffer, FALSE);
		}
		return bResult;
	};
	HRESULT hr = S_OK;

	if (ThemeHelper::IsAllowTransparent() and ThemeHelper::VerifyThemeData(hTheme, TEXT("Menu")))
	{
		if (hWnd and IsWindow(hWnd))
		{
			if (iPartId != MENU_POPUPBACKGROUND)
			{
				AcrylicHelper::SetEffect(
				    hWnd,
				    GetCurrentFlyoutEffect()
				);
				SetFlyout(NULL);
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
			        iPartId == MENU_POPUPITEM and iStateId != MPI_HOT
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
			if (iPartId == MENU_POPUPITEM and (iStateId != MPI_HOT or iStateId != MPI_DISABLEDHOT))
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
    LPCWSTR       pszText,
    int           cchText,
    DWORD         dwTextFlags,
    LPRECT        pRect,
    const DTTOPTS *pOptions
)
{
	HRESULT hr = S_OK;
	if (
	    ThemeHelper::IsAllowTransparent() and
	    ThemeHelper::VerifyThemeData(hTheme, TEXT("Menu")) and
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
    LPCWSTR pszText,
    int     cchText,
    DWORD   dwTextFlags,
    DWORD   dwTextFlags2,
    LPCRECT pRect
)
{
	// dwTextFlags2 从未被使用，始终为0
	// dwTextFlags 不支持DT_CALCRECT
	HRESULT hr = S_OK;

	if (ThemeHelper::IsAllowTransparent() and ThemeHelper::VerifyThemeData(hTheme, TEXT("Menu")))
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
    LPCWSTR lpchText,
    int     cchText,
    LPRECT  lprc,
    UINT    format
)
{
	int nResult = 0;
	__declspec(thread) static int nLastResult = 0;

	if (!ThemeHelper::IsAllowTransparent())
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
	else if ((VerifyCaller(_ReturnAddress(), TEXT("Uxtheme")) or VerifyCaller(_ReturnAddress(), TEXT("Comctl32")) or (format & DT_CALCRECT)))
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
		HTHEME hTheme = OpenThemeData(NULL, TEXT("Menu"));
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
	__declspec(thread) static COLORREF dwLastColor = 0xFFFFFF;

	if ((lpMenuInfo->fMask & MIM_BACKGROUND) and lpMenuInfo->hbrBack and ThemeHelper::IsAllowTransparent())
	{
		PBYTE pvBits = nullptr;
		MENUINFO MenuInfo = *lpMenuInfo;
		HBITMAP hBitmap = ThemeHelper::CreateDIB(NULL, 1, 1, (PVOID*)&pvBits);
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

bool TranslucentFlyoutsLib::VerifyCaller(PVOID pvCaller, LPCWSTR pszCallerModuleName)
{
	HMODULE hModule = DetourGetContainingModule(pvCaller);
	return hModule == GetModuleHandle(pszCallerModuleName);
}

void TranslucentFlyoutsLib::SetFlyout(HWND hWnd)
{
	TranslucentFlyoutsLib::hWnd = hWnd;
}

/*LRESULT CALLBACK TranslucentFlyoutsLib::SubclassProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (Message)
	{
		default:
			return DefSubclassProc(hWnd, Message, wParam, lParam);
	}
	return 0;
}*/

LRESULT CALLBACK TranslucentFlyoutsLib::WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_CREATE:
		{
			/*WCHAR p[261];
			GetClassName(hWnd, p, MAX_PATH);
			OutputDebugString(L"current:");
			OutputDebugString(p);
			GetClassName(GetParent(hWnd), p, MAX_PATH);
			OutputDebugString(L"parent:");
			OutputDebugString(p);*/

			if (ThemeHelper::IsAllowTransparent() and ThemeHelper::IsValidFlyout(hWnd))
			{
				SetFlyout(hWnd);
				RedrawWindow(
				    hWnd,
				    nullptr, nullptr,
				    RDW_INTERNALPAINT
				);
			}
			break;
		}

		case WM_NCCREATE:
		{
			if (ThemeHelper::IsAllowTransparent() and ThemeHelper::IsValidFlyout(hWnd))
			{
				BufferedPaintInit();
				//SetWindowSubclass(hWnd, SubclassProc, 0, 0);
			}

			break;
		}

		case WM_NCDESTROY:
		{
			if (ThemeHelper::IsAllowTransparent() and ThemeHelper::IsValidFlyout(hWnd))
			{
				BufferedPaintUnInit();
				//RemoveWindowSubclass(hWnd, SubclassProc, 0);
			}

			break;
		}
		default:
			break;
	}
	return 0;
}