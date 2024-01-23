#pragma once
#include "framework.h"
#include "winrt.hpp"
#include "DiagnosticsHandler.hpp"

namespace TranslucentFlyouts::CommonFlyoutsHandler
{
	// client side
	void Startup();
	void Shutdown();
	void Update();
	void OnVisualTreeChanged(IInspectable* element, DiagnosticsHandler::FrameworkType framework, DiagnosticsHandler::MutationType mutation);
	// server side
	void OnRegistryItemsChanged();
}