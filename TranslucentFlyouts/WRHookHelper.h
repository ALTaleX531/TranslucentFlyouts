//		!!!IMPORTANT!!!
//
//		THIS FILE IS UNDER TESTING!!! WHICH IS NOT RESPONSIBLE FOR THE STABILITY!!!
//
//		!!!IMPORTANT!!!
#pragma once
#include "pch.h"
#include "WRHelper.h"
#include "DetoursHelper.h"

#ifndef DEFINE_VTBL
	#define DEFINE_VTBL(name, ...) struct name##Vtbl : ##__VA_ARGS__
#endif
#ifndef DEFINE_WINRT_VTBL
	#define DEFINE_WINRT_VTBL(name, ...) DEFINE_VTBL(name, IInspectableVtbl) {##__VA_ARGS__}
#endif
#ifndef DEFINE_WINRT_INTERFACE
	#define DEFINE_WINRT_INTERFACE(name, ...) struct name {const name##Vtbl *lpVtbl;}
#endif
//
#ifndef DEFINE_COM_VTBL
	#define DEFINE_COM_VTBL(name, ...) DEFINE_VTBL(name, IUnknownVtbl) {##__VA_ARGS__}
#endif
#ifndef DEFINE_COM_INTERFACE
	#define DEFINE_COM_INTERFACE(name, ...) struct name {const name##Vtbl *lpVtbl;}
#endif
//
#ifndef DEFINE_WINRT_OBJ
	#define DEFINE_WINRT_OBJ(name, ...) struct name;DEFINE_WINRT_VTBL(name, __VA_ARGS__);DEFINE_WINRT_INTERFACE(name);
#endif
#ifndef DEFINE_COM_OBJ
	#define DEFINE_COM_OBJ(name, ...) struct name;DEFINE_COM_VTBL(name, __VA_ARGS__);DEFINE_COM_INTERFACE(name);
#endif

namespace TranslucentFlyoutsLib
{
	struct IUnknownVtbl
	{
		HRESULT(STDMETHODCALLTYPE*QueryInterface)(
		    IUnknown * This,
		    REFIID riid,
		    void ** ppvObject
		);
		ULONG(STDMETHODCALLTYPE*AddRef)(IUnknown* This);
		ULONG(STDMETHODCALLTYPE*Release)(IUnknown * This);
	};
	struct IInspectableVtbl : IUnknownVtbl
	{
		HRESULT(STDMETHODCALLTYPE*GetIids)(
		    IInspectable * This,
		    ULONG *iidCount,
		    IID **iids
		);
		HRESULT(STDMETHODCALLTYPE*GetRuntimeClassName)(IInspectable * This, HSTRING *className);
		HRESULT(STDMETHODCALLTYPE*GetTrustLevel)(IInspectable * This, TrustLevel *trustLevel);
	};

	DEFINE_WINRT_OBJ(
	    IDesktopWindowXamlSource,
	    HRESULT(STDMETHODCALLTYPE *get_Content)(
	        IDesktopWindowXamlSource *This,
	        UI::Xaml::IUIElement **value
	    );
	    HRESULT(STDMETHODCALLTYPE *put_Content)(
	        IDesktopWindowXamlSource *This,
	        UI::Xaml::IUIElement *value
	    );
	    HRESULT(STDMETHODCALLTYPE *get_HasFocus)(
	        IDesktopWindowXamlSource *This,
	        boolean *value
	    );
	    HRESULT(STDMETHODCALLTYPE *add_TakeFocusRequested)(
	        IDesktopWindowXamlSource *This,
	        __FITypedEventHandler_2_Windows__CUI__CXaml__CHosting__CDesktopWindowXamlSource_Windows__CUI__CXaml__CHosting__CDesktopWindowXamlSourceTakeFocusRequestedEventArgs *handler,
	        EventRegistrationToken *token
	    );
	    HRESULT(STDMETHODCALLTYPE *remove_TakeFocusRequested)(
	        IDesktopWindowXamlSource *This,
	        EventRegistrationToken token
	    );
	    HRESULT(STDMETHODCALLTYPE *add_GotFocus)(
	        IDesktopWindowXamlSource *This,
	        __FITypedEventHandler_2_Windows__CUI__CXaml__CHosting__CDesktopWindowXamlSource_Windows__CUI__CXaml__CHosting__CDesktopWindowXamlSourceGotFocusEventArgs *handler,
	        EventRegistrationToken *token
	    );
	    HRESULT(STDMETHODCALLTYPE *remove_GotFocus)(
	        IDesktopWindowXamlSource *This,
	        EventRegistrationToken token
	    );
	    HRESULT(STDMETHODCALLTYPE *NavigateFocus)(
	        IDesktopWindowXamlSource *This,
	        UI::Xaml::Hosting::IXamlSourceFocusNavigationRequest *request,
	        UI::Xaml::Hosting::IXamlSourceFocusNavigationResult **result
	    );
	);

