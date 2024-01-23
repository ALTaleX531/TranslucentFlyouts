#pragma once
#include "pch.h"

namespace TranslucentFlyouts::KbxLabelHooks
{
	void Prepare();
	void Startup();
	void Shutdown();
	void EnableHooks(bool enable);
	void DisableHooks();
}