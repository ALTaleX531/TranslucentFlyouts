#pragma once
#include "pch.h"

namespace TranslucentFlyouts::ImmersiveHooks
{
	void Prepare();
	void Startup();
	void Shutdown();
	void EnableHooks(PVOID baseAddress, bool enable);
	void DisableHooks();
}