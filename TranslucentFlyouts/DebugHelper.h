#pragma once
#include "pch.h"
#include "ThemeHelper.h"
#include "DetoursHelper.h"

namespace TranslucentFlyoutsLib
{
	template <typename... Args>
	static inline void DbgPrint(Args&&... args)
	{
		TCHAR pszDebugString[MAX_PATH];
		_stprintf_s(pszDebugString, MAX_PATH, args...);
		OutputDebugString(pszDebugString);
	}

	static inline void ThemeDbgPrint(HTHEME hTheme, HDC hdc, PVOID pvCaller = _ReturnAddress())
	{
		TCHAR pszThemeClass[MAX_PATH] = {};
		TCHAR pszClassName[MAX_PATH] = {};
		TCHAR pszWindowText[MAX_PATH] = {};
		TCHAR pszCallerModule[MAX_PATH] = {};
		HWND hwnd = GetWindowFromHDC(hdc);
		GetClassName(hwnd, pszClassName, MAX_PATH);
		InternalGetWindowText(hwnd, pszWindowText, MAX_PATH);
		GetThemeClass(hTheme, pszThemeClass, 260);
		GetModuleFileName(DetourGetContainingModule(pvCaller), pszCallerModule, MAX_PATH);
		DbgPrint(TEXT("[0x%p][%s - %s] -> [%s] from [%s]"), hwnd, pszClassName, pszWindowText, pszThemeClass, pszCallerModule);
	}

	static inline void GdiDbgPrint(HDC hdc, LPCTSTR pszCustomString, PVOID pvCaller = _ReturnAddress())
	{
		TCHAR pszClassName[MAX_PATH] = {};
		TCHAR pszWindowText[MAX_PATH] = {};
		TCHAR pszCallerModule[MAX_PATH] = {};
		HWND hwnd = GetWindowFromHDC(hdc);
		GetClassName(hwnd, pszClassName, MAX_PATH);
		InternalGetWindowText(hwnd, pszWindowText, MAX_PATH);
		GetModuleFileName(DetourGetContainingModule(pvCaller), pszCallerModule, MAX_PATH);
		DbgPrint(TEXT("[0x%p][%s - %s] -> %s [%s]"), hwnd, pszClassName, pszWindowText, pszCustomString, pszCallerModule);
	}

	static inline void HighlightBox(HDC hdc, LPCRECT lpRect, COLORREF dwColor)
	{
		HPEN hPen = CreatePen(PS_SOLID, 4, dwColor);
		HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
		Rectangle(hdc, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
	}
}