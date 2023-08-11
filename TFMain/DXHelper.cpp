#include "pch.h"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "DXHelper.hpp"

using namespace std;
using namespace wil;
using namespace D2D1;
using namespace TranslucentFlyouts;
using namespace TranslucentFlyouts::DXHelper;

LazyDX::InternalHook LazyDX::g_internalHook{};

BOOL WINAPI LazyDX::InternalHook::FreeLibrary(
	HMODULE hLibModule
)
{
	auto dxList{g_internalHook.dxList};
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

		auto f = [](PTP_CALLBACK_INSTANCE pci, PVOID)
		{
			// Wait for 100ms so that Kernel32!FreeLibrary can return safely to LazyD2D::FreeLibrary,
			// then go back to its caller, ending the function call
			DisassociateCurrentThreadFromCallback(pci);
			FreeLibraryWhenCallbackReturns(pci, HINST_THISCOMPONENT);
			Sleep(100);
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

void LazyDX::NotifyDeviceLost()
{
	auto& dxList{g_internalHook.dxList};
	for (auto it = dxList.begin(); it != dxList.end(); it++)
	{
		auto& lazyDX{*it};
		
		try
		{
			lazyDX->DestroyDeviceResources();
			lazyDX->CreateDeviceResources();
		}
		CATCH_LOG()
	}
}

LazyDX::InternalHook::InternalHook()
{
	actualFreeLibrary = reinterpret_cast<decltype(actualFreeLibrary)>(DetourFindFunction("kernel32.dll", "FreeLibrary"));
	if (!actualFreeLibrary)
	{
		LOG_LAST_ERROR();
		return;
	}

	StartupHook();
}

LazyDX::InternalHook::~InternalHook()
{
	ShutdownHook();
	actualFreeLibrary = nullptr;
}

void LazyDX::InternalHook::StartupHook()
{
	if (!hooked)
	{
		THROW_HR_IF(E_FAIL, !Hooking::WriteIAT(GetModuleHandleW(L"user32.dll"), "api-ms-win-core-libraryloader-l1-2-0.dll"sv, {{"FreeLibrary", FreeLibrary}}));
		hooked = true;
	}
}

void LazyDX::InternalHook::ShutdownHook()
{
	if (hooked)
	{
		Hooking::WriteIAT(GetModuleHandleW(L"user32.dll"), "api-ms-win-core-libraryloader-l1-2-0.dll"sv, {{"FreeLibrary", actualFreeLibrary}});
		hooked = false;
	}
}

LazyDX::LazyDX()
{
	g_internalHook.dxList.push_back(this);
}

LazyDX::~LazyDX()
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

LazyD2D& LazyD2D::GetInstance()
{
	static LazyD2D instance{};
	return instance;
}

bool LazyD2D::EnsureInitialized()
{
	auto& lazyD2D{GetInstance()};

	return (LazyDComposition::EnsureInitialized() && lazyD2D.m_dcRT && lazyD2D.m_factory ? true : false);
}

LazyD2D::LazyD2D()
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

LazyD2D::~LazyD2D()
{
	DestroyDeviceIndependentResources();
	DestroyDeviceResources();
}

void LazyD2D::CreateDeviceIndependentResources()
{
	try
	{
		com_ptr<ID2D1Factory> factory{nullptr};
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
void LazyD2D::CreateDeviceResources()
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

		com_ptr<ID2D1DCRenderTarget> dcRT{nullptr};
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
void LazyD2D::DestroyDeviceIndependentResources() noexcept
{
	m_factory.reset();
}
void LazyD2D::DestroyDeviceResources() noexcept
{
	m_dcRT.reset();
}

/* ======================================================================================== */

LazyD3D& LazyD3D::GetInstance()
{
	static LazyD3D instance{};
	return instance;
}

bool LazyD3D::EnsureInitialized()
{
	auto& lazyD3D{GetInstance()};

	return (lazyD3D.m_d3dDevice && lazyD3D.m_dxgiDevice ? true : false);
}

LazyD3D::LazyD3D()
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

LazyD3D::~LazyD3D()
{
	DestroyDeviceIndependentResources();
	DestroyDeviceResources();
}

void LazyD3D::CreateDeviceIndependentResources()
{
}
void LazyD3D::CreateDeviceResources()
{
	try
	{
		auto CleanUp{Utils::RoInit()};

		com_ptr<IDXGIDevice3> dxgiDevice{nullptr};
		com_ptr<ID3D11Device> d3dDevice{nullptr};

		THROW_IF_FAILED(
			D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				D3D11_CREATE_DEVICE_BGRA_SUPPORT,
				nullptr,
				0,
				D3D11_SDK_VERSION,
				&d3dDevice,
				nullptr,
				nullptr
			)
		);
		d3dDevice.query_to(&dxgiDevice);

		d3dDevice.copy_to(&m_d3dDevice);
		dxgiDevice.copy_to(&m_dxgiDevice);
	}
	catch (...)
	{
		LOG_CAUGHT_EXCEPTION();
	}
}
void LazyD3D::DestroyDeviceIndependentResources() noexcept
{
}

void LazyD3D::DestroyDeviceResources() noexcept
{
	m_d3dDevice.reset();
	m_dxgiDevice.reset();
}

/* ======================================================================================== */

LazyDComposition& LazyDComposition::GetInstance()
{
	static LazyDComposition instance{};
	return instance;
}

bool LazyDComposition::EnsureInitialized()
{
	auto& lazyDComp{GetInstance()};

	if (lazyDComp.m_dcompDevice)
	{
		try
		{
			BOOL valid{FALSE};
			THROW_IF_FAILED(
				lazyDComp.m_dcompDevice.query<IDCompositionDevice>()->CheckDeviceState(&valid)
			);

			if (!valid)
			{
				LazyDX::NotifyDeviceLost();
			}
		}
		CATCH_LOG()
	}

	return (lazyDComp.m_dcompDevice ? true : false);
}

LazyDComposition::LazyDComposition()
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

LazyDComposition::~LazyDComposition()
{
	DestroyDeviceIndependentResources();
	DestroyDeviceResources();
}

void LazyDComposition::CreateDeviceIndependentResources()
{
}
void LazyDComposition::CreateDeviceResources()
{
	try
	{
		com_ptr<IDCompositionDesktopDevice> dcompDevice{nullptr};
		THROW_IF_FAILED(
			DCompositionCreateDevice3(
				m_lazyD3D.GetDxgiDevice().get(),
				IID_PPV_ARGS(&dcompDevice)
			)
		);

		dcompDevice.copy_to(&m_dcompDevice);
	}
	catch (...)
	{
		LOG_CAUGHT_EXCEPTION();
	}
}
void LazyDComposition::DestroyDeviceIndependentResources() noexcept
{
}

void LazyDComposition::DestroyDeviceResources() noexcept
{
	m_dcompDevice.reset();
}

ColorF TranslucentFlyouts::DXHelper::MakeColorF(DWORD argb)
{
	auto a{static_cast<float>(argb >> 24) / 255.f};
	auto r{static_cast<float>(argb >> 16 & 0xff) / 255.f};
	auto g{static_cast<float>(argb >> 8 & 0xff) / 255.f};
	auto b{static_cast<float>(argb & 0xff) / 255.f};

	return ColorF{
		r,
		g,
		b,
		a
	};
}
