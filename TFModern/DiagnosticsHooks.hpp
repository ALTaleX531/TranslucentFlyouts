#pragma once
#include "pch.h"

namespace TranslucentFlyouts::DiagnosticsHooks
{
	namespace WUX
	{
		inline winrt::impl::abi_t<winrt::Windows::System::IDispatcherQueue>** (STDMETHODCALLTYPE* g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId)(
			struct Diagnostics_DiagnosticsInterop* This,
			winrt::impl::abi_t<winrt::Windows::System::IDispatcherQueue>** result,
			DWORD threadId
		)
		{
			nullptr
		};
	}
	namespace MUX
	{
		inline winrt::impl::abi_t<winrt::Microsoft::UI::Dispatching::IDispatcherQueue>** (STDMETHODCALLTYPE* g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId)(
			struct Diagnostics_DiagnosticsInterop* This,
			winrt::impl::abi_t<winrt::Microsoft::UI::Dispatching::IDispatcherQueue>** result,
			DWORD threadId
		)
		{
			nullptr
		};
	}

	void Prepare();
	void Startup();
	void Shutdown();
}