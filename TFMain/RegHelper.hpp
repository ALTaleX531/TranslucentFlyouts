#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace RegHelper
	{
		DWORD GetDword(std::wstring_view subItemName, std::wstring_view valueName, DWORD defaultValue, bool useFallback = true);
		std::optional<DWORD> TryGetDword(std::wstring_view subItemName, std::wstring_view valueName, bool useFallback = true);
		
		DWORD64 _GetQword(std::wstring_view valueName, DWORD64 defaultValue);
		void _SetQword(std::wstring_view valueName, DWORD64 value);
		std::optional<std::wstring> _TryGetString(std::wstring_view valueName);
		void _SetString(std::wstring_view valueName, std::wstring_view value);
	}
}

