#pragma once
#include "pch.h"

namespace TranslucentFlyouts::Framework
{
	void Startup();
	void Shutdown();
	void Prepare();
	void Update();
	void CALLBACK HandleWinEvent(
		HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
		LONG idObject, LONG idChild,
		DWORD dwEventThread, DWORD dwmsEventTime
	);
}