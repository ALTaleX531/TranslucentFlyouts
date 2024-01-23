#pragma once
#include "pch.h"

namespace TranslucentFlyouts::UxThemeHooks
{
	void Prepare();
	void Startup();
	void Shutdown();
	void EnableHooks(bool enable);
	void DisableHooks();
}