//		!!!IMPORTANT!!!
//
//		THIS FILE IS UNDER TESTING!!! WHICH IS NOT RESPONSIBLE FOR THE STABILITY!!!
//
//		!!!IMPORTANT!!!
#include "pch.h"
#include "DebugHelper.h"
#include "DetoursHelper.h"
#include "WRHookHelper.h"

//using namespace TranslucentFlyoutsLib;
//extern HMODULE g_hModule;
//
//DetoursHook TranslucentFlyoutsLib::IUIFramework_LoadUIHook;
//DetoursHook TranslucentFlyoutsLib::IUIFramework_InitializeHook;
//DetoursHook TranslucentFlyoutsLib::CoCreateInstanceHook("Combase", "CoCreateInstance", MyCoCreateInstance);
//// Windows Runtime
//DetoursHook TranslucentFlyoutsLib::RoGetActivationFactoryHook("Combase", "RoGetActivationFactory", MyRoGetActivationFactory);
//DetoursHook TranslucentFlyoutsLib::RoActivateInstanceHook("Combase", "RoActivateInstance", MyRoActivateInstance);
////
//DetoursHook TranslucentFlyoutsLib::IDesktopWindowXamlSourceFactory_CreateInstanceHook;
//DetoursHook TranslucentFlyoutsLib::IDesktopWindowXamlSourceNative_AttachToWindowHook;
//DetoursHook TranslucentFlyoutsLib::IDesktopWindowXamlSource_put_ContentHook;
////
//DetoursHook TranslucentFlyoutsLib::IMenuFlyoutFactory_CreateInstanceHook;
//DetoursHook TranslucentFlyoutsLib::IMenuFlyoutItemFactory_CreateInstanceHook;
//DetoursHook TranslucentFlyoutsLib::IMenuFlyoutSubItemFactory_ActivateInstanceHook;
//DetoursHook TranslucentFlyoutsLib::IToggleMenuFlyoutItemFactory_CreateInstanceHook;
//DetoursHook TranslucentFlyoutsLib::IMenuBarItemFlyoutFactory_CreateInstanceHook;
////
//DetoursHook TranslucentFlyoutsLib::ICommandBarFlyoutFactory_CreateInstanceHook;
//DetoursHook TranslucentFlyoutsLib::IAppBarButtonFactory_CreateInstanceHook;
//DetoursHook TranslucentFlyoutsLib::IAppBarToggleButtonFactory_CreateInstanceHook;
////
//DetoursHook TranslucentFlyoutsLib::IMenuFlyoutItem_put_TextHook;

