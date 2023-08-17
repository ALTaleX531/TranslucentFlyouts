#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	// A class that aims to patch the calls to related Theme APIs within UxTheme.dll

	namespace UxThemePatcher
	{
		void PrepareUxTheme();
		bool IsUxThemeAPIOffsetReady();
		void InitUxThemeOffset();

		void Startup();
		void Shutdown();
	}
}