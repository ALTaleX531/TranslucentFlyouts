#pragma once
#include "Utils.hpp"
#include "dx.h"

namespace TranslucentFlyouts
{
	// https://social.msdn.microsoft.com/Forums/vstudio/en-US/edad7bd5-4577-4782-b850-40b141e43198/coinitializecouninitialize-are-they-really-needed-for-direct2d-and-directwrite
	// DirectWrite/Direct2D/Direct3D are COM like APIs but they don't use COM.
	// They are not registered in registry like normal COM components,
	// they do not following COM threading models,
	// they don't support any sort of marshaling etc.
	// They're not COM.

	// CoInitialize functionality is important for writing advanced object oriented classes,
	// as well as writing interface or factory type classes that provide an interface,
	// like extending the direct2d and direct3d api's.
	// It has something to do with memory management and efficiency during runtime and code complexity issues that become apparent with large code sets and classes,
	// multiple inheritance, etc... and other software engineering problems.
	// It also helps when creating class creation and security when running instantiating interfaces in a distributed or networked application.
	namespace DXHelper
	{
		class LazyDX
		{
		public:
			LazyDX(const LazyDX&) = delete;
			LazyDX& operator=(const LazyDX&) = delete;

			static void NotifyDeviceLost();
		protected:
			LazyDX();
			virtual ~LazyDX() noexcept;

			virtual void CreateDeviceIndependentResources() = 0;
			virtual void CreateDeviceResources() = 0;
			virtual void DestroyDeviceIndependentResources() noexcept = 0;
			virtual void DestroyDeviceResources() noexcept = 0;
		private:
			struct InternalHook
			{
				InternalHook();
				~InternalHook();
				// User32.dll calls FreeLibrary to unload our module after UnhookWinEventHook, 
				// we hook this function so that we can release all dx resources
				// to avoid possible unhandled exception

				// Without the hook, injected processes may have small chances to crash...
				static BOOL WINAPI FreeLibrary(
					HMODULE hLibModule
				);

				void StartupHook();
				void ShutdownHook();

				bool hooked{false};
				std::vector<LazyDX*> dxList;
				decltype(FreeLibrary)* actualFreeLibrary{nullptr};
			};
			static InternalHook g_internalHook;
		};

		class LazyD3D : public LazyDX
		{
		public:
			friend class wil::manually_managed_shutdown_aware_object<LazyD3D>;
			static bool EnsureInitialized();
			static LazyD3D& GetInstance();
			virtual ~LazyD3D() noexcept = default;
			LazyD3D(const LazyD3D&) = delete;
			LazyD3D& operator=(const LazyD3D&) = delete;

			wil::com_ptr<IDXGIDevice3> GetDxgiDevice() const { return m_dxgiDevice; };
			wil::com_ptr<ID3D11Device> GetD3DDevice() const { return m_d3dDevice; };
			void ProcessShutdown() {}
		protected:
			void CreateDeviceIndependentResources() override;
			void CreateDeviceResources() override;
			void DestroyDeviceIndependentResources() noexcept override;
			void DestroyDeviceResources() noexcept override;
		private:
			LazyD3D();

			wil::com_ptr<IDXGIDevice3> m_dxgiDevice{nullptr};
			wil::com_ptr<ID3D11Device> m_d3dDevice{nullptr};
		};

		class LazyD2D : public LazyDX
		{
		public:
			friend class wil::manually_managed_shutdown_aware_object<LazyD2D>;
			static bool EnsureInitialized();
			static LazyD2D& GetInstance();
			virtual ~LazyD2D() noexcept = default;
			LazyD2D(const LazyD2D&) = delete;
			LazyD2D& operator=(const LazyD2D&) = delete;

			wil::com_ptr<ID2D1DCRenderTarget> GetRenderTarget() const { return m_dcRT; };
			wil::com_ptr<ID2D1Factory> GetFactory() const { return m_factory; };
			void ProcessShutdown() {}
		protected:
			void CreateDeviceIndependentResources() override;
			void CreateDeviceResources() override;
			void DestroyDeviceIndependentResources() noexcept override;
			void DestroyDeviceResources() noexcept override;
		private:
			LazyD2D();

			wil::com_ptr<ID2D1DCRenderTarget> m_dcRT{nullptr};
			wil::com_ptr<ID2D1Factory> m_factory{nullptr};
		};

		class LazyDComposition : public LazyDX
		{
		public:
			friend class wil::manually_managed_shutdown_aware_object<LazyDComposition>;
			static bool EnsureInitialized();
			static LazyDComposition& GetInstance();
			virtual ~LazyDComposition() noexcept = default;
			LazyDComposition(const LazyDComposition&) = delete;
			LazyDComposition& operator=(const LazyDComposition&) = delete;

			wil::com_ptr<IDCompositionDesktopDevice> GetDCompositionDevice() const { return m_dcompDevice; };
			void ProcessShutdown() {}
		protected:
			void CreateDeviceIndependentResources() override;
			void CreateDeviceResources() override;
			void DestroyDeviceIndependentResources() noexcept override;
			void DestroyDeviceResources() noexcept override;
		private:
			LazyDComposition();

			LazyD3D& m_lazyD3D{LazyD3D::GetInstance()};
			wil::com_ptr<IDCompositionDesktopDevice> m_dcompDevice{nullptr};
		};

		D2D1::ColorF MakeColorF(DWORD argb);
	}
}