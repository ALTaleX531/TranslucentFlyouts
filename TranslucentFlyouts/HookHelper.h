#pragma once
#include "pch.h"

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
		LPCTSTR lpchText,
		int     cchText,
		LPRECT  lprc,
		UINT    format
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
	extern void CALLBACK HandleWinEvent(
		HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
		LONG idObject, LONG idChild,
		DWORD dwEventThread, DWORD dwmsEventTime
	);
	extern LRESULT CALLBACK SubclassProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};