#pragma once
#include "pch.h"

namespace TranslucentFlyouts::FlyoutAnimation
{
	namespace
	{
		using namespace std::chrono;
		using namespace std::chrono_literals;

		// Defined in win32kfull.sys!zzzMNFadeSelection
		constexpr milliseconds standardFadeoutDuration{ 350ms };

		// Defined in https://learn.microsoft.com/en-us/windows/apps/design/signature-experiences/motion
		constexpr milliseconds standardPopupInDuration{ 250ms };
		constexpr milliseconds standardFadeInDuration{ 87ms };
		// Defined in WinUI
		constexpr float standardStartPosRatio{ 0.5f };
	}

	HRESULT CreateMenuFadeOut(
		HWND hWnd,
		MENUBARINFO mbi,
		std::chrono::milliseconds duration
	);
	HRESULT CreateMenuPopupIn(
		HWND hWnd,
		float startPosRatio,
		std::chrono::milliseconds popInDuration,
		std::chrono::milliseconds fadeInDuration,
		DWORD animationStyle
	);
	HRESULT CreateDropDownPopupIn(
		HWND hWnd,
		float startPosRatio,
		std::chrono::milliseconds popInDuration,
		std::chrono::milliseconds fadeInDuration,
		DWORD animationStyle
	);
}