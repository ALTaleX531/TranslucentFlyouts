#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace TFMain
	{
		using Callback = std::function<void(HWND, DWORD)>;

		void CALLBACK HandleWinEvent(
			HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
			LONG idObject, LONG idChild,
			DWORD dwEventThread, DWORD dwmsEventTime
		);

		void AddCallback(Callback callback);
		void DeleteCallback(Callback callback);
		
		void Startup();
		void Shutdown();

		static constexpr DWORD lightMode_GradientColor{ 0x9EDDDDDD };
		static constexpr DWORD darkMode_GradientColor{ 0x412B2B2B };
		void ApplyBackdropEffect(std::wstring_view keyName, HWND hWnd, bool darkMode, DWORD darkMode_GradientColor, DWORD lightMode_GradientColor);
		HRESULT ApplyRoundCorners(std::wstring_view keyName, HWND hWnd);
		HRESULT ApplySysBorderColors(std::wstring_view keyName, HWND hWnd, bool useDarkMode, DWORD darkMode_BorderColor, DWORD lightMode_BorderColor);
	}
}