	DEFINE_COM_OBJ(
	    IDesktopWindowXamlSourceNative,
	    HRESULT(STDMETHODCALLTYPE *AttachToWindow)(
	        IDesktopWindowXamlSourceNative *This,
	        HWND parentWnd
	    );
	    HRESULT(STDMETHODCALLTYPE *get_WindowHandle)(
	        IDesktopWindowXamlSourceNative *This,
	        HWND *hWnd
	    );
	);

	DEFINE_WINRT_OBJ(
	    IMenuFlyoutFactory,
	    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
	        IMenuFlyoutFactory *This,
	        IInspectable *baseInterface,
	        IInspectable **innerInterface,
	        UI::Xaml::Controls::IMenuFlyout **value
	    );
	);
	DEFINE_WINRT_OBJ(
	    IMenuFlyoutItemFactory,
	    HRESULT(STDMETHODCALLTYPE*CreateInstance)(
	        IMenuFlyoutItemFactory *This,
	        IInspectable *baseInterface,
	        IInspectable **innerInterface,
	        UI::Xaml::Controls::IMenuFlyoutItem **value
	    );
	);
	DEFINE_WINRT_OBJ(
	    IMenuFlyoutSubItemFactory,
	    HRESULT(STDMETHODCALLTYPE *ActivateInstance)(
	        IMenuFlyoutSubItemFactory* This,
	        UI::Xaml::Controls::IMenuFlyoutSubItem **instance
	    );
	);
	DEFINE_WINRT_OBJ(
	    IToggleMenuFlyoutItemFactory,
	    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
	        IToggleMenuFlyoutItemFactory *This,
	        IInspectable *baseInterface,
	        IInspectable **innerInterface,
	        UI::Xaml::Controls::IToggleMenuFlyoutItem **value
	    );
	);
	DEFINE_WINRT_OBJ(
	    IMenuBarItemFlyoutFactory,
	    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
	        IMenuBarItemFlyoutFactory *This,
	        IInspectable *baseInterface,
	        IInspectable **innerInterface,
	        UI::Xaml::Controls::IMenuBarItemFlyout **value
	    );
	);
	//
	DEFINE_WINRT_OBJ(
	    ICommandBarFlyoutFactory,
	    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
	        ICommandBarFlyoutFactory *This,
	        IInspectable *baseInterface,
	        IInspectable **innerInterface,
	        UI::Xaml::Controls::ICommandBarFlyout **value
	    );
	);
	DEFINE_WINRT_OBJ(
	    IAppBarButtonFactory,
	    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
	        IAppBarButtonFactory *This,
	        IInspectable *baseInterface,
	        IInspectable **innerInterface,
	        UI::Xaml::Controls::IAppBarButton **value
	    );
	);
	DEFINE_WINRT_OBJ(
	    IAppBarToggleButtonFactory,
	    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
	        IAppBarToggleButtonFactory *This,
	        IInspectable *baseInterface,
	        IInspectable **innerInterface,
	        UI::Xaml::Controls::IAppBarToggleButton **value
	    );
	);
	DEFINE_WINRT_OBJ(
	    IMenuFlyoutItem,
	    HRESULT(STDMETHODCALLTYPE *get_Text)(
	        IMenuFlyoutItem *This,
	        HSTRING *value
	    );
	    HRESULT(STDMETHODCALLTYPE *put_Text)(
	        IMenuFlyoutItem *This,
	        HSTRING value
	    );
	);

	DEFINE_WINRT_OBJ(
	    IDesktopWindowXamlSourceFactory,
	    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
	        IDesktopWindowXamlSourceFactory *This,
	        IInspectable *baseInterface,
	        IInspectable **innerInterface,
	        IDesktopWindowXamlSource **value
	    );
	);

	DEFINE_COM_OBJ(
	    IUIFramework,
	    HRESULT(STDMETHODCALLTYPE*Initialize)(
	        IUIFramework* This,
	        HWND frameWnd,
	        IUIApplication *application
	    );
	    HRESULT(STDMETHODCALLTYPE*Destroy)(IUIFramework *This);
	    HRESULT(STDMETHODCALLTYPE*LoadUI)(
	        IUIFramework *This,
	        HINSTANCE instance,
	        LPCWSTR resourceName
	    );
	    HRESULT(STDMETHODCALLTYPE*GetView)(
	        IUIFramework *This,
	        UINT32 viewId,
	        REFIID riid,
	        void **ppv
	    );
	    HRESULT(STDMETHODCALLTYPE*GetUICommandProperty)(
	        IUIFramework *This,
	        UINT32 commandId,
	        REFPROPERTYKEY key,
	        PROPVARIANT *value
	    );
	    HRESULT(STDMETHODCALLTYPE*SetUICommandProperty)(
	        IUIFramework *This,
	        UINT32 commandId,
	        REFPROPERTYKEY key,
	        REFPROPVARIANT value
	    );
	    HRESULT(STDMETHODCALLTYPE*InvalidateUICommand)(
	        IUIFramework *This,
	        UINT32 commandId,
	        UI_INVALIDATIONS flags,
	        const PROPERTYKEY *key
	    );
	    HRESULT(STDMETHODCALLTYPE*FlushPendingInvalidations)(IUIFramework *This);
	    HRESULT(STDMETHODCALLTYPE*SetModes)(
	        IUIFramework *This,
	        INT32 iModes
	    );
	);

	extern HRESULT STDMETHODCALLTYPE MyIUIFramework_LoadUI(
		::IUIFramework *This,
	    HINSTANCE instance,
	    LPCWSTR resourceName
	);
	extern HRESULT STDMETHODCALLTYPE MyIUIFramework_Initialize(
	    ::IUIFramework *This,
	    HWND frameWnd,
	    IUIApplication *application
	);
	extern HRESULT WINAPI MyCoCreateInstance(
	    REFCLSID  rclsid,
	    LPUNKNOWN pUnkOuter,
	    DWORD     dwClsContext,
	    REFIID    riid,
	    LPVOID *ppv
	);
	extern HRESULT WINAPI MyRoGetActivationFactory(
	    HSTRING activatableClassId,
	    REFIID  iid,
	    void    **factory
	);
	extern HRESULT WINAPI MyRoActivateInstance(
	    HSTRING      activatableClassId,
	    IInspectable **instance
	);
	extern HRESULT STDMETHODCALLTYPE MyIDesktopWindowXamlSourceFactory_CreateInstance(
	    IInspectable * baseInterface,
	    IInspectable ** innerInterface,
	    UI::Xaml::Hosting::IDesktopWindowXamlSource ** value
	);
	extern HRESULT STDMETHODCALLTYPE MyIDesktopWindowXamlSource_put_Content(
	    UI::Xaml::Hosting::IDesktopWindowXamlSource* This,
	    UI::Xaml::IUIElement * value
	);
	extern HRESULT STDMETHODCALLTYPE MyIDesktopWindowXamlSourceNative_AttachToWindow(
	    ::IDesktopWindowXamlSourceNative* This,
	    HWND parentWnd
	);
	//
	extern HRESULT STDMETHODCALLTYPE MyIMenuFlyoutFactory_CreateInstance(
	    UI::Xaml::Controls::IMenuFlyoutFactory *This,
	    IInspectable *baseInterface,
	    IInspectable **innerInterface,
	    UI::Xaml::Controls::IMenuFlyout **value
	);
	extern HRESULT STDMETHODCALLTYPE MyIMenuFlyoutItemFactory_CreateInstance(
	    UI::Xaml::Controls::IMenuFlyoutItemFactory *This,
	    IInspectable *baseInterface,
	    IInspectable **innerInterface,
	    UI::Xaml::Controls::IMenuFlyoutItem **value
	);
	extern HRESULT STDMETHODCALLTYPE MyIMenuFlyoutSubItemFactory_ActivateInstance(
	    IMenuFlyoutSubItemFactory *This,
	    UI::Xaml::Controls::IMenuFlyoutSubItem **instance
	);
	extern HRESULT STDMETHODCALLTYPE MyIToggleMenuFlyoutItemFactory_CreateInstance(
	    IToggleMenuFlyoutItemFactory *This,
	    IInspectable *baseInterface,
	    IInspectable **innerInterface,
	    UI::Xaml::Controls::IToggleMenuFlyoutItem **value
	);
	//
	extern HRESULT STDMETHODCALLTYPE MyIMenuBarItemFlyoutFactory_CreateInstance(
	    IMenuBarItemFlyoutFactory *This,
	    IInspectable *baseInterface,
	    IInspectable **innerInterface,
	    UI::Xaml::Controls::IMenuBarItemFlyout **value
	);
	//
	extern HRESULT STDMETHODCALLTYPE MyICommandBarFlyoutFactory_CreateInstance(
	    ICommandBarFlyoutFactory *This,
	    IInspectable *baseInterface,
	    IInspectable **innerInterface,
	    UI::Xaml::Controls::ICommandBarFlyout **value
	);
	extern HRESULT STDMETHODCALLTYPE MyIAppBarButtonFactory_CreateInstance(
	    IAppBarButtonFactory *This,
	    IInspectable *baseInterface,
	    IInspectable **innerInterface,
	    UI::Xaml::Controls::IAppBarButton **value
	);
	extern HRESULT STDMETHODCALLTYPE MyIAppBarToggleButtonFactory_CreateInstance(
	    IAppBarToggleButtonFactory *This,
	    IInspectable *baseInterface,
	    IInspectable **innerInterface,
	    UI::Xaml::Controls::IAppBarToggleButton **value
	);
	//
	extern HRESULT STDMETHODCALLTYPE MyIMenuFlyoutItem_put_Text(
	    UI::Xaml::Controls::IMenuFlyoutItem *This,
	    HSTRING value
	);
	// COM
	extern DetoursHook IUIFramework_LoadUIHook;
	extern DetoursHook IUIFramework_InitializeHook;
	extern DetoursHook CoCreateInstanceHook;
	// Windows Runtime
	extern DetoursHook RoActivateInstanceHook;
	extern DetoursHook RoGetActivationFactoryHook;
	//
	extern DetoursHook IDesktopWindowXamlSourceFactory_CreateInstanceHook;
	extern DetoursHook IDesktopWindowXamlSourceNative_AttachToWindowHook;
	extern DetoursHook IDesktopWindowXamlSource_put_ContentHook;
	//
	extern DetoursHook IMenuFlyoutFactory_CreateInstanceHook;
	extern DetoursHook IMenuFlyoutItemFactory_CreateInstanceHook;
	extern DetoursHook IMenuFlyoutSubItemFactory_ActivateInstanceHook;
	extern DetoursHook IToggleMenuFlyoutItemFactory_CreateInstanceHook;
	extern DetoursHook IMenuBarItemFlyoutFactory_CreateInstanceHook;
	//
	extern DetoursHook ICommandBarFlyoutFactory_CreateInstanceHook;
	extern DetoursHook IAppBarButtonFactory_CreateInstanceHook;
	extern DetoursHook IAppBarToggleButtonFactory_CreateInstanceHook;
	//
	extern DetoursHook IMenuFlyoutItem_put_TextHook;
	//
	extern void WRHookStartup();
	extern void WRHookShutdown();
}