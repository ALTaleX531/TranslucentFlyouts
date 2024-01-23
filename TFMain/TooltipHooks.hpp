#pragma once
#include "pch.h"

namespace TranslucentFlyouts::TooltipHooks
{
	void Prepare();
	void Startup();
	void Shutdown();
	void EnableHooks(bool enable);
	void EnableMarginHooks(bool enable);
	void DisableHooks();
}