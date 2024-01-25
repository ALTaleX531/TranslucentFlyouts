#include "pch.h"
#include "HookHelper.hpp"
#include "SystemHelper.hpp"
#include "Framework.hpp"
#include "DiagnosticsHooks.hpp"
#include "DiagnosticsHandler.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::DiagnosticsHandler
{
	namespace WUX
	{
		MIDL_INTERFACE("f96d6d82-6e05-4c67-bc47-7cd8f7b40297")
		IXamlTestHooks : public IUnknown
		{
		};
		using IXamlTestHooks_GetAllXamlRoots = HRESULT (STDMETHODCALLTYPE*)(IXamlTestHooks* This, winrt::impl::abi_t<winrt::Windows::Foundation::Collections::IVectorView<winrt::impl::abi_t<winrt::Windows::UI::Xaml::IXamlRoot>*>>** xamlRoots);
		constexpr size_t IXamlTestHooks_GetAllXamlRoots_VTableIndex{ 138 };
		constexpr size_t IXamlTestHooks_GetAllXamlRoots_VTableIndex_win11_21h2{ 135 };
		constexpr size_t IXamlTestHooks_GetAllXamlRoots_VTableIndex_win10{ 133 };

		MIDL_INTERFACE("3b1f9832-6a1c-4fb4-afe3-3bc92159acda")
		IDxamlCoreTestHooks : public IInspectable
		{
		};

		MIDL_INTERFACE("91be536b-9599-428b-9a72-0618f28019e8")
		IDxamlCoreTestHooksStatics : public IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE GetForCurrentThread(
				IDxamlCoreTestHooks **ppResult
			) = 0;
		};

		MIDL_INTERFACE("412b49d7-b8b7-416a-b49b-57f9edbef991")
		IXamlIsland : public IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE get_AppContent(
				IInspectable * *app_content
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE get_Content(
				winrt::impl::abi_t<winrt::Windows::UI::Xaml::IUIElement>** element
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE put_Content(
				winrt::impl::abi_t<winrt::Windows::UI::Xaml::IUIElement>* element
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE get_FocusController(
				IInspectable** focus_controller
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE get_MaterialProperties(
				IInspectable** material_properties
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE put_MaterialProperties(
				IInspectable* material_properties
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE SetScreenOffsetOverride(
				winrt::Windows::Foundation::Point offset_on_screen
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE SetFocus() = 0;
		};

		MIDL_INTERFACE("3ead2336-b073-456f-bcaf-82587eb63487")
		IXamlIslandStatics : public IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE GetIslandFromElement(
				winrt::impl::abi_t<winrt::Windows::UI::Xaml::IDependencyObject> * pElement,
				IXamlIsland * *ppResult
			) = 0;
		};

		MIDL_INTERFACE("b3ab45d8-6a4e-4e76-a00d-32d4643a9f1a")
		IFrameworkApplicationPrivate : public IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE StartOnCurrentThread(winrt::impl::abi_t<winrt::Windows::UI::Xaml::ApplicationInitializationCallback> * callback) = 0;
			virtual HRESULT STDMETHODCALLTYPE CreateIsland(IXamlIsland** island) = 0;
			virtual HRESULT STDMETHODCALLTYPE CreateIslandWithAppWindow(IInspectable* app_window, IXamlIsland** island) = 0;
			virtual HRESULT STDMETHODCALLTYPE CreateIslandWithContentBridge(
				IInspectable* owner,
				IInspectable* content_bridge,
				IXamlIsland** island
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE RemoveIsland(IXamlIsland* island) = 0;
			virtual HRESULT STDMETHODCALLTYPE SetSynchronizationWindow(HWND hwnd) = 0;
		};

		bool WalkVisualTree(winrt::Windows::UI::Xaml::DependencyObject element, const std::function<bool(winrt::Windows::UI::Xaml::DependencyObject element)>& callback)
		{
			using VisualTreeHelper = winrt::Windows::UI::Xaml::Media::VisualTreeHelper;

			callback(element);
			for (int32_t index{0}, childCount = VisualTreeHelper::GetChildrenCount(element); index < childCount; index++)
			{
				auto child{ VisualTreeHelper::GetChild(element, index) };

				if (!WalkVisualTree(child, callback))
				{
					return false;
				}
			}

			return true;
		}
	}

	namespace MUX
	{
		MIDL_INTERFACE("43d4bcbd-4f02-4651-9ecc-dcfec9f786a7")
		IXamlTestHooks : public IUnknown
		{
		};
		using IXamlTestHooks_GetAllXamlRoots = HRESULT(STDMETHODCALLTYPE*)(IXamlTestHooks* This, winrt::impl::abi_t<winrt::Windows::Foundation::Collections::IVectorView<winrt::impl::abi_t<winrt::Microsoft::UI::Xaml::IXamlRoot>*>>** xamlRoots);
		constexpr size_t IXamlTestHooks_GetAllXamlRoots_VTableIndex{ 130 };

		MIDL_INTERFACE("61615723-8486-4376-84d5-27d0ff539580")
		IDxamlCoreTestHooks : public IInspectable
		{
		};

		MIDL_INTERFACE("84118843-ff81-55aa-9153-bca77f03a774")
		IDxamlCoreTestHooksStatics : public IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE GetForCurrentThread(
				IDxamlCoreTestHooks * *ppResult
			) = 0;
		};

		MIDL_INTERFACE("c223b4d3-2a18-5f61-bdb8-f90d7fee9a8f")
		IXamlIsland : public IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE get_Content(
				winrt::impl::abi_t<winrt::Microsoft::UI::Xaml::IUIElement> **element
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE put_Content(
				winrt::impl::abi_t<winrt::Microsoft::UI::Xaml::IUIElement>* element
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE get_FocusController(
				IInspectable** focus_controller
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE SetScreenOffsetOverride(
				winrt::Windows::Foundation::Point offset_on_screen
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE TrySetFocus() = 0;
		};

		MIDL_INTERFACE("b3d608be-c816-469b-b645-9679b55717c7")
		IXamlIslandStatics : public IInspectable
		{
			virtual HRESULT STDMETHODCALLTYPE GetIslandFromElement(
				winrt::impl::abi_t<winrt::Microsoft::UI::Xaml::IDependencyObject> * pElement,
				IXamlIsland * *ppResult
			) = 0;
		};

		MIDL_INTERFACE("0c858b8b-3e6d-55f5-a221-673af73c19b3")
		IFrameworkApplicationPrivate : public IInspectable
		{
			virtual HRESULT get_Windows(winrt::impl::abi_t<winrt::Windows::Foundation::Collections::IVectorView<winrt::impl::abi_t<winrt::Microsoft::UI::Xaml::IWindow>*>> **ppValue) = 0;
			virtual HRESULT StartOnCurrentThread(winrt::impl::abi_t<winrt::Microsoft::UI::Xaml::ApplicationInitializationCallback>* callback) = 0;
			virtual HRESULT CreateIsland(IXamlIsland** island) = 0;
			virtual HRESULT CreateIslandWithContentBridge(
				IInspectable* owner,
				IInspectable* content_bridge,
				IXamlIsland** island
			) = 0;
			virtual HRESULT RemoveIsland(IXamlIsland* island) = 0;
			virtual HRESULT SetSynchronizationWindow(HWND hwnd) = 0;
		};

		bool WalkVisualTree(winrt::Microsoft::UI::Xaml::DependencyObject element, const std::function<bool(winrt::Microsoft::UI::Xaml::DependencyObject element)>& callback)
		{
			using VisualTreeHelper = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper;

			callback(element);
			for (int32_t index{ 0 }, childCount = VisualTreeHelper::GetChildrenCount(element); index < childCount; index++)
			{
				auto child{ VisualTreeHelper::GetChild(element, index) };

				if (!WalkVisualTree(child, callback))
				{
					return false;
				}
			}

			return true;
		}
	}
}

void DiagnosticsHandler::Prepare()
{
	DiagnosticsHooks::Prepare();
}

void DiagnosticsHandler::Startup()
{
	DiagnosticsHooks::Startup();

	HookHelper::ThreadSnapshotExcludeSelf threads{};
	threads.Suspend();
	
	auto wuxModule{ GetModuleHandleW(L"Windows.UI.Xaml.dll") };
	if (wuxModule && DiagnosticsHooks::WUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId)
	{
		threads.ForEach([](const PSS_THREAD_ENTRY& threadEntry)
		{
			winrt::Windows::System::DispatcherQueue dispatcherQueue{nullptr};
			DiagnosticsHooks::WUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId(
				nullptr,
				reinterpret_cast<winrt::impl::abi_t<winrt::Windows::System::IDispatcherQueue>**>(winrt::put_abi(dispatcherQueue)),
				threadEntry.ThreadId
			);
			if (dispatcherQueue)
			{
				dispatcherQueue.TryEnqueue([]
				{
					try
					{
						auto name{ winrt::hstring(L"Windows.UI.Xaml.DxamlCoreTestHooks") };
						winrt::com_ptr<WUX::IDxamlCoreTestHooksStatics> dxamlCoreTestHooksStatic{ winrt::get_activation_factory<WUX::IDxamlCoreTestHooksStatics>(name) };

						winrt::com_ptr<WUX::IDxamlCoreTestHooks> dxamlCoreTestHooks{ nullptr };
						THROW_IF_FAILED(dxamlCoreTestHooksStatic->GetForCurrentThread(dxamlCoreTestHooks.put()));

						auto xamlTestHooks{ dxamlCoreTestHooks.as<WUX::IXamlTestHooks>() };
						winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::UI::Xaml::XamlRoot> xamlRootCollection{};
						
						WUX::IXamlTestHooks_GetAllXamlRoots ptr{ nullptr };
						if (SystemHelper::g_buildNumber >= 22621)
						{
							ptr = reinterpret_cast<decltype(ptr)>(HookHelper::GetObjectVTable(xamlTestHooks.get())[WUX::IXamlTestHooks_GetAllXamlRoots_VTableIndex]);
						}
						else if (SystemHelper::g_buildNumber >= 22000)
						{
							ptr = reinterpret_cast<decltype(ptr)>(HookHelper::GetObjectVTable(xamlTestHooks.get())[WUX::IXamlTestHooks_GetAllXamlRoots_VTableIndex_win11_21h2]);
						}
						else if (SystemHelper::g_buildNumber < 22000)
						{
							ptr = reinterpret_cast<decltype(ptr)>(HookHelper::GetObjectVTable(xamlTestHooks.get())[WUX::IXamlTestHooks_GetAllXamlRoots_VTableIndex_win10]);
						}
						THROW_IF_FAILED(
							ptr(
								xamlTestHooks.get(),
								reinterpret_cast<winrt::impl::abi_t<winrt::Windows::Foundation::Collections::IVectorView<winrt::impl::abi_t<winrt::Windows::UI::Xaml::IXamlRoot>*>>**>(
									winrt::put_abi(xamlRootCollection)
								)
							)
						);

						for (auto xamlRoot : xamlRootCollection)
						{
							WUX::WalkVisualTree(xamlRoot.Content(), [](winrt::Windows::UI::Xaml::DependencyObject element)
							{
								if (element)
								{
									OnVisualTreeChanged(reinterpret_cast<IInspectable*>(winrt::get_abi(element)), FrameworkType::UWP, MutationType::Add);
								}
								return true;
							});
						}
					}
					catch (...)
					{

					}
				});

#ifdef _DEBUG
				OutputDebugStringW(
					std::format(
						L"[WUX] Get DispatcherQueue of {}\n",
						threadEntry.ThreadId
					).c_str()
				);
#endif
			}
			return true;
		});
	}
	
	auto muxModule{ GetMUXHandle() };
	if (muxModule && DiagnosticsHooks::MUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId)
	{
		threads.ForEach([](const PSS_THREAD_ENTRY& threadEntry)
		{
			winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcherQueue{ nullptr };
			DiagnosticsHooks::MUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId(
				nullptr,
				reinterpret_cast<winrt::impl::abi_t<winrt::Microsoft::UI::Dispatching::IDispatcherQueue>**>(winrt::put_abi(dispatcherQueue)),
				threadEntry.ThreadId
			);
			if (dispatcherQueue)
			{
				dispatcherQueue.TryEnqueue([]()
				{
					try
					{
						auto name{ winrt::hstring(L"Microsoft.UI.Xaml.DxamlCoreTestHooks") };
						winrt::com_ptr<MUX::IDxamlCoreTestHooksStatics> dxamlCoreTestHooksStatic{ winrt::get_activation_factory<MUX::IDxamlCoreTestHooksStatics>(name) };

						winrt::com_ptr<MUX::IDxamlCoreTestHooks> dxamlCoreTestHooks{ nullptr };
						THROW_IF_FAILED(dxamlCoreTestHooksStatic->GetForCurrentThread(dxamlCoreTestHooks.put()));

						auto xamlTestHooks{ dxamlCoreTestHooks.as<MUX::IXamlTestHooks>() };
						winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::UI::Xaml::XamlRoot> xamlRootCollection{};

						MUX::IXamlTestHooks_GetAllXamlRoots ptr{ reinterpret_cast<decltype(ptr)>(HookHelper::GetObjectVTable(xamlTestHooks.get())[MUX::IXamlTestHooks_GetAllXamlRoots_VTableIndex]) };
						THROW_IF_FAILED(
							ptr(
								xamlTestHooks.get(),
								reinterpret_cast<winrt::impl::abi_t<winrt::Windows::Foundation::Collections::IVectorView<winrt::impl::abi_t<winrt::Microsoft::UI::Xaml::IXamlRoot>*>>**>(
									winrt::put_abi(xamlRootCollection)
								)
							)
						);

						for (auto xamlRoot : xamlRootCollection)
						{
							MUX::WalkVisualTree(xamlRoot.Content(), [](winrt::Microsoft::UI::Xaml::DependencyObject element)
							{
								if (element)
								{
									OnVisualTreeChanged(reinterpret_cast<IInspectable*>(winrt::get_abi(element)), FrameworkType::WinUI, MutationType::Add);
								}
								return true;
							});
						}
					}
					catch (...)
					{

					}
				});

#ifdef _DEBUG
				OutputDebugStringW(
					std::format(
						L"[MUX] Get DispatcherQueue of {}\n",
						threadEntry.ThreadId
					).c_str()
				);
#endif
			}
			return true;
		});
	}

	threads.Resume();
}

void DiagnosticsHandler::Shutdown()
{
	DiagnosticsHooks::Shutdown();
}

void DiagnosticsHandler::OnVisualTreeChanged(IInspectable* element, FrameworkType framework, MutationType mutation)
{
	Framework::OnVisualTreeChanged(element, framework, mutation);
	
	winrt::hstring className{};
	if (FAILED(element->GetRuntimeClassName(reinterpret_cast<HSTRING*>(winrt::put_abi(className)))))
	{
		return;
	}

#ifdef _DEBUG
	OutputDebugStringW(
		std::format(
			L"[{}] object {}: [{}]\n",
			framework == FrameworkType::UWP ? L"UWP" : L"WinUI",
			mutation == MutationType::Add ? L"Added" : L"Removed",
			className.c_str()
		).c_str()
	);
#endif
}

bool DiagnosticsHandler::IsMUXModule(HMODULE moduleHandle)
{
	if (!moduleHandle)
	{
		return false;
	}

	WCHAR moduleBaseName[MAX_PATH + 1];
	if (!GetModuleBaseNameW(GetCurrentProcess(), moduleHandle, moduleBaseName, MAX_PATH))
	{
		return false;
	}

	if (_wcsicmp(moduleBaseName, L"Microsoft.UI.Xaml.dll") != 0)
	{
		return false;
	}

	if (!GetProcAddress(moduleHandle, "OverrideXamlMetadataProvider"))
	{
		return false;
	}

	return true;
}
HMODULE DiagnosticsHandler::GetMUXHandle()
{
	HMODULE muxModule{ nullptr };
	DWORD bytesNeeded{ 0 };
	if (!EnumProcessModules(GetCurrentProcess(), nullptr, 0, &bytesNeeded))
	{
		return muxModule;
	}
	DWORD moduleCount{ bytesNeeded / sizeof(HMODULE) };
	auto moduleList{ std::make_unique<HMODULE[]>(moduleCount) };
	if (!EnumProcessModules(GetCurrentProcess(), moduleList.get(), bytesNeeded, &bytesNeeded))
	{
		return muxModule;
	}

	for (DWORD i = 0; i < moduleCount; i++)
	{
		HMODULE moduleHandle{ moduleList[i] };

		if (!IsMUXModule(moduleHandle))
		{
			continue;
		}

		muxModule = moduleHandle;
		break;
	}

	return muxModule;
}