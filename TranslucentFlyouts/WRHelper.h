//		!!!IMPORTANT!!!
//
//		THIS FILE IS UNDER TESTING!!! WHICH IS NOT RESPONSIBLE FOR THE STABILITY!!!
//
//		!!!IMPORTANT!!!
#pragma once
#include "pch.h"

namespace TranslucentFlyoutsLib
{
	inline void ThrowIfFailed(const HRESULT& hr)
	{
		if (FAILED(hr))
		{
			throw hr;
		}
	}

	template <class T>
	static inline auto GetActivationFactory(HSTRING activatableClassId)
	{
		ComPtr<T> pFactory{nullptr};

		HRESULT hr = RoGetActivationFactory(activatableClassId, IID_PPV_ARGS(&pFactory));
		if (hr == REGDB_E_CLASSNOTREG)
		{
			CO_MTA_USAGE_COOKIE pCookie{nullptr};
			ThrowIfFailed(CoIncrementMTAUsage(&pCookie));
			hr = RoGetActivationFactory(activatableClassId, IID_PPV_ARGS(&pFactory));
		}
		ThrowIfFailed(hr);

		return pFactory;
	}

	template <typename T, typename T2>
	static inline void CreateInstanceWithFactory(HSTRING activatableClassId, T2** pInspectable)
	{
		ComPtr<IInspectable> baseInterface;
		ComPtr<IInspectable> innerInterface;
		ComPtr<T> pFactory{GetActivationFactory<T>(activatableClassId)};
		ThrowIfFailed(pFactory->CreateInstance(baseInterface.Get(), &innerInterface, (T2**)pInspectable));
	}

	static inline void ActivateInstanceWithFactory(HSTRING activatableClassId, IInspectable** pInspectable)
	{
		ComPtr<IActivationFactory> pFactory{GetActivationFactory<IActivationFactory>(activatableClassId)};
		ThrowIfFailed(pFactory->ActivateInstance(pInspectable));
	}

	template <typename T>
	static inline auto winrt_cast(IInspectable* pInspectable)
	{
		ComPtr<T> instance{nullptr};
		ThrowIfFailed(pInspectable->QueryInterface(IID_PPV_ARGS(&instance)));
		return instance;
	}
	template <typename T, typename Ptr>
	static inline auto winrt_cast(const ComPtr<Ptr>& pInspectable)
	{
		ComPtr<T> instance{nullptr};
		ThrowIfFailed(pInspectable.As(&instance));
		return instance;
	}

	static inline LPCWSTR HStringGetBuffer(HSTRING hString)
	{
		return WindowsGetStringRawBuffer(hString, nullptr);
	}

	static inline LPCWSTR HStringGetBuffer(IInspectable* instance)
	{
		HSTRING hString = nullptr;
		if (SUCCEEDED(instance->GetRuntimeClassName(&hString)))
		{
			return HStringGetBuffer(hString);
		}
		return nullptr;
	}

	static inline bool VerifyHString(HSTRING hString, LPCWSTR lpString)
	{
		return !_wcsicmp(HStringGetBuffer(hString), lpString);
	}

	static inline bool VerifyHString(IInspectable* instance, LPCWSTR lpString)
	{
		return !_wcsicmp(HStringGetBuffer(instance), lpString);
	}

	static inline auto GetAcrylicBrush(
	    DOUBLE fOpacity = 0.f,
	UI::Color color = {0, 0, 0, 0}
	)
	{
		ComPtr<UI::Xaml::Media::IAcrylicBrush> pAcrylicBrush = nullptr;
		// 简单地初始化一下工厂
		CreateInstanceWithFactory
		<UI::Xaml::Media::IAcrylicBrushFactory, UI::Xaml::Media::IAcrylicBrush>
		(HStringReference(RuntimeClass_Windows_UI_Xaml_Media_AcrylicBrush).Get(), &pAcrylicBrush);
		// 设置合成源为窗口背景
		ThrowIfFailed(pAcrylicBrush->put_BackgroundSource(UI::Xaml::Media::AcrylicBackgroundSource_HostBackdrop));
		// 设置叠加色
		ThrowIfFailed(pAcrylicBrush->put_TintColor(color));
		// 设置叠加色不透明度
		ThrowIfFailed(pAcrylicBrush->put_TintOpacity(fOpacity));
		//
		return winrt_cast<UI::Xaml::Media::IAcrylicBrush>(pAcrylicBrush);
	}

