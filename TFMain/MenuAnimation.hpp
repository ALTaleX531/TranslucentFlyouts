#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace MenuAnimation
	{
		namespace 
		{
			using namespace std::chrono_literals;
			// Defined in win32kfull.sys!zzzMNFadeSelection
			constexpr std::chrono::milliseconds standardFadeoutDuration{350ms};
		}

		HRESULT CreateFadeOut(
			HWND hWnd,
			MENUBARINFO mbi,
			std::chrono::milliseconds duration
		);
	};
}