#pragma once
#include "pch.h"
#include "TFMain.hpp"

namespace TranslucentFlyouts
{
	// A class that aims to patch the calls to related Theme APIs within UxTheme.dll

	namespace UxThemePatcher
	{
		void Prepare(const TFMain::InteractiveIO& io);

		void Startup();
		void Shutdown();
	}
}