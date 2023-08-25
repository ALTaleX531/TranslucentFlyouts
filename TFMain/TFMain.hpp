#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace TFMain
	{
		class InteractiveIO
		{
		public:
			~InteractiveIO();

			enum class StringType
			{
				Notification,
				Warning,
				Error
			};
			enum class WaitType
			{
				NoWait,
				WaitYN,
				WaitAnyKey
			};

			// Return true/false if waitType is YN, otherwise always return true.
			bool OutputString(
				StringType strType,
				WaitType waitType,
				UINT strResourceId,
				std::wstring_view prefixStr,
				std::wstring_view additionalStr,
				bool requireConsole = false
			) const;

		private:
			static void Startup();
			static void Shutdown();
		};

		void CALLBACK HandleWinEvent(
			HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
			LONG idObject, LONG idChild,
			DWORD dwEventThread, DWORD dwmsEventTime
		);

		using Callback = std::function<void(HWND, DWORD)>;
		void AddCallback(Callback callback);
		void DeleteCallback(Callback callback);

		void Startup();
		void Shutdown();

		void Prepare();

		static constexpr DWORD lightMode_GradientColor{ 0x9EDDDDDD };
		static constexpr DWORD darkMode_GradientColor{ 0x412B2B2B };
		void ApplyBackdropEffect(std::wstring_view keyName, HWND hWnd, bool darkMode, DWORD darkMode_GradientColor, DWORD lightMode_GradientColor);
		HRESULT ApplyRoundCorners(std::wstring_view keyName, HWND hWnd);
		HRESULT ApplySysBorderColors(std::wstring_view keyName, HWND hWnd, bool useDarkMode, DWORD darkMode_BorderColor, DWORD lightMode_BorderColor);
	}
}