#include "pch.h"
#include "Utils.hpp"
#include "Framework.hpp"
#include "DiagnosticsHandler.hpp"
#include "CommonFlyoutsHandler.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::CommonFlyoutsHandler
{
#pragma data_seg(".shared")
	inline struct CommonFlyoutsContext : Framework::ImmersiveContext
	{

	} g_commonFlyoutsContext{};
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")
	std::unordered_map<void*, winrt::com_ptr<IInspectable>> g_originalBrush{};

	void ReplaceBackgroundBrush(IInspectable* element)
	{
		// WUX
		{
			winrt::Windows::UI::Xaml::Controls::Control control{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Windows::UI::Xaml::Controls::Control>(), winrt::put_abi(control))))
			{
				g_originalBrush[winrt::get_abi(control)] = control.Background().as<IInspectable>();
				Framework::SetAcrylicBrushForBackground(control, &g_commonFlyoutsContext);
			}
		}
		{
			winrt::Windows::UI::Xaml::Controls::Panel panel{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Windows::UI::Xaml::Controls::Panel>(), winrt::put_abi(panel))))
			{
				g_originalBrush[winrt::get_abi(panel)] = panel.Background().as<IInspectable>();
				Framework::SetAcrylicBrushForBackground(panel, &g_commonFlyoutsContext);
			}
		}
		{
			winrt::Windows::UI::Xaml::Controls::Border border{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Windows::UI::Xaml::Controls::Border>(), winrt::put_abi(border))))
			{
				g_originalBrush[winrt::get_abi(border)] = border.Background().as<IInspectable>();
				Framework::SetAcrylicBrushForBackground(border, &g_commonFlyoutsContext);
			}
		}
		{
			winrt::Windows::UI::Xaml::Controls::ContentPresenter content{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Windows::UI::Xaml::Controls::ContentPresenter>(), winrt::put_abi(content))))
			{
				g_originalBrush[winrt::get_abi(content)] = content.Background().as<IInspectable>();
				Framework::SetAcrylicBrushForBackground(content, &g_commonFlyoutsContext);
			}
		}
		// MUX
		{
			winrt::Microsoft::UI::Xaml::Controls::Control control{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Microsoft::UI::Xaml::Controls::Control>(), winrt::put_abi(control))))
			{
				g_originalBrush[winrt::get_abi(control)] = control.Background().as<IInspectable>();
				Framework::SetAcrylicBrushForBackground(control, &g_commonFlyoutsContext);
			}
		}
		{
			winrt::Microsoft::UI::Xaml::Controls::Panel panel{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Microsoft::UI::Xaml::Controls::Panel>(), winrt::put_abi(panel))))
			{
				g_originalBrush[winrt::get_abi(panel)] = panel.Background().as<IInspectable>();
				Framework::SetAcrylicBrushForBackground(panel, &g_commonFlyoutsContext);
			}
		}
		{
			winrt::Microsoft::UI::Xaml::Controls::Border border{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Microsoft::UI::Xaml::Controls::Border>(), winrt::put_abi(border))))
			{
				g_originalBrush[winrt::get_abi(border)] = border.Background().as<IInspectable>();
				Framework::SetAcrylicBrushForBackground(border, &g_commonFlyoutsContext);
			}
		}
		{
			winrt::Microsoft::UI::Xaml::Controls::ContentPresenter content{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Microsoft::UI::Xaml::Controls::ContentPresenter>(), winrt::put_abi(content))))
			{
				g_originalBrush[winrt::get_abi(content)] = content.Background().as<IInspectable>();
				Framework::SetAcrylicBrushForBackground(content, &g_commonFlyoutsContext);
			}
		}
	}
	bool RestoreBackgroundBrush(IInspectable* element)
	{
		if (g_originalBrush.find(element) == g_originalBrush.end())
		{
			return false;
		}
		// WUX
		{
			winrt::Windows::UI::Xaml::Controls::Control control{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Windows::UI::Xaml::Controls::Control>(), winrt::put_abi(control))))
			{
				control.Background(g_originalBrush[element].as<winrt::Windows::UI::Xaml::Media::Brush>());
			}
		}
		{
			winrt::Windows::UI::Xaml::Controls::Panel panel{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Windows::UI::Xaml::Controls::Panel>(), winrt::put_abi(panel))))
			{
				panel.Background(g_originalBrush[element].as<winrt::Windows::UI::Xaml::Media::Brush>());
			}
		}
		{
			winrt::Windows::UI::Xaml::Controls::Border border{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Windows::UI::Xaml::Controls::Border>(), winrt::put_abi(border))))
			{
				border.Background(g_originalBrush[element].as<winrt::Windows::UI::Xaml::Media::Brush>());
			}
		}
		{
			winrt::Windows::UI::Xaml::Controls::ContentPresenter content{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Windows::UI::Xaml::Controls::ContentPresenter>(), winrt::put_abi(content))))
			{
				content.Background(g_originalBrush[element].as<winrt::Windows::UI::Xaml::Media::Brush>());
			}
		}
		// MUX
		{
			winrt::Microsoft::UI::Xaml::Controls::Control control{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Microsoft::UI::Xaml::Controls::Control>(), winrt::put_abi(control))))
			{
				control.Background(g_originalBrush[element].as<winrt::Microsoft::UI::Xaml::Media::Brush>());
			}
		}
		{
			winrt::Microsoft::UI::Xaml::Controls::Panel panel{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Microsoft::UI::Xaml::Controls::Panel>(), winrt::put_abi(panel))))
			{
				panel.Background(g_originalBrush[element].as<winrt::Microsoft::UI::Xaml::Media::Brush>());
			}
		}
		{
			winrt::Microsoft::UI::Xaml::Controls::Border border{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Microsoft::UI::Xaml::Controls::Border>(), winrt::put_abi(border))))
			{
				border.Background(g_originalBrush[element].as<winrt::Microsoft::UI::Xaml::Media::Brush>());
			}
		}
		{
			winrt::Microsoft::UI::Xaml::Controls::ContentPresenter content{ nullptr };
			if (SUCCEEDED(element->QueryInterface(winrt::guid_of<winrt::Microsoft::UI::Xaml::Controls::ContentPresenter>(), winrt::put_abi(content))))
			{
				content.Background(g_originalBrush[element].as<winrt::Microsoft::UI::Xaml::Media::Brush>());
			}
		}
		return true;
	}
}

