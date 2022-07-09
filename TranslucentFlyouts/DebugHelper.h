#pragma once
#include "pch.h"
#include "ThemeHelper.h"
#include "DetoursHelper.h"

namespace TranslucentFlyoutsLib
{
	template <typename... Args>
	static inline void DbgPrint(Args&&... args)
	{
		TCHAR pszDebugString[MAX_PATH] = {};
		_stprintf_s(pszDebugString, std::forward<Args>(args)...);
		OutputDebugString(pszDebugString);
	}

	static inline void COMDbgPrint(HRESULT hr, LPCTSTR pszCustomString = TEXT(""))
	{
		TCHAR pszErrorString[MAX_PATH] = {};
		FormatMessage(
		    FORMAT_MESSAGE_FROM_SYSTEM |
		    FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL,
			hr,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    (LPTSTR)&pszErrorString,
		    MAX_PATH,
		    NULL
		);
		DbgPrint(TEXT("%s -> hr:0x%x[%s]"), pszCustomString, hr, pszErrorString);
	}

	static inline void WindowDbgPrint(HWND hwnd, LPCTSTR pszCustomString = TEXT(""))
	{
		TCHAR pszClassName[MAX_PATH] = {};
		TCHAR pszWindowText[MAX_PATH] = {};
		GetClassName(hwnd, pszClassName, MAX_PATH);
		InternalGetWindowText(hwnd, pszWindowText, MAX_PATH);
		DbgPrint(TEXT("%s -> [%s - %s]0x%x"), pszCustomString, pszClassName, pszWindowText, hwnd);
	}

	static inline void ThemeDbgPrint(HTHEME hTheme, HDC hdc, LPCTSTR pszCustomString = TEXT(""), PVOID pvCaller = _ReturnAddress())
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

	static inline void FunctionDbgPrint(LPCTSTR pszCustomString = TEXT(""), PVOID pvCaller = _ReturnAddress())
	{
		TCHAR pszCallerModule[MAX_PATH] = {};
		GetModuleFileName(DetourGetContainingModule(pvCaller), pszCallerModule, MAX_PATH);
		DbgPrint(L"%s from [%s]", pszCustomString, pszCallerModule);
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