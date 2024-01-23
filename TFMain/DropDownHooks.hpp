#pragma once
#include "pch.h"

namespace TranslucentFlyouts::DropDownHooks
{
	void Prepare();
	void Startup();
	void Shutdown();
	void EnableHooks(bool enable);
	void DisableHooks();
}