void CommonFlyoutsHandler::Startup()
{
}
void CommonFlyoutsHandler::Shutdown()
{
	/*for (auto [element, brush] : g_originalBrush)
	{
		if (!Utils::IsBadReadPtr(element))
		{
			RestoreBackgroundBrush(reinterpret_cast<IInspectable*>(element));
		}
	}*/
	g_originalBrush.clear();
}
void CommonFlyoutsHandler::Update()
{
}

void CommonFlyoutsHandler::OnVisualTreeChanged(IInspectable* element, DiagnosticsHandler::FrameworkType framework, DiagnosticsHandler::MutationType mutation)
{
	winrt::hstring className{};
	if (FAILED(element->GetRuntimeClassName(reinterpret_cast<HSTRING*>(winrt::put_abi(className)))))
	{
		return;
	}

	if (mutation == DiagnosticsHandler::MutationType::Add)
	{
		if (
			className == winrt::name_of<winrt::Microsoft::UI::Xaml::Controls::Primitives::CommandBarFlyoutCommandBar>() ||
			className == winrt::name_of<winrt::Windows::UI::Xaml::Controls::Primitives::CommandBarFlyoutCommandBar>() ||
			className == winrt::name_of<winrt::Windows::UI::Xaml::Controls::MenuFlyoutPresenter>() ||
			className == winrt::name_of<winrt::Windows::UI::Xaml::Controls::CommandBarOverflowPresenter>() ||
			className == winrt::name_of<winrt::Windows::UI::Xaml::Controls::ToolTip>() ||
			className == winrt::name_of<winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutPresenter>() ||
			className == winrt::name_of<winrt::Microsoft::UI::Xaml::Controls::CommandBarOverflowPresenter>() ||
			className == winrt::name_of<winrt::Microsoft::UI::Xaml::Controls::ToolTip>()
		)
		{
			ReplaceBackgroundBrush(element);
		}
	}
	else
	{
		if (RestoreBackgroundBrush(element))
		{
			g_originalBrush.erase(element);
		}
	}
}

void CommonFlyoutsHandler::OnRegistryItemsChanged()
{
	memcpy_s(&g_commonFlyoutsContext, sizeof(g_commonFlyoutsContext), &Framework::g_immersiveContext, sizeof(Framework::g_immersiveContext));
}