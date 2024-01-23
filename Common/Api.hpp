#pragma once
#include "framework.h"
#include "cpprt.h"

namespace TranslucentFlyouts::Api
{
	struct ServiceInfo
	{
		HWND hostWindow{ nullptr };
		HWINEVENTHOOK hook{ nullptr };
	};
	using unique_service_info = wil::unique_mapview_ptr<ServiceInfo>;

	unique_service_info GetServiceInfo(std::wstring_view serviceName, bool readOnly = true);
	std::pair<wil::unique_handle, unique_service_info> CreateService(std::wstring_view serviceName);
	bool IsServiceRunning(std::wstring_view serviceName);
	bool IsHostProcess(std::wstring_view serviceName);
	bool IsCurrentProcessInBlockList();

	namespace InteractiveIO
	{
		enum class StringType
		{
			Notification,
			Warning,
			Error
		};
		enum class WaitType
		{
			NoWait,
			WaitYN,
			WaitAnyKey
		};

		// Return true/false if waitType is YN, otherwise always return true.
		bool OutputToConsole(
			StringType strType,
			WaitType waitType,
			UINT strResourceId,
			std::wstring_view prefixStr,
			std::wstring_view additionalStr,
			bool requireConsole = false
		);
		void Startup();
		void Shutdown();
	}

	bool IsPartDisabled(std::wstring_view part);
	bool IsPartDisabledExternally(std::wstring_view part = L"");
	bool IsStartAllBackTakingOver(std::wstring_view part = L"");
}