//void TranslucentFlyoutsLib::WRHookStartup()
//{
//	/*Detours::Batch(
//	    TRUE,
//	    CoCreateInstanceHook,
//	    RoGetActivationFactoryHook,
//	    RoActivateInstanceHook
//	);*/
//}
//
//void TranslucentFlyoutsLib::WRHookShutdown()
//{
//	//Detours::Batch(
//	//    FALSE,
//	//    CoCreateInstanceHook,
//	//    IUIFramework_InitializeHook,
//	//    IUIFramework_LoadUIHook,
//	//    RoGetActivationFactoryHook,
//	//    RoActivateInstanceHook, // Reserved
//	//    //
//	//    IDesktopWindowXamlSourceFactory_CreateInstanceHook, // 用于测试
//	//    IDesktopWindowXamlSourceNative_AttachToWindowHook, // 用于测试
//	//    IDesktopWindowXamlSource_put_ContentHook, // 用于测试
//	//    //
//	//    IMenuFlyoutFactory_CreateInstanceHook, // 像任务栏右键弹出的主菜单
//	//    IMenuFlyoutItemFactory_CreateInstanceHook, // 像任务栏右键弹出的菜单项
//	//    IMenuFlyoutSubItemFactory_ActivateInstanceHook, // 像任务栏右键弹出的子菜单项
//	//    IToggleMenuFlyoutItemFactory_CreateInstanceHook, // 菜单栏弹出的展开子菜单项
//	//    IMenuBarItemFlyoutFactory_CreateInstanceHook, // 菜单栏弹出的菜单项
//	//    //
//	//    ICommandBarFlyoutFactory_CreateInstanceHook, // 文件管理器除桌面外的主菜单
//	//    IAppBarButtonFactory_CreateInstanceHook, // 文件管理器除桌面外的菜单项
//	//    IAppBarToggleButtonFactory_CreateInstanceHook, // 文件管理器除桌面外的展开子菜单项
//	//    //
//	//    IMenuFlyoutItem_put_TextHook // 用于整活和测试
//	//);
//}
//
//HRESULT WINAPI TranslucentFlyoutsLib::MyCoCreateInstance(
//    REFCLSID  rclsid,
//    LPUNKNOWN pUnkOuter,
//    DWORD     dwClsContext,
//    REFIID    riid,
//    LPVOID *ppv
//)
//{
//	HRESULT hr = S_OK;
//	hr = CoCreateInstanceHook.OldFunction<decltype(MyCoCreateInstance)>(
//	         rclsid,
//	         pUnkOuter,
//	         dwClsContext,
//	         riid,
//	         ppv
//	     );
//	if (SUCCEEDED(hr))
//	{
//		if (!IUIFramework_InitializeHook.IsHookInstalled())
//		{
//			try
//			{
//				ComPtr<::IUIFramework> pFramework;
//				ThrowIfFailed(
//				    CoCreateInstanceHook.OldFunction<decltype(MyCoCreateInstance)>(
//				        CLSID_UIRibbonFramework,
//				        nullptr,
//				        CLSCTX_INPROC_SERVER,
//				        IID_PPV_ARGS(&pFramework)
//				    )
//				);
//				IUIFramework_InitializeHook.Initialize(((IUIFramework*)pFramework.Get())->lpVtbl->Initialize, MyIUIFramework_Initialize);
//				Detours::Begin();
//				IUIFramework_InitializeHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...) { }
//		}
//		if (!IUIFramework_LoadUIHook.IsHookInstalled())
//		{
//			try
//			{
//				ComPtr<::IUIFramework> pFramework;
//				ThrowIfFailed(
//				    CoCreateInstanceHook.OldFunction<decltype(MyCoCreateInstance)>(
//				        CLSID_UIRibbonFramework,
//				        nullptr,
//				        CLSCTX_INPROC_SERVER,
//				        IID_PPV_ARGS(&pFramework)
//				    )
//				);
//				IUIFramework_LoadUIHook.Initialize(((IUIFramework *)pFramework.Get())->lpVtbl->LoadUI, MyIUIFramework_LoadUI);
//				Detours::Begin();
//				IUIFramework_LoadUIHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIUIFramework_LoadUI(
//    ::IUIFramework *This,
//    HINSTANCE instance,
//    LPCWSTR resourceName
//)
//{
//	TCHAR pszModule[MAX_PATH + 1];
//	GetModuleFileName(instance, pszModule, MAX_PATH);
//	DbgPrint(L"MyIUIFramework_LoadUI -> %s - %s", resourceName, pszModule);
//	HRESULT hr =  IUIFramework_LoadUIHook.OldFunction<decltype(MyIUIFramework_LoadUI)>(This, instance, resourceName);
//	try
//	{
//		ComPtr<IPropertyStore> spPropertyStore = com_cast<IPropertyStore>(This);
//
//		PROPVARIANT propvarBackground;
//		PROPVARIANT propvarHighlight;
//		PROPVARIANT propvarText;
//
//		// UI_HSBCOLOR is a type defined in UIRibbon.h that is composed of
//		// three component values: hue, saturation and brightness, respectively.
//		UI_HSBCOLOR HighlightColor = UI_HSB(0, 0xFF, 0xFF);
//		UI_HSBCOLOR BackgroundColor = UI_HSB(0, 0xAA, 0xAA);
//		UI_HSBCOLOR TextColor = UI_HSB(0, 0, 0xFF);
//
//		InitPropVariantFromUInt32(BackgroundColor, &propvarBackground);
//		InitPropVariantFromUInt32(HighlightColor, &propvarHighlight);
//		InitPropVariantFromUInt32(TextColor, &propvarText);
//
//		spPropertyStore->SetValue(UI_PKEY_GlobalHighlightColor, propvarHighlight);
//		spPropertyStore->SetValue(UI_PKEY_GlobalBackgroundColor, propvarBackground);
//		spPropertyStore->SetValue(UI_PKEY_GlobalTextColor, propvarText);
//
//		spPropertyStore->Commit();
//	}
//	catch (...)
//	{
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIUIFramework_Initialize(
//    ::IUIFramework *This,
//    HWND frameWnd,
//    IUIApplication *application
//)
//{
//	HRESULT hr = S_OK;
//	hr = IUIFramework_InitializeHook.OldFunction<decltype(MyIUIFramework_Initialize)>(
//	         This,
//	         frameWnd,
//	         application
//	     );
//	if (SUCCEEDED(hr))
//	{
//		WindowDbgPrint(frameWnd, L"MyIUIFramework_Initialize");
//	}
//	return hr;
//}
//
//// This function is nearly deprecated, it is actually no need to hook this function at all
//HRESULT WINAPI TranslucentFlyoutsLib::MyRoActivateInstance(
//    HSTRING      activatableClassId,
//    IInspectable **instance
//)
//{
//	HRESULT hr = RoActivateInstanceHook.OldFunction<decltype(MyRoActivateInstance)>(
//	                 activatableClassId,
//	                 instance
//	             );
//	return hr;
//}
//
//HRESULT WINAPI TranslucentFlyoutsLib::MyRoGetActivationFactory(
//    HSTRING activatableClassId,
//    REFIID  iid,
//    void **factory
//)
//{
//	HRESULT hr = RoGetActivationFactoryHook.OldFunction<decltype(MyRoGetActivationFactory)>(
//	                 activatableClassId,
//	                 iid,
//	                 factory
//	             );
//
//	if (VerifyCaller(g_hModule))
//	{
//		return hr;
//	}
//	if (SUCCEEDED(hr))
//	{
//		if (!IDesktopWindowXamlSourceFactory_CreateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<UI::Xaml::Hosting::IDesktopWindowXamlSourceFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<UI::Xaml::Hosting::IDesktopWindowXamlSourceFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Hosting_DesktopWindowXamlSource).Get());
//				//
//				IDesktopWindowXamlSourceFactory_CreateInstanceHook.Initialize(((IDesktopWindowXamlSourceFactory *)pFactory.Get())->lpVtbl->CreateInstance, MyIDesktopWindowXamlSourceFactory_CreateInstance);
//				Detours::Begin();
//				IDesktopWindowXamlSourceFactory_CreateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		//
//		//
//		//
//		if (!IMenuFlyoutFactory_CreateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<UI::Xaml::Controls::IMenuFlyoutFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<UI::Xaml::Controls::IMenuFlyoutFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_MenuFlyout).Get());
//				//
//				IMenuFlyoutFactory_CreateInstanceHook.Initialize(((IMenuFlyoutFactory *)pFactory.Get())->lpVtbl->CreateInstance, MyIMenuFlyoutFactory_CreateInstance);
//				Detours::Begin();
//				IMenuFlyoutFactory_CreateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		if (!IMenuFlyoutSubItemFactory_ActivateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<IActivationFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<IActivationFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_MenuFlyoutSubItem).Get());
//				//
//				IMenuFlyoutSubItemFactory_ActivateInstanceHook.Initialize(((IMenuFlyoutSubItemFactory*)pFactory.Get())->lpVtbl->ActivateInstance, MyIMenuFlyoutSubItemFactory_ActivateInstance);
//				Detours::Begin();
//				IMenuFlyoutSubItemFactory_ActivateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		if (!IMenuFlyoutItemFactory_CreateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<UI::Xaml::Controls::IMenuFlyoutItemFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<UI::Xaml::Controls::IMenuFlyoutItemFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_MenuFlyoutItem).Get());
//				//
//				IMenuFlyoutItemFactory_CreateInstanceHook.Initialize(((IMenuFlyoutItemFactory *)pFactory.Get())->lpVtbl->CreateInstance, MyIMenuFlyoutItemFactory_CreateInstance);
//				Detours::Begin();
//				IMenuFlyoutItemFactory_CreateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		if (!IToggleMenuFlyoutItemFactory_CreateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<UI::Xaml::Controls::IToggleMenuFlyoutItemFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<UI::Xaml::Controls::IToggleMenuFlyoutItemFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_ToggleMenuFlyoutItem).Get());
//				//
//				IToggleMenuFlyoutItemFactory_CreateInstanceHook.Initialize(((IToggleMenuFlyoutItemFactory *)pFactory.Get())->lpVtbl->CreateInstance, MyIToggleMenuFlyoutItemFactory_CreateInstance);
//				Detours::Begin();
//				IToggleMenuFlyoutItemFactory_CreateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		if (!IMenuBarItemFlyoutFactory_CreateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<UI::Xaml::Controls::IMenuBarItemFlyoutFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<UI::Xaml::Controls::IMenuBarItemFlyoutFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_MenuBarItemFlyout).Get());
//				//
//				IMenuBarItemFlyoutFactory_CreateInstanceHook.Initialize(((IMenuBarItemFlyoutFactory *)pFactory.Get())->lpVtbl->CreateInstance, MyIMenuBarItemFlyoutFactory_CreateInstance);
//				Detours::Begin();
//				IMenuBarItemFlyoutFactory_CreateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		//
//		//
//		//
//		if (!ICommandBarFlyoutFactory_CreateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<UI::Xaml::Controls::ICommandBarFlyoutFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<UI::Xaml::Controls::ICommandBarFlyoutFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_CommandBarFlyout).Get());
//				//
//				ICommandBarFlyoutFactory_CreateInstanceHook.Initialize(((ICommandBarFlyoutFactory *)pFactory.Get())->lpVtbl->CreateInstance, MyICommandBarFlyoutFactory_CreateInstance);
//				Detours::Begin();
//				ICommandBarFlyoutFactory_CreateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		if (!IAppBarButtonFactory_CreateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<UI::Xaml::Controls::IAppBarButtonFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<UI::Xaml::Controls::IAppBarButtonFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_AppBarButton).Get());
//				//
//				IAppBarButtonFactory_CreateInstanceHook.Initialize(((IAppBarButtonFactory *)pFactory.Get())->lpVtbl->CreateInstance, MyIAppBarButtonFactory_CreateInstance);
//				Detours::Begin();
//				IAppBarButtonFactory_CreateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		if (!IAppBarToggleButtonFactory_CreateInstanceHook.IsHookInstalled())
//		{
//			ComPtr<UI::Xaml::Controls::IAppBarToggleButtonFactory> pFactory = nullptr;
//			try
//			{
//				pFactory = GetActivationFactory<UI::Xaml::Controls::IAppBarToggleButtonFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_AppBarToggleButton).Get());
//				//
//				IAppBarToggleButtonFactory_CreateInstanceHook.Initialize(((IAppBarToggleButtonFactory *)pFactory.Get())->lpVtbl->CreateInstance, MyIAppBarToggleButtonFactory_CreateInstance);
//				Detours::Begin();
//				IAppBarToggleButtonFactory_CreateInstanceHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		//DbgPrint(HStringGetBuffer(activatableClassId));
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyICommandBarFlyoutFactory_CreateInstance(
//    ICommandBarFlyoutFactory *This,
//    IInspectable *baseInterface,
//    IInspectable **innerInterface,
//    UI::Xaml::Controls::ICommandBarFlyout **value
//)
//{
//	HRESULT hr = S_OK;
//	hr = ICommandBarFlyoutFactory_CreateInstanceHook.OldFunction<decltype(MyICommandBarFlyoutFactory_CreateInstance)>(
//	         This,
//	         baseInterface,
//	         innerInterface,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyICommandBarFlyoutFactory_CreateInstance");
//		/*auto pFlyout = *value;
//		try
//		{
//			SetAcrylicStyle(pFlyout);
//		}
//		catch (...)
//		{
//		}*/
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIAppBarButtonFactory_CreateInstance(
//    IAppBarButtonFactory *This,
//    IInspectable *baseInterface,
//    IInspectable **innerInterface,
//    UI::Xaml::Controls::IAppBarButton **value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IAppBarButtonFactory_CreateInstanceHook.OldFunction<decltype(MyIAppBarButtonFactory_CreateInstance)>(
//	         This,
//	         baseInterface,
//	         innerInterface,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyIAppBarButtonFactory_CreateInstance");
//		auto pItem = *value;
//		try
//		{
//			ComPtr<UI::Xaml::Controls::IControl> pControl = winrt_cast<UI::Xaml::Controls::IControl>(pItem);
//			SetAcrylicBrush(pControl.Get());
//		}
//		catch (...)
//		{
//		}
//	}
//	return hr;
//}
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIAppBarToggleButtonFactory_CreateInstance(
//    IAppBarToggleButtonFactory *This,
//    IInspectable *baseInterface,
//    IInspectable **innerInterface,
//    UI::Xaml::Controls::IAppBarToggleButton **value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IAppBarToggleButtonFactory_CreateInstanceHook.OldFunction<decltype(MyIAppBarToggleButtonFactory_CreateInstance)>(
//	         This,
//	         baseInterface,
//	         innerInterface,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyIAppBarToggleButtonFactory_CreateInstance");
//		auto pItem = *value;
//		try
//		{
//			ComPtr<UI::Xaml::Controls::IControl> pControl = winrt_cast<UI::Xaml::Controls::IControl>(pItem);
//			SetAcrylicBrush(pControl.Get());
//		}
//		catch (...)
//		{
//		}
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIMenuFlyoutFactory_CreateInstance(
//    UI::Xaml::Controls::IMenuFlyoutFactory *This,
//    IInspectable *baseInterface,
//    IInspectable **innerInterface,
//    UI::Xaml::Controls::IMenuFlyout **value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IMenuFlyoutFactory_CreateInstanceHook.OldFunction<decltype(MyIMenuFlyoutFactory_CreateInstance)>(
//	         This,
//	         baseInterface,
//	         innerInterface,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyIMenuFlyoutFactory_CreateInstance");
//		auto pFlyout = *value;
//		try
//		{
//			SetAcrylicStyle(pFlyout);
//		}
//		catch (...)
//		{
//		}
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIMenuFlyoutSubItemFactory_ActivateInstance(
//    IMenuFlyoutSubItemFactory *This,
//    UI::Xaml::Controls::IMenuFlyoutSubItem **instance
//)
//{
//	HRESULT hr = S_OK;
//	hr = IMenuFlyoutSubItemFactory_ActivateInstanceHook.OldFunction<decltype(MyIMenuFlyoutSubItemFactory_ActivateInstance)>(
//	         This,
//	         instance
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyIMenuFlyoutSubItemFactory_ActivateInstance");
//		auto pItem = *instance;
//		try
//		{
//			ComPtr<UI::Xaml::Controls::IControl> pControl = winrt_cast<UI::Xaml::Controls::IControl>(pItem);
//			SetAcrylicBrush(pControl.Get());
//		}
//		catch (...)
//		{
//		}
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIMenuFlyoutItemFactory_CreateInstance(
//    UI::Xaml::Controls::IMenuFlyoutItemFactory *This,
//    IInspectable *baseInterface,
//    IInspectable **innerInterface,
//    UI::Xaml::Controls::IMenuFlyoutItem **value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IMenuFlyoutItemFactory_CreateInstanceHook.OldFunction<decltype(MyIMenuFlyoutItemFactory_CreateInstance)>(
//	         This,
//	         baseInterface,
//	         innerInterface,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyIMenuFlyoutItemFactory_CreateInstance");
//		auto pItem = *value;
//		try
//		{
//			ComPtr<UI::Xaml::Controls::IControl> pControl = winrt_cast<UI::Xaml::Controls::IControl>(pItem);
//			SetAcrylicBrush(pControl.Get());
//		}
//		catch (...)
//		{
//		}
//		//
//		/*if (!IMenuFlyoutItem_put_TextHook.IsHookInstalled())
//		{
//			DbgPrint(L"IMenuFlyoutItem_put_Text -> Hooked");
//			IMenuFlyoutItem_put_TextHook.Initialize(((IMenuFlyoutItem *)pItem)->lpVtbl->put_Text, MyIMenuFlyoutItem_put_Text);
//			Detours::Begin();
//			IMenuFlyoutItem_put_TextHook.SetHookState();
//			Detours::Commit();
//		}*/
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIToggleMenuFlyoutItemFactory_CreateInstance(
//    IToggleMenuFlyoutItemFactory *This,
//    IInspectable *baseInterface,
//    IInspectable **innerInterface,
//    UI::Xaml::Controls::IToggleMenuFlyoutItem **value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IToggleMenuFlyoutItemFactory_CreateInstanceHook.OldFunction<decltype(MyIToggleMenuFlyoutItemFactory_CreateInstance)>(
//	         This,
//	         baseInterface,
//	         innerInterface,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyIToggleMenuFlyoutItemFactory_CreateInstance");
//		auto pItem = *value;
//		try
//		{
//			ComPtr<UI::Xaml::Controls::IControl> pControl = winrt_cast<UI::Xaml::Controls::IControl>(pItem);
//			SetAcrylicBrush(pControl.Get());
//		}
//		catch (...)
//		{
//		}
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIMenuBarItemFlyoutFactory_CreateInstance(
//    IMenuBarItemFlyoutFactory *This,
//    IInspectable *baseInterface,
//    IInspectable **innerInterface,
//    UI::Xaml::Controls::IMenuBarItemFlyout **value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IMenuBarItemFlyoutFactory_CreateInstanceHook.OldFunction<decltype(MyIMenuBarItemFlyoutFactory_CreateInstance)>(
//	         This,
//	         baseInterface,
//	         innerInterface,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyIMenuBarItemFlyoutFactory_CreateInstance");
//		auto pItem = *value;
//		try
//		{
//			ComPtr<UI::Xaml::Controls::IMenuFlyout> pFlyout = winrt_cast<UI::Xaml::Controls::IMenuFlyout>(pItem);
//			SetAcrylicStyle(pFlyout.Get());
//		}
//		catch (...)
//		{
//		}
//	}
//	return hr;
//}
//
///*HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIMenuFlyoutItem_put_Text(
//    UI::Xaml::Controls::IMenuFlyoutItem *This,
//    HSTRING value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IMenuFlyoutItem_put_TextHook.OldFunction<decltype(MyIMenuFlyoutItem_put_Text)>(
//	         This,
//	         HStringReference(L"maple真可爱！").Get()
//	     );
//	return hr;
//}*/
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIDesktopWindowXamlSourceFactory_CreateInstance(
//    IInspectable *baseInterface,
//    IInspectable **innerInterface,
//    UI::Xaml::Hosting::IDesktopWindowXamlSource **value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IDesktopWindowXamlSourceFactory_CreateInstanceHook.OldFunction<decltype(MyIDesktopWindowXamlSourceFactory_CreateInstance)>(
//	         baseInterface,
//	         innerInterface,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		DbgPrint(L"MyIDesktopWindowXamlSourceFactory_CreateInstance -> invoked");
//		//
//		if (!IDesktopWindowXamlSource_put_ContentHook.IsHookInstalled())
//		{
//			DbgPrint(L"IDesktopWindowXamlSource_put_Content -> Hooked");
//			//
//			try
//			{
//				ComPtr<UI::Xaml::Hosting::IDesktopWindowXamlSource> pDesktopWindowXamlSource = winrt_cast<UI::Xaml::Hosting::IDesktopWindowXamlSource>(*value);
//				//
//				IDesktopWindowXamlSource_put_ContentHook.Initialize(((IDesktopWindowXamlSource *)pDesktopWindowXamlSource.Get())->lpVtbl->put_Content, MyIDesktopWindowXamlSource_put_Content);
//				Detours::Begin();
//				IDesktopWindowXamlSource_put_ContentHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//		//
//		if (!IDesktopWindowXamlSourceNative_AttachToWindowHook.IsHookInstalled())
//		{
//			DbgPrint(L"IDesktopWindowXamlSourceNative_AttachToWindow -> Hooked");
//			//
//			try
//			{
//				ComPtr<::IDesktopWindowXamlSourceNative> pDesktopWindowXamlSourceNative = winrt_cast<::IDesktopWindowXamlSourceNative>(reinterpret_cast<UI::Xaml::Hosting::IDesktopWindowXamlSource *>(*value));
//				//
//				IDesktopWindowXamlSourceNative_AttachToWindowHook.Initialize(((IDesktopWindowXamlSourceNative *)pDesktopWindowXamlSourceNative.Get())->lpVtbl->AttachToWindow, MyIDesktopWindowXamlSourceNative_AttachToWindow);
//				Detours::Begin();
//				IDesktopWindowXamlSourceNative_AttachToWindowHook.SetHookState();
//				Detours::Commit();
//			}
//			catch (...)
//			{
//			}
//		}
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIDesktopWindowXamlSourceNative_AttachToWindow(
//    ::IDesktopWindowXamlSourceNative *This,
//    HWND parentWnd
//)
//{
//	HRESULT hr = S_OK;
//	hr = IDesktopWindowXamlSourceNative_AttachToWindowHook.OldFunction<decltype(MyIDesktopWindowXamlSourceNative_AttachToWindow)>(
//	         This,
//	         parentWnd
//	     );
//	if (SUCCEEDED(hr))
//	{
//		WindowDbgPrint(parentWnd, L"MyIDesktopWindowXamlSourceNative_AttachToWindow -> invoked");
//	}
//	return hr;
//}
//
//HRESULT STDMETHODCALLTYPE TranslucentFlyoutsLib::MyIDesktopWindowXamlSource_put_Content(
//    UI::Xaml::Hosting::IDesktopWindowXamlSource *This,
//    UI::Xaml::IUIElement *value
//)
//{
//	HRESULT hr = S_OK;
//	hr = IDesktopWindowXamlSource_put_ContentHook.OldFunction<decltype(MyIDesktopWindowXamlSource_put_Content)>(
//	         This,
//	         value
//	     );
//	if (SUCCEEDED(hr))
//	{
//		try
//		{
//			ComPtr<UI::Composition::ICompositor> pCompositor;
//			ComPtr<UI::Composition::ISpriteVisual> pVisual;
//			ComPtr<UI::Composition::ICompositionColorBrush> pBrush;
//			ComPtr<UI::Xaml::Hosting::IElementCompositionPreviewStatics> pCompositionPreview = GetActivationFactory<UI::Xaml::Hosting::IElementCompositionPreviewStatics>(HStringReference(RuntimeClass_Windows_UI_Xaml_Hosting_ElementCompositionPreview).Get());
//			ComPtr<UI::Xaml::Controls::IPanel> pPanel = winrt_cast<UI::Xaml::Controls::IPanel>(value);
//			ActivateInstanceWithFactory(HStringReference(RuntimeClass_Windows_UI_Composition_Compositor).Get(), &pCompositor);
//			ThrowIfFailed(pCompositor->CreateSpriteVisual(&pVisual));
//			ThrowIfFailed(pCompositor->CreateColorBrushWithColor({255, 255, 0, 0}, &pBrush));
//			ThrowIfFailed(pVisual->put_Brush(winrt_cast<UI::Composition::ICompositionBrush>(pBrush).Get()));
//			ThrowIfFailed(pCompositionPreview->SetElementChildVisual(value, winrt_cast<UI::Composition::IVisual>(pVisual).Get()));
//			SetAcrylicBrush(pPanel.Get(), 0.f, {0, 0, 0, 0});
//		}
//		catch (...) {}
//		DbgPrint(L"MyIDesktopWindowXamlSource_put_Content -> invoked -> %s", HStringGetBuffer(value));
//	}
//	return hr;
//}