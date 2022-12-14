#include "pch.h"
#include "tflapi.h"
#include "ThemeHelper.h"
#include "AcrylicHelper.h"
#include "TranslucentFlyoutsLib.h"

using namespace TranslucentFlyoutsLib;
extern HMODULE g_hModule;
extern HWND g_hWnd;
extern DWORD g_referenceCount;

void TranslucentFlyoutsLib::Startup()
{
	Detours::Begin();
	//
	TranslucentFlyoutsLib::Win32HookStartup();
	//TranslucentFlyoutsLib::WRHookStartup();
	//
	Detours::Commit();
}

void TranslucentFlyoutsLib::Shutdown()
{
	Detours::Begin();
	//
	TranslucentFlyoutsLib::Win32HookShutdown();
	//TranslucentFlyoutsLib::WRHookShutdown();
	//
	Detours::Commit();
}

void TranslucentFlyoutsLib::OnWindowsCreated(HWND hWnd)
{
	if (IsAllowTransparent())
	{
		if (IsPopupMenuFlyout(hWnd) and GetCurrentFlyoutPolicy() & PopupMenu)
		{
			g_hWnd = hWnd;
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

void TranslucentFlyoutsLib::OnWindowHid(HWND hWnd)
{
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

		if (dwEvent == EVENT_OBJECT_HIDE)
		{
			OnWindowHid(hWnd);
		}
	}
}