#include "pch.h"
#include "Hooking.hpp"
#include "DXHelper.hpp"

namespace TranslucentFlyouts::DXHelper
{
	using namespace std;
	using namespace D2D1;

	LazyDX::InternalHook LazyDX::g_internalHook{};
}

BOOL WINAPI TranslucentFlyouts::DXHelper::LazyDX::InternalHook::FreeLibrary(
	HMODULE hLibModule
)
{
	auto& dxList{g_internalHook.dxList};
	auto actualFreeLibrary{g_internalHook.actualFreeLibrary};

	if (hLibModule == HINST_THISCOMPONENT)
	{
		for (auto it = dxList.begin(); it != dxList.end(); it++)
		{
			auto& lazyDX{*it};
			lazyDX->DestroyDeviceResources();
			lazyDX->DestroyDeviceIndependentResources();
		}
		dxList.clear();
		g_internalHook.ShutdownHook();

		auto f = [](PTP_CALLBACK_INSTANCE pci, PVOID)
		{
			// Wait for 10ms so that Kernel32!FreeLibrary can return safely to LazyD2D::FreeLibrary,
			// then go back to its caller, ending the function call
			DisassociateCurrentThreadFromCallback(pci);
			FreeLibraryWhenCallbackReturns(pci, HINST_THISCOMPONENT);
			Sleep(10);
		};
		if (TrySubmitThreadpoolCallback(f, nullptr, nullptr))
		{
			SetLastError(ERROR_SUCCESS);
			return TRUE;
		}
		else
		{
			LOG_LAST_ERROR();
		}
	}

	return actualFreeLibrary(hLibModule);
}

void TranslucentFlyouts::DXHelper::LazyDX::NotifyDeviceLost()
{
	auto& dxList{g_internalHook.dxList};
	for (auto it = dxList.begin(); it != dxList.end(); it++)
	{
		auto& lazyDX{*it};
		lazyDX->DestroyDeviceResources();
		lazyDX->CreateDeviceResources();
	}
}

TranslucentFlyouts::DXHelper::LazyDX::InternalHook::InternalHook()
{
	actualFreeLibrary = reinterpret_cast<decltype(actualFreeLibrary)>(DetourFindFunction("kernel32.dll", "FreeLibrary"));
	if (!actualFreeLibrary)
	{
		LOG_LAST_ERROR();
		return;
	}

	StartupHook();
}

TranslucentFlyouts::DXHelper::LazyDX::InternalHook::~InternalHook()
{
	ShutdownHook();
	actualFreeLibrary = nullptr;
}

void TranslucentFlyouts::DXHelper::LazyDX::InternalHook::StartupHook()
{
	if (!hooked)
	{
		THROW_HR_IF(E_FAIL, !Hooking::WriteIAT(GetModuleHandleW(L"user32.dll"), "api-ms-win-core-libraryloader-l1-2-0.dll"sv, {{"FreeLibrary", FreeLibrary}}));
		hooked = true;
	}
}

void TranslucentFlyouts::DXHelper::LazyDX::InternalHook::ShutdownHook()
{
	if (hooked)
	{
		Hooking::WriteIAT(GetModuleHandleW(L"user32.dll"), "api-ms-win-core-libraryloader-l1-2-0.dll"sv, {{"FreeLibrary", actualFreeLibrary}});
		hooked = false;
	}
}

TranslucentFlyouts::DXHelper::LazyDX::LazyDX()
{
	g_internalHook.dxList.push_back(this);
}

TranslucentFlyouts::DXHelper::LazyDX::~LazyDX()
{
	auto& dxList{g_internalHook.dxList};
	for (auto it = dxList.begin(); it != dxList.end();)
	{
		if (*it == this)
		{
			it = dxList.erase(it);
			break;
		}
		else
		{
			it++;
		}
	}
}

/* ======================================================================================== */

TranslucentFlyouts::DXHelper::LazyD2D& TranslucentFlyouts::DXHelper::LazyD2D::GetInstance()
{
	static LazyD2D instance{};
	return instance;
}

bool TranslucentFlyouts::DXHelper::LazyD2D::EnsureInitialized()
{
	auto& lazyD2D{GetInstance()};

	if (lazyD2D.m_dcRT)
	{
		if (FAILED(lazyD2D.m_dcRT->Flush()))
		{
			LazyDX::NotifyDeviceLost();
		}
	}

	return (lazyD2D.m_dcRT && lazyD2D.m_factory ? true : false);
}

TranslucentFlyouts::DXHelper::LazyD2D::LazyD2D()
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

void TranslucentFlyouts::DXHelper::LazyD2D::CreateDeviceIndependentResources()
{
	try
	{
		wil::com_ptr<ID2D1Factory> factory{nullptr};
		THROW_IF_FAILED(
			D2D1CreateFactory<ID2D1Factory>(
				D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_MULTI_THREADED,
				&factory
				)
		);

		factory.copy_to(&m_factory);
	}
	catch (...)
	{
		LOG_CAUGHT_EXCEPTION();
	}
}
void TranslucentFlyouts::DXHelper::LazyD2D::CreateDeviceResources()
{
	try
	{
		THROW_HR_IF_NULL(E_INVALIDARG, m_factory);

		D2D1_RENDER_TARGET_PROPERTIES properties
		{
			RenderTargetProperties(
				// Always use software rendering, otherwise the performance is pretty bad!
				// The reason is you incur costs transferring from GPU to System
				D2D1_RENDER_TARGET_TYPE_SOFTWARE,
				PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				96.f, 96.f
			)
		};

		wil::com_ptr<ID2D1DCRenderTarget> dcRT{nullptr};
		THROW_IF_FAILED(
			m_factory->CreateDCRenderTarget(
				&properties,
				&dcRT
			)
		);

		dcRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

		dcRT.copy_to(&m_dcRT);
	}
	catch (...)
	{
		LOG_CAUGHT_EXCEPTION();
	}
}
void TranslucentFlyouts::DXHelper::LazyD2D::DestroyDeviceIndependentResources()
{
	m_factory.reset();
}
void TranslucentFlyouts::DXHelper::LazyD2D::DestroyDeviceResources()
{
	m_dcRT.reset();
}

D2D1::ColorF TranslucentFlyouts::DXHelper::COLORREF2ColorF(COLORREF color, std::byte alpha)
{
	return ColorF(
			   static_cast<float>(GetRValue(color)) / 255.f,
			   static_cast<float>(GetGValue(color)) / 255.f,
			   static_cast<float>(GetBValue(color)) / 255.f,
			   static_cast<float>(std::to_integer<int>(alpha)) / 255.f
		   );
}