#pragma once
#include "pch.h"

namespace TranslucentFlyouts::TraverseLogHooks
{
	void Prepare();
	void Startup();
	void Shutdown();
	void EnableHooks(bool enable);
	void DisableHooks();
}