	static inline void SetAcrylicStyle(
	    UI::Xaml::Controls::IMenuFlyout* pFlyout,
	    DOUBLE fOpacity = 0.f,
	    UI::Color color = {0, 0, 0, 0}
	)
	{
		ComPtr<UI::Xaml::IStyle> pStyle;
		ComPtr<UI::Xaml::ISetter> pSetter;
		ComPtr<UI::Xaml::ISetterFactory> pSetterFactory;
		ComPtr<UI::Xaml::IStyleFactory> pStyleFactory;
		ComPtr<UI::Xaml::ISetterBaseCollection> pSetters;
		ComPtr<UI::Xaml::IDependencyProperty> pProperty;
		ComPtr<UI::Xaml::Controls::IControlStatics> pControlStatics;
		ComPtr<Foundation::Collections::IVector<UI::Xaml::SetterBase *>> pVector;
		// 简单地初始化一下工厂
		pControlStatics = GetActivationFactory<UI::Xaml::Controls::IControlStatics>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_Control).Get());
		pSetterFactory = GetActivationFactory<UI::Xaml::ISetterFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Setter).Get());
		pStyleFactory = GetActivationFactory<UI::Xaml::IStyleFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Style).Get());
		// 获取背景属性对象
		ThrowIfFailed(
		    pControlStatics->get_BackgroundProperty(&pProperty)
		);
		// 创建一个风格样式
		ThrowIfFailed(pStyleFactory->CreateInstance({HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_MenuFlyoutPresenter).Get(), UI::Xaml::Interop::TypeKind_Primitive}, &pStyle));
		// 创建一个值为亚克力画刷的设置器
		ThrowIfFailed(
		    pSetterFactory->CreateInstance(
		        pProperty.Get(),
		        GetAcrylicBrush().Get(),
		        &pSetter
		    )
		);
		// 获取所有设置器
		ThrowIfFailed(pStyle->get_Setters(&pSetters));
		// 转换为Vector来读写
		pVector = winrt_cast<Foundation::Collections::IVector<UI::Xaml::SetterBase *>>(pSetters);
		// 追加我们的设置器
		ThrowIfFailed(pVector->Append(winrt_cast<UI::Xaml::ISetterBase>(pSetter).Get()));
		// 设置风格样式到浮出菜单
		ThrowIfFailed(pFlyout->put_MenuFlyoutPresenterStyle(pStyle.Get()));
	}

	static inline void SetAcrylicStyle(
		UI::Xaml::Controls::IFlyout *pFlyout,
		DOUBLE fOpacity = 0.f,
		UI::Color color = {0, 0, 0, 0}
	)
	{
		ComPtr<UI::Xaml::IStyle> pStyle;
		ComPtr<UI::Xaml::ISetter> pSetter;
		ComPtr<UI::Xaml::ISetterFactory> pSetterFactory;
		ComPtr<UI::Xaml::IStyleFactory> pStyleFactory;
		ComPtr<UI::Xaml::ISetterBaseCollection> pSetters;
		ComPtr<UI::Xaml::IDependencyProperty> pProperty;
		ComPtr<UI::Xaml::Controls::IControlStatics> pControlStatics;
		ComPtr<Foundation::Collections::IVector<UI::Xaml::SetterBase *>> pVector;
		// 简单地初始化一下工厂
		pControlStatics = GetActivationFactory<UI::Xaml::Controls::IControlStatics>(HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_Control).Get());
		pSetterFactory = GetActivationFactory<UI::Xaml::ISetterFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Setter).Get());
		pStyleFactory = GetActivationFactory<UI::Xaml::IStyleFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Style).Get());
		// 获取背景属性对象
		ThrowIfFailed(
			pControlStatics->get_BackgroundProperty(&pProperty)
		);
		// 创建一个风格样式
		ThrowIfFailed(pStyleFactory->CreateInstance({HStringReference(RuntimeClass_Windows_UI_Xaml_Controls_FlyoutPresenter).Get(), UI::Xaml::Interop::TypeKind_Primitive}, &pStyle));
		// 创建一个值为亚克力画刷的设置器
		ThrowIfFailed(
			pSetterFactory->CreateInstance(
			pProperty.Get(),
			GetAcrylicBrush().Get(),
			&pSetter
		)
		);
		// 获取所有设置器
		ThrowIfFailed(pStyle->get_Setters(&pSetters));
		// 转换为Vector来读写
		pVector = winrt_cast<Foundation::Collections::IVector<UI::Xaml::SetterBase *>>(pSetters);
		// 追加我们的设置器
		ThrowIfFailed(pVector->Append(winrt_cast<UI::Xaml::ISetterBase>(pSetter).Get()));
		// 设置风格样式到浮出菜单
		ThrowIfFailed(pFlyout->put_FlyoutPresenterStyle(pStyle.Get()));
	}

	static inline void SetAcrylicBrush(
	    UI::Xaml::Controls::IControl *pControl,
	    DOUBLE fOpacity = 0.f,
	    UI::Color color = {0, 0, 0, 0}
	)
	{
		//
		ThrowIfFailed(pControl->put_Background(winrt_cast<UI::Xaml::Media::IBrush>(GetAcrylicBrush(fOpacity, color)).Get()));
		ThrowIfFailed(pControl->put_BorderBrush(winrt_cast<UI::Xaml::Media::IBrush>(GetAcrylicBrush(fOpacity, color)).Get()));
	}

	static inline void SetAcrylicBrush(
	    UI::Xaml::Controls::IPanel *pPanel,
	    DOUBLE fOpacity = 0.f,
	    UI::Color color = {0, 0, 0, 0}
	)
	{
		//
		ThrowIfFailed(pPanel->put_Background(winrt_cast<UI::Xaml::Media::IBrush>(GetAcrylicBrush(fOpacity, color)).Get()));
	}
}