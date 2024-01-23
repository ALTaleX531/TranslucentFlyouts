#pragma once
#include "pch.h"

namespace TranslucentFlyouts::MenuHooks
{
	void Prepare();
	void Startup();
	void Shutdown();
	void EnableHooks(bool enable);
	void EnableIconHooks(bool enable);
	void DisableHooks();

	inline thread_local HMODULE g_targetModule{ nullptr };
}