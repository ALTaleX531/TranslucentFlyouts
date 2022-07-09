#include "pch.h"
#include "SettingsHelper.h"

using namespace TranslucentFlyoutsLib;

#pragma data_seg("shared")
Settings TranslucentFlyoutsLib::g_settings = {};
#pragma data_seg()
#pragma comment(linker,"/SECTION:shared,RWS")