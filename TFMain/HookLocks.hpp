#pragma once
#include "pch.h"

namespace TranslucentFlyouts::ExplorerFrameHooks
{
	inline wil::srwlock g_lock{};
}
namespace TranslucentFlyouts::RibbonHooks
{
	inline wil::srwlock g_lock{};
}