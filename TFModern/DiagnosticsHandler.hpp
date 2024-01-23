#pragma once
#include "pch.h"

namespace TranslucentFlyouts::DiagnosticsHandler
{
	void Startup();
	void Shutdown();
	void Prepare();

	enum class MutationType
	{
		Add,
		Remove
	};
	enum class FrameworkType
	{
		UWP,
		WinUI
	};
	HMODULE GetMUXHandle();
	bool IsMUXModule(HMODULE moduleHandle);
	inline void OnVisualTreeChanged(IInspectable* element, FrameworkType framework, MutationType mutation);
}