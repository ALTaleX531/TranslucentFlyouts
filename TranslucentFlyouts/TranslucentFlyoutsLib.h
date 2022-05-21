#pragma once
#include "pch.h"
#include "DetoursHelper.h"

class TranslucentFlyoutsLib
{
public:
	static void Startup();
	static void Shutdown();
	static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
private:
	static Detours DrawThemeBackgroundHook;
	static Detours DrawThemeTextExHook;
	static Detours DrawThemeTextHook;
	static Detours DrawTextWHook;
	static Detours SetMenuInfoHook;
	static Detours SetMenuItemBitmapsHook;
	static Detours InsertMenuItemWHook;
	static Detours SetMenuItemInfoWHook;

	static BYTE bAlpha;
	static DWORD dwEffect;
private:
	static inline bool VerifyCaller(PVOID pvCaller, LPCWSTR pszCallerModuleName);
private:
	static HRESULT WINAPI MyDrawThemeBackground(
	    HTHEME  hTheme,
	    HDC     hdc,
	    int     iPartId,
	    int     iStateId,
	    LPCRECT pRect,
	    LPCRECT pClipRect
	);
	static HRESULT WINAPI MyDrawThemeTextEx(
	    HTHEME        hTheme,
	    HDC           hdc,
	    int           iPartId,
	    int           iStateId,
	    LPCWSTR       pszText,
	    int           cchText,
	    DWORD         dwTextFlags,
	    LPRECT        pRect,
	    const DTTOPTS *pOptions
	);
	static HRESULT WINAPI MyDrawThemeText(
	    HTHEME  hTheme,
	    HDC     hdc,
	    int     iPartId,
	    int     iStateId,
	    LPCWSTR pszText,
	    int     cchText,
	    DWORD   dwTextFlags,
	    DWORD   dwTextFlags2,
	    LPCRECT pRect
	);
	static int WINAPI MyDrawTextW(
	    HDC     hdc,
	    LPCWSTR lpchText,
	    int     cchText,
	    LPRECT  lprc,
	    UINT    format
	);
	static BOOL WINAPI MySetMenuInfo(
	    HMENU hMenu,
	    LPCMENUINFO lpMenuInfo
	);
	static BOOL WINAPI MySetMenuItemBitmaps(
	    HMENU   hMenu,
	    UINT    uPosition,
	    UINT    uFlags,
	    HBITMAP hBitmapUnchecked,
	    HBITMAP hBitmapChecked
	);
	static BOOL WINAPI MyInsertMenuItemW(
	    HMENU            hMenu,
	    UINT             item,
	    BOOL             fByPosition,
	    LPCMENUITEMINFOW lpmii
	);
	static BOOL WINAPI MySetMenuItemInfoW(
	    HMENU            hMenu,
	    UINT             item,
	    BOOL             fByPositon,
	    LPCMENUITEMINFOW lpmii
	);
};