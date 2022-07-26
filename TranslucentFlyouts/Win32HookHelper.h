#pragma once
#include "pch.h"
#include "DetoursHelper.h"

namespace TranslucentFlyoutsLib
{
	extern HRESULT WINAPI MyDrawThemeBackground(
	    HTHEME  hTheme,
	    HDC     hdc,
	    int     iPartId,
	    int     iStateId,
	    LPCRECT pRect,
	    LPCRECT pClipRect
	);
	extern HRESULT WINAPI MyDrawThemeTextEx(
	    HTHEME        hTheme,
	    HDC           hdc,
	    int           iPartId,
	    int           iStateId,
	    LPCTSTR       pszText,
	    int           cchText,
	    DWORD         dwTextFlags,
	    LPRECT        pRect,
	    const DTTOPTS *pOptions
	);
	extern HRESULT WINAPI MyDrawThemeText(
	    HTHEME  hTheme,
	    HDC     hdc,
	    int     iPartId,
	    int     iStateId,
	    LPCTSTR pszText,
	    int     cchText,
	    DWORD   dwTextFlags,
	    DWORD   dwTextFlags2,
	    LPCRECT pRect
	);
	extern int WINAPI MyDrawTextW(
	    HDC     hdc,
	    LPCWSTR lpchText,
	    int     cchText,
	    LPRECT  lprc,
	    UINT    format
	);
	extern int WINAPI MyDrawTextExW(
	    HDC              hdc,
	    LPWSTR           lpchText,
	    int              cchText,
	    LPRECT           lprc,
	    UINT             format,
	    LPDRAWTEXTPARAMS lpdtp
	);
	extern BOOL WINAPI MySetMenuInfo(
	    HMENU hMenu,
	    LPCMENUINFO lpMenuInfo
	);
	extern BOOL WINAPI MySetMenuItemBitmaps(
	    HMENU   hMenu,
	    UINT    uPosition,
	    UINT    uFlags,
	    HBITMAP hBitmapUnchecked,
	    HBITMAP hBitmapChecked
	);
	extern BOOL WINAPI MyInsertMenuItemW(
	    HMENU            hMenu,
	    UINT             item,
	    BOOL             fByPosition,
	    LPCMENUITEMINFOW lpmii
	);
	extern BOOL WINAPI MySetMenuItemInfoW(
	    HMENU            hMenu,
	    UINT             item,
	    BOOL             fByPositon,
	    LPCMENUITEMINFOW lpmii
	);
	extern BOOL WINAPI MyModifyMenuW(
	    HMENU    hMnu,
	    UINT     uPosition,
	    UINT     uFlags,
	    UINT_PTR uIDNewItem,
	    LPCWSTR  lpNewItem
	);
	extern HDC WINAPI MyCreateCompatibleDC(HDC hdc);
	extern BOOL WINAPI MyDeleteDC(HDC hdc);
	extern BOOL WINAPI MyDeleteObject(HGDIOBJ ho);
	//
	// ͸��������
	extern DetoursHook DrawThemeBackgroundHook;
	// ������Ⱦ
	extern DetoursHook DrawThemeTextExHook;
	extern DetoursHook DrawThemeTextHook;
	extern DetoursHook DrawTextWHook;
	extern DetoursHook DrawTextExWHook;
	// ͼ���޸�
	extern DetoursHook SetMenuInfoHook;
	extern DetoursHook SetMenuItemBitmapsHook;
	extern DetoursHook InsertMenuItemWHook;
	extern DetoursHook SetMenuItemInfoWHook;
	extern DetoursHook ModifyMenuWHook;
	// ���ھ���ļ�¼
	extern DetoursHook CreateCompatibleDCHook;
	extern DetoursHook DeleteDCHook;
	extern DetoursHook DeleteObjectHook;
	//
	extern void Win32HookStartup();
	extern void Win32HookShutdown();
};