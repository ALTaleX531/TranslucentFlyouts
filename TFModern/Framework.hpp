#pragma once
#include "framework.h"
#include "winrt.hpp"
#include "DiagnosticsHandler.hpp"

namespace TranslucentFlyouts::Framework
{
#pragma data_seg(".shared")
	inline struct ImmersiveContext
	{
		bool disabled{ false };
		winrt::Windows::UI::Color darkMode_TintColor{ 255, 32, 32, 32 };
		winrt::Windows::UI::Color lightMode_TintColor{ 255, 243, 243, 243 };
		float darkMode_TintOpacity{ 0.1f };
		float lightMode_TintOpacity{ 0.1f };
		float darkMode_LuminosityOpacity{ 0.0f };
		float lightMode_LuminosityOpacity{ 0.0f };
		float darkMode_Opacity{ 1.0f };
		float lightMode_Opacity{ 1.0f };
		/*winrt::Windows::UI::Color darkMode_TintColor{ 255, 32, 32, 32 };
		winrt::Windows::UI::Color lightMode_TintColor{ 255, 243, 243, 243 };
		float darkMode_TintOpacity{ 0.3f };
		float lightMode_TintOpacity{ 0.15f };
		float darkMode_LuminosityOpacity{ 0.8f };
		float lightMode_LuminosityOpacity{ 0.8f };
		float darkMode_Opacity{ 1.0f };
		float lightMode_Opacity{ 1.0f };*/
	} g_immersiveContext{};
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")
	template <typename T>
	constexpr bool is_wux_object_v = std::is_base_of_v<winrt::impl::base_one<T, winrt::Windows::UI::Xaml::DependencyObject>, T>;

	template <typename ElementType>
	void SetAcrylicBrushForBackground(const ElementType& element, const ImmersiveContext* context)
	{
		bool isLight
		{
			static_cast<DWORD>(element.ActualTheme()) == 1
		};
		
		if constexpr (is_wux_object_v<ElementType>)
		{
			winrt::Windows::UI::Xaml::Media::AcrylicBrush acrylicBrush{};
			acrylicBrush.TintColor(isLight ? context->lightMode_TintColor : context->darkMode_TintColor);
			acrylicBrush.FallbackColor(isLight ? context->lightMode_TintColor : context->darkMode_TintColor);
			acrylicBrush.TintOpacity(isLight ? context->lightMode_TintOpacity : context->darkMode_TintOpacity);
			acrylicBrush.TintLuminosityOpacity(isLight ? context->lightMode_LuminosityOpacity : context->darkMode_LuminosityOpacity);
			acrylicBrush.Opacity(isLight ? context->lightMode_Opacity : context->darkMode_Opacity);
			element.Background(acrylicBrush);
		}
		else
		{
			winrt::Microsoft::UI::Xaml::Media::AcrylicBrush acrylicBrush{};
			acrylicBrush.TintColor(isLight ? context->lightMode_TintColor : context->darkMode_TintColor);
			acrylicBrush.FallbackColor(isLight ? context->lightMode_TintColor : context->darkMode_TintColor);
			acrylicBrush.TintOpacity(isLight ? context->lightMode_TintOpacity : context->darkMode_TintOpacity);
			acrylicBrush.TintLuminosityOpacity(isLight ? context->lightMode_LuminosityOpacity : context->darkMode_LuminosityOpacity);
			acrylicBrush.Opacity(isLight ? context->lightMode_Opacity : context->darkMode_Opacity);
			element.Background(acrylicBrush);
		}
	}

	void Startup();
	void Shutdown();
	void OnVisualTreeChanged(IInspectable* element, DiagnosticsHandler::FrameworkType framework, DiagnosticsHandler::MutationType mutation);

	void Prepare();
	void CALLBACK HandleWinEvent(
		HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
		LONG idObject, LONG idChild,
		DWORD dwEventThread, DWORD dwmsEventTime
	);
	void CleanUp();
}