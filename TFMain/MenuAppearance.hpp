#pragma once
#include "pch.h"

namespace TranslucentFlyouts::MenuAppearance
{
	constexpr DWORD lightMode_HotColor{ 0x30000000 };
	constexpr DWORD darkMode_HotColor{ 0x41808080 };

	constexpr DWORD lightMode_DisabledHotColor{ 0x00000000 };
	constexpr DWORD darkMode_DisabledHotColor{ 0x00000000 };

	constexpr DWORD lightMode_SeparatorColor{ 0x30262626 };
	constexpr DWORD darkMode_SeparatorColor{ 0x30D9D9D9 };

	constexpr DWORD lightMode_FocusingColor{ 0xFF000000 };
	constexpr DWORD darkMode_FocusingColor{ 0xFFFFFFFF };

	constexpr DWORD focusingWidth{ 1000 };
	constexpr DWORD cornerRadius{ 8 };
	constexpr DWORD separatorWidth{ 1000 };

	constexpr int systemOutlineSize{ 1 };
	constexpr int nonClientMarginStandardSize{ 3 };
}