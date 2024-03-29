﻿#include "pch.h"
#include "resource.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "Framework.hpp"
#include "DwmThumbnailAPI.hpp"
#include "Application.hpp"
#include "MenuHandler.hpp"
#include "TooltipHandler.hpp"
#include "DropDownHandler.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::Framework
{
	std::array g_winEventCallbacks
	{
		MenuHandler::HandleWinEvent,
		TooltipHandler::HandleWinEvent,
		DropDownHandler::HandleWinEvent
	};
	std::array g_startupRoutines
	{
		MenuHandler::Startup,
		TooltipHandler::Startup,
		DropDownHandler::Startup
	};
	std::array g_shutdownRoutines
	{
		MenuHandler::Shutdown,
		TooltipHandler::Shutdown,
		DropDownHandler::Shutdown
	};
	std::array g_prepareRoutines
	{
		MenuHandler::Prepare,
		TooltipHandler::Prepare,
		DropDownHandler::Prepare
	};
	std::array g_updateRoutines
	{
		MenuHandler::Update,
		TooltipHandler::Update,
		DropDownHandler::Update
	};
	
	bool g_startup{false};

	void DoExplorerCrashCheck()
	{
		static std::chrono::steady_clock::time_point g_lastExplorerDied{ std::chrono::steady_clock::time_point{} - std::chrono::seconds(30) };
		static DWORD g_lastExplorerPid
		{ 
			[]
			{
				DWORD explorerPid{ 0 };
				GetWindowThreadProcessId(GetShellWindow(), &explorerPid);

				return explorerPid;
			} ()
		};
		
		DWORD explorerPid{ 0 };
		GetWindowThreadProcessId(GetShellWindow(), &explorerPid);

		if (explorerPid)
		{
			g_lastExplorerPid = explorerPid;
		}

		// Died twice in a short time!
		if (g_lastExplorerPid && explorerPid == 0)
		{
			const auto currentTimePoint{ std::chrono::steady_clock::now() };

			if (currentTimePoint < g_lastExplorerDied + std::chrono::seconds(30)) [[unlikely]]
			{
				Application::UninstallHook();

				if (
					MessageBoxW(
						nullptr,
						Utils::GetResWString<IDS_STRING109>().c_str(),
						nullptr,
						MB_ICONERROR | MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION | MB_SETFOREGROUND | MB_YESNO
					) == IDNO
				)
				{
					Application::InstallHook();
				}
				else
				{
					std::thread{ [&]
					{
						Application::StopService();
					} }.detach();
				}
				return;
			}

			g_lastExplorerDied = currentTimePoint;
			g_lastExplorerPid = 0;
		}
	}
}

void CALLBACK Framework::HandleWinEvent(
	HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
	LONG idObject, LONG idChild,
	DWORD dwEventThread, DWORD dwmsEventTime
)
{
	if (Api::IsHostProcess(Application::g_serviceName))
	{
		DoExplorerCrashCheck();
		return;
	}
	if (!g_startup)
	{
		return;
	}
	Update();

	DWORD processId{ 0 };
	GetWindowThreadProcessId(hWnd, &processId);
	if(
		idObject != OBJID_WINDOW ||
		idChild != CHILDID_SELF ||
		!hWnd || !IsWindow(hWnd) ||
		processId != GetCurrentProcessId() ||
		dwEventThread != GetCurrentThreadId()
	)
	{
		return;
	}

	for (auto callback : g_winEventCallbacks)
	{
		callback(hWinEventHook, dwEvent, hWnd, idObject, idChild, dwEventThread, dwmsEventTime);
	}
}

void Framework::Startup()
{
	if (g_startup)
	{
		return;
	}

	LOG_IF_FAILED(DwmThumbnailAPI::Initialize());
	for (auto startup : g_startupRoutines)
	{
		startup();
	}

	g_startup = true;
}

void Framework::Shutdown()
{
	if (!g_startup)
	{
		return;
	}

	for (auto shutdown : g_shutdownRoutines)
	{
		shutdown();
	}

	g_startup = false;
}

void Framework::Prepare()
{
	for (auto prepare : g_prepareRoutines)
	{
		prepare();
	}
}

void Framework::Update()
{
	for (auto update : g_updateRoutines)
	{
		update();
	}
}