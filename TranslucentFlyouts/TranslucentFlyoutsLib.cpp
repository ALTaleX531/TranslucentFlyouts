#include "pch.h"
#include "tflapi.h"
//#include "SettingsHelper.h"
#include "ThemeHelper.h"
#include "AcrylicHelper.h"
#include "TranslucentFlyoutsLib.h"

using namespace TranslucentFlyoutsLib;
extern HMODULE g_hModule;
extern HWND g_hWnd;

void TranslucentFlyoutsLib::Startup()
{
	Detours::Begin();
	//
	TranslucentFlyoutsLib::Win32HookStartup();
	TranslucentFlyoutsLib::WRHookStartup();
	//
	Detours::Commit();
}

void TranslucentFlyoutsLib::Shutdown()
{
	Detours::Begin();
	//
	TranslucentFlyoutsLib::Win32HookShutdown();
	TranslucentFlyoutsLib::WRHookShutdown();
	//
	Detours::Commit();
	//
	ClearDeviceContextList();
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
		if (IsPopupMenuFlyout(hWnd) and GetCurrentFlyoutPolicy() & PopupMenu)
		{
			g_hWnd = hWnd;
		}
		if (IsViewControlFlyout(hWnd) and GetCurrentFlyoutPolicy() & ViewControl)
		{
			SetWindowEffect(
			    hWnd,
			    GetCurrentFlyoutEffect(),
			    GetCurrentFlyoutBorder()
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
		if (IsTooltipFlyout(hWnd) and GetCurrentFlyoutPolicy() & Tooltip)
		{
			SetWindowEffect(
			    hWnd,
				GetCurrentFlyoutEffect(),
				GetCurrentFlyoutBorder()
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