#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace RegHelper
	{
		DWORD GetDword(std::wstring_view subItemName, std::wstring_view valueName, DWORD defaultValue, bool useFallback = true);
	}
}

