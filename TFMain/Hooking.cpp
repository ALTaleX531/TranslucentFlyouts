#include "pch.h"
#include "Utils.hpp"
#include "Hooking.hpp"

namespace TranslucentFlyouts::Hooking
{
	using namespace std;

	namespace Detours
	{
		// Begin to install hooks
		LONG Begin();
		// End attaching
		LONG End(bool commit = true);
	}
}

void TranslucentFlyouts::Hooking::WriteMemory(PVOID memoryAddress, function<void()> callback) try
{
	THROW_HR_IF_NULL(E_INVALIDARG, memoryAddress);
	THROW_HR_IF_NULL(E_INVALIDARG, callback);

	MEMORY_BASIC_INFORMATION mbi{};
	THROW_LAST_ERROR_IF(
		VirtualQuery(
			memoryAddress,
			&mbi,
			sizeof(MEMORY_BASIC_INFORMATION)
		) == 0
	);
	THROW_IF_WIN32_BOOL_FALSE(
		VirtualProtect(
			mbi.BaseAddress,
			mbi.RegionSize,
			PAGE_READWRITE,
			&mbi.Protect
		)
	);
	callback();
	THROW_IF_WIN32_BOOL_FALSE(
		VirtualProtect(
			mbi.BaseAddress,
			mbi.RegionSize,
			mbi.Protect,
			&mbi.Protect
		)
	);
}
CATCH_LOG_RETURN()

void TranslucentFlyouts::Hooking::WalkIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName)> callback) try
{
	THROW_HR_IF(E_INVALIDARG, dllName.empty());
	THROW_HR_IF_NULL(E_INVALIDARG, baseAddress);
	THROW_HR_IF_NULL(E_INVALIDARG, callback);

	ULONG size{0ul};
	auto importDescriptor
	{
		static_cast<PIMAGE_IMPORT_DESCRIPTOR>(
			ImageDirectoryEntryToData(
				baseAddress,
				TRUE,
				IMAGE_DIRECTORY_ENTRY_IMPORT,
				&size
			)
		)
	};

	THROW_HR_IF_NULL(E_INVALIDARG, importDescriptor);

	LPCSTR moduleName{nullptr};
	while (importDescriptor->Name != 0)
	{
		moduleName = reinterpret_cast<LPCSTR>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->Name);

		if (!_stricmp(moduleName, dllName.data()))
		{
			break;
		}

		importDescriptor++;
	}

	THROW_HR_IF_NULL_MSG(E_NOT_SET, moduleName, "Cannot find specific module for [%hs]!", dllName.data());
	THROW_HR_IF_MSG(E_NOT_SET, importDescriptor->Name == 0, "Cannot find specific module for [%hs]!", dllName.data());

	auto thunk{reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->FirstThunk)};
	auto nameThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->OriginalFirstThunk);

	bool result{true};
	while (thunk->u1.Function)
	{
		LPCSTR functionName{nullptr};
		auto functionAddress = reinterpret_cast<PVOID*>(&thunk->u1.Function);

		BOOL importedByName{!IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)};
		if (importedByName)
		{
			functionName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(reinterpret_cast<UINT_PTR>(baseAddress) + static_cast<RVA>(nameThunk->u1.AddressOfData))->Name;
		}
		else
		{
			functionName = MAKEINTRESOURCEA(IMAGE_ORDINAL(thunk->u1.Ordinal));
		}

		result = callback(functionAddress, functionName, importedByName);
		if (!result)
		{
			break;
		}

		thunk++;
		nameThunk++;
	}
}
CATCH_LOG_RETURN()

void TranslucentFlyouts::Hooking::WalkDeleyloadIAT(PVOID baseAddress, string_view dllName, function<bool(HMODULE* moduleHandle, PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName)> callback) try
{
	THROW_HR_IF(E_INVALIDARG, dllName.empty());
	THROW_HR_IF_NULL(E_INVALIDARG, baseAddress);
	THROW_HR_IF_NULL(E_INVALIDARG, callback);

	ULONG size{0ul};
	auto importDescriptor
	{
		static_cast<PIMAGE_DELAYLOAD_DESCRIPTOR>(
			ImageDirectoryEntryToData(
				baseAddress,
				TRUE,
				IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
				&size
			)
		)
	};

	THROW_HR_IF_NULL(E_INVALIDARG, importDescriptor);

	LPCSTR moduleName{nullptr};
	while (importDescriptor->DllNameRVA != 0)
	{
		moduleName = reinterpret_cast<LPCSTR>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->DllNameRVA);

		if (!_stricmp(moduleName, dllName.data()))
		{
			break;
		}

		importDescriptor++;
	}

	THROW_HR_IF_NULL_MSG(E_NOT_SET, moduleName, "Cannot find specific module for [%hs]!", dllName.data());
	THROW_HR_IF_MSG(E_NOT_SET, importDescriptor->DllNameRVA == 0, "Cannot find specific module for [%hs]!", dllName.data());

	auto attributes{importDescriptor->Attributes.RvaBased};
	THROW_HR_IF_MSG(E_NOT_SET, attributes != 1, "Unsupported delay loaded dll![%hs]", dllName.data());

	auto moduleHandle = reinterpret_cast<HMODULE*>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->ModuleHandleRVA);
	auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->ImportAddressTableRVA);
	auto nameThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->ImportNameTableRVA);

	bool result{true};
	while (thunk->u1.Function)
	{
		LPCSTR functionName{nullptr};
		auto functionAddress = reinterpret_cast<PVOID*>(&thunk->u1.Function);

		BOOL importedByName{!IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)};
		if (importedByName)
		{
			functionName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(reinterpret_cast<UINT_PTR>(baseAddress) + static_cast<RVA>(nameThunk->u1.AddressOfData))->Name;
		}
		else
		{
			functionName = MAKEINTRESOURCEA(IMAGE_ORDINAL(thunk->u1.Ordinal));
		}

		result = callback(moduleHandle, functionAddress, functionName, importedByName);
		if (!result)
		{
			break;
		}

		thunk++;
		nameThunk++;
	}
}
CATCH_LOG_RETURN()

size_t TranslucentFlyouts::Hooking::WriteIAT(PVOID baseAddress, std::string_view dllName, std::unordered_map<LPCSTR, PVOID> hookMap)
{
	size_t result{hookMap.size()};

	Hooking::WalkIAT(baseAddress, dllName, [&](PVOID * functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName) -> bool
	{
		auto tempHookMap{hookMap};
		for (auto [targetFunctionNameOrOrdinal, hookFunctionAddress] : tempHookMap)
		{
			if (
				(importedByName == TRUE && !strcmp(functionNameOrOrdinal, targetFunctionNameOrOrdinal)) ||
				(importedByName == FALSE && functionNameOrOrdinal == targetFunctionNameOrOrdinal)
			)
			{
				Hooking::WriteMemory(functionAddress, [&]()
				{
					InterlockedExchangePointer(functionAddress, hookFunctionAddress);
				});
				hookMap.erase(targetFunctionNameOrOrdinal);

				break;
			}
		}

		if (hookMap.empty())
		{
			return false;
		}

		return true;
	});

	result -= hookMap.size();

	return result;
}

size_t TranslucentFlyouts::Hooking::WriteDelayloadIAT(PVOID baseAddress, std::string_view dllName, std::unordered_map<LPCSTR, PVOID> hookMap)
{
	size_t result{hookMap.size()};

	Hooking::WalkDeleyloadIAT(baseAddress, dllName, [&](HMODULE * moduleHandle, PVOID * functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName) -> bool
	{
		auto tempHookMap{hookMap};
		for (auto [targetFunctionNameOrOrdinal, hookFunctionAddress] : tempHookMap)
		{
			if (
				(importedByName == TRUE && !strcmp(functionNameOrOrdinal, targetFunctionNameOrOrdinal)) ||
				(importedByName == FALSE && functionNameOrOrdinal == targetFunctionNameOrOrdinal)
			)
			{
				if (*moduleHandle == nullptr)
				{
					Hooking::WriteMemory(moduleHandle, [&]()
					{
						InterlockedExchangePointer(reinterpret_cast<PVOID*>(moduleHandle), GetModuleHandleA(dllName.data()));
					});
				}

				Hooking::WriteMemory(functionAddress, [&]()
				{
					InterlockedExchangePointer(functionAddress, hookFunctionAddress);
				});
				hookMap.erase(targetFunctionNameOrOrdinal);

				break;
			}
		}

		if (hookMap.empty())
		{
			return false;
		}

		return true;
	});

	result -= hookMap.size();

	return result;
}

LONG TranslucentFlyouts::Hooking::Detours::Begin()
{
	DetourSetIgnoreTooSmall(TRUE);
	RETURN_IF_WIN32_ERROR(DetourTransactionBegin());
	RETURN_IF_WIN32_ERROR(DetourUpdateThread(GetCurrentThread()));
	return ERROR_SUCCESS;
}

LONG TranslucentFlyouts::Hooking::Detours::End(bool commit)
{
	return ((commit ? DetourTransactionCommit() : DetourTransactionAbort()));
}

HRESULT TranslucentFlyouts::Hooking::Detours::Write(function<void()> callback) try
{
	HRESULT hr{HRESULT_FROM_WIN32(Hooking::Detours::Begin())};
	if (FAILED(hr))
	{
		return hr;
	}

	callback();

	return HRESULT_FROM_WIN32(Hooking::Detours::End(true));
}
catch (...)
{
	LOG_CAUGHT_EXCEPTION();
	Hooking::Detours::End(false);
	return wil::ResultFromCaughtException();
}

void TranslucentFlyouts::Hooking::Detours::Attach(string_view dllName, string_view funcName, PVOID* realFuncAddr, PVOID hookFuncAddr)
{
	THROW_HR_IF_NULL(E_INVALIDARG, realFuncAddr);
	THROW_HR_IF_NULL(E_INVALIDARG, *realFuncAddr);
	*realFuncAddr = DetourFindFunction(dllName.data(), funcName.data());
	THROW_LAST_ERROR_IF_NULL(*realFuncAddr);

	Attach(realFuncAddr, hookFuncAddr);
}

void TranslucentFlyouts::Hooking::Detours::Attach(PVOID* realFuncAddr, PVOID hookFuncAddr)
{
	THROW_HR_IF_NULL(E_INVALIDARG, realFuncAddr);
	THROW_HR_IF_NULL(E_INVALIDARG, *realFuncAddr);
	THROW_HR_IF_NULL(E_INVALIDARG, hookFuncAddr);

	THROW_IF_WIN32_ERROR(DetourAttach(realFuncAddr, hookFuncAddr));
}

void TranslucentFlyouts::Hooking::Detours::Detach(PVOID* realFuncAddr, PVOID hookFuncAddr)
{
	THROW_IF_WIN32_ERROR(DetourDetach(realFuncAddr, hookFuncAddr));
}

TranslucentFlyouts::Hooking::DllNotifyRoutine& TranslucentFlyouts::Hooking::DllNotifyRoutine::GetInstance()
{
	static DllNotifyRoutine instance{};
	return instance;
};

TranslucentFlyouts::Hooking::DllNotifyRoutine::DllNotifyRoutine()
{
	try
	{
		m_actualLdrRegisterDllNotification = reinterpret_cast<NTSTATUS(NTAPI*)(ULONG, PLDR_DLL_NOTIFICATION_FUNCTION, PVOID, PVOID*)>(DetourFindFunction("ntdll.dll", "LdrRegisterDllNotification"));
		THROW_LAST_ERROR_IF_NULL(m_actualLdrRegisterDllNotification);

		m_actualLdrUnregisterDllNotification = reinterpret_cast<NTSTATUS(NTAPI*)(PVOID)>(DetourFindFunction("ntdll.dll", "LdrUnregisterDllNotification"));
		THROW_LAST_ERROR_IF_NULL(m_actualLdrUnregisterDllNotification);

		THROW_IF_NTSTATUS_FAILED(m_actualLdrRegisterDllNotification(0, LdrDllNotification, this, &m_cookie));
	}
	catch (...)
	{
		m_internalError = true;
		LOG_CAUGHT_EXCEPTION();
	}
}

TranslucentFlyouts::Hooking::DllNotifyRoutine::~DllNotifyRoutine() noexcept
{
	if (!m_actualLdrUnregisterDllNotification)
	{
		return;
	}
	if (!m_cookie)
	{
		return;
	}

	m_actualLdrUnregisterDllNotification(m_cookie);
	m_cookie = nullptr;
}

VOID CALLBACK TranslucentFlyouts::Hooking::DllNotifyRoutine::LdrDllNotification(
	ULONG NotificationReason,
	PCLDR_DLL_NOTIFICATION_DATA NotificationData,
	PVOID Context
)
{
	DllNotifyRoutine& dllnotifyRoutine{*reinterpret_cast<DllNotifyRoutine*>(Context)};
	if (!dllnotifyRoutine.m_callbackList.empty())
	{
		for (const auto& callback : dllnotifyRoutine.m_callbackList)
		{
			if (callback)
			{
				bool load{NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED ? true : false};
				DllInfo info
				{
					load ? NotificationData->Loaded.FullDllName : NotificationData->Unloaded.FullDllName,
					load ? NotificationData->Loaded.BaseDllName : NotificationData->Unloaded.BaseDllName,
					load ? NotificationData->Loaded.DllBase : NotificationData->Unloaded.DllBase,
					load ? NotificationData->Loaded.SizeOfImage : NotificationData->Unloaded.SizeOfImage,
				};
				callback(
					load,
					info
				);
			}
		}
	}
}

void TranslucentFlyouts::Hooking::DllNotifyRoutine::AddCallback(Callback callback)
{
	m_callbackList.push_back(callback);
}

void TranslucentFlyouts::Hooking::DllNotifyRoutine::DeleteCallback(Callback callback)
{
	for (auto it = m_callbackList.begin(); it != m_callbackList.end();)
	{
		auto& callback{*it};
		if (*callback.target<void(bool load, DllInfo info)>() == *callback.target<void(bool load, DllInfo info)>())
		{
			it = m_callbackList.erase(it);
			break;
		}
		else
		{
			it++;
		}
	}
}

TranslucentFlyouts::Hooking::MsgHooks& TranslucentFlyouts::Hooking::MsgHooks::GetInstance()
{
	static MsgHooks instance{};
	return instance;
};

TranslucentFlyouts::Hooking::MsgHooks::~MsgHooks()
{
	UninstallAll();
}

LRESULT CALLBACK TranslucentFlyouts::Hooking::MsgHooks::CallWndHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	auto& hookInfo{GetInstance().m_hookMap[GetCurrentThreadId()]};
	auto& callbackList{GetInstance().m_callbackList};
	if (code == HC_ACTION)
	{
		auto callWndStruct{reinterpret_cast<LPCWPSTRUCT>(lParam)};
		if (!callbackList.empty())
		{
			for (const auto& callback : callbackList)
			{
				if (callback)
				{
					callback(callWndStruct->hwnd, callWndStruct->message, callWndStruct->wParam, callWndStruct->lParam, 0, false);
				}
			}
		}
	}
	return CallNextHookEx(hookInfo.callWindowHook.get(), code, wParam, lParam);
}

LRESULT CALLBACK TranslucentFlyouts::Hooking::MsgHooks::CallWndRetHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	auto& hookInfo{GetInstance().m_hookMap[GetCurrentThreadId()]};
	auto& callbackList{GetInstance().m_callbackList};
	if (code == HC_ACTION)
	{
		auto callWndRetStruct{reinterpret_cast<LPCWPRETSTRUCT>(lParam)};
		if (!callbackList.empty())
		{
			for (const auto& callback : callbackList)
			{
				if (callback)
				{
					callback(callWndRetStruct->hwnd, callWndRetStruct->message, callWndRetStruct->wParam, callWndRetStruct->lParam, callWndRetStruct->lResult, true);
				}
			}
		}
	}
	return CallNextHookEx(hookInfo.callWindowRetHook.get(), code, wParam, lParam);
}

void TranslucentFlyouts::Hooking::MsgHooks::AddCallback(Callback callback)
{
	m_callbackList.push_back(callback);
}

void TranslucentFlyouts::Hooking::MsgHooks::DeleteCallback(Callback callback)
{
	for (auto it = m_callbackList.begin(); it != m_callbackList.end();)
	{
		auto& callback{*it};
		if (*callback.target<void(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid)>() == *callback.target<void(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid)>())
		{
			it = m_callbackList.erase(it);
			break;
		}
		else
		{
			it++;
		}
	}
}

void TranslucentFlyouts::Hooking::MsgHooks::Install(HWND hWnd)
{
	DWORD threadId{GetWindowThreadProcessId(hWnd, nullptr)};

	if (m_hookMap.find(threadId) == m_hookMap.end())
	{
		m_hookMap[threadId].callWindowHook.reset(SetWindowsHookExW(WH_CALLWNDPROC, CallWndHookProc, nullptr, threadId));
		m_hookMap[threadId].callWindowRetHook.reset(SetWindowsHookExW(WH_CALLWNDPROCRET, CallWndRetHookProc, nullptr, threadId));
	}

	m_hookMap[threadId].windowList.push_back(hWnd);
}

void TranslucentFlyouts::Hooking::MsgHooks::Uninstall(HWND hWnd)
{
	DWORD threadId{GetWindowThreadProcessId(hWnd, nullptr)};

	if (m_hookMap.find(threadId) == m_hookMap.end())
	{
		return;
	}

	auto& windowMap{m_hookMap[threadId].windowList};
	for (auto it = windowMap.begin(); it != windowMap.end();)
	{
		auto& window = *it;
		if (window == hWnd)
		{
			it = windowMap.erase(it);
			break;
		}
		else
		{
			it++;
		}
	}

	if (windowMap.empty())
	{
		m_hookMap.erase(threadId);
	}
}

void TranslucentFlyouts::Hooking::MsgHooks::UninstallAll()
{
	m_hookMap.clear();
}

TranslucentFlyouts::Hooking::FunctionCallHook::~FunctionCallHook()
{
	Detach();
}

void TranslucentFlyouts::Hooking::FunctionCallHook::InitJumpRegion(PVOID startAddress)
{
	m_jumpRegion.reset(
		static_cast<std::byte*>(DetourAllocateRegionWithinJumpBounds(startAddress, &m_size))
	);

	m_current = m_jumpRegion.get();
	m_end = m_current + m_size;
}

std::byte* TranslucentFlyouts::Hooking::FunctionCallHook::AllocJmpCode(PVOID detourDestination)
{
	LOG_HR_IF(E_INVALIDARG, !m_jumpRegion);
	if (!m_jumpRegion)
	{
		return nullptr;
	}

#ifdef _WIN64
	std::byte jmpCode[14] {std::byte{0xFF}, std::byte{0x25}};
	memcpy_s(&jmpCode[6], sizeof(PVOID), &detourDestination, sizeof(detourDestination));
#else
	std::byte jmpCode[] {std::byte{0xB8}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xFF}, std::byte{0xE0}, std::byte{0}};
	memcpy_s(&jmpCode[1], sizeof(PVOID), &detourDestination, sizeof(detourDestination));
#endif

	if (m_current + sizeof(jmpCode) > m_end)
	{
		return nullptr;
	}

	auto startPos = m_current;
	memcpy(startPos, jmpCode, sizeof(jmpCode));
	m_current += sizeof(jmpCode);

	return startPos;
}

void TranslucentFlyouts::Hooking::FunctionCallHook::Attach(PVOID functionThunkAddress, PVOID detourAddress, PVOID detourDestination, int callersCount)
{
	LOG_HR_IF(E_INVALIDARG, !functionThunkAddress);
	LOG_HR_IF(E_INVALIDARG, !detourAddress);
	LOG_HR_IF(E_INVALIDARG, !detourDestination);
	LOG_HR_IF(E_INVALIDARG, !callersCount);

	if (!functionThunkAddress)
	{
		return;
	}
	if (!detourAddress)
	{
		return;
	}
	if (!detourDestination)
	{
		return;
	}
	if (!m_jumpRegion)
	{
		InitJumpRegion(functionThunkAddress);
	}
	
	auto jmpBytes{AllocJmpCode(detourDestination)};
	auto functionBytes{reinterpret_cast<std::byte*>(functionThunkAddress)};
	auto functionIsEnd = [](std::byte * bytes, int offset)
	{
#ifdef _WIN64
		if (
			bytes[offset] == std::byte{0xC3} &&
			bytes[offset + 1] == std::byte{0xCC} &&
			bytes[offset + 2] == std::byte{0xCC} &&
			bytes[offset + 3] == std::byte{0xCC} &&
			bytes[offset + 4] == std::byte{0xCC} &&
			bytes[offset + 5] == std::byte{0xCC} &&
			bytes[offset + 6] == std::byte{0xCC} &&
			bytes[offset + 7] == std::byte{0xCC} &&
			bytes[offset + 8] == std::byte{0xCC}
		)
#else
		if (
			bytes[offset] == std::byte{0xC2} &&
			bytes[offset + 2] == std::byte{0x00}
		)
#endif
		{
			return true;
		}
		return false;
	};

	LOG_HR_IF(E_INVALIDARG, !jmpBytes);
	if (!jmpBytes)
	{
		return;
	}

	for (int i = 0; !functionIsEnd(functionBytes, i) && i < 65535 && callersCount; i++)
	{
		if (functionBytes[i] == std::byte{0xE8})
		{
			auto offsetAddress{reinterpret_cast<LONG*>(&functionBytes[i + 1])};
			auto offset{*offsetAddress};
			auto callBaseAddress{&functionBytes[i + 5]};
			auto targetAddress{reinterpret_cast<PVOID>(callBaseAddress + offset)};

			if (targetAddress == detourAddress)
			{
				WriteMemory(offsetAddress, [&]()
				{
					*offsetAddress = static_cast<LONG>(jmpBytes - callBaseAddress);
					m_hookedMap[offsetAddress] = offset;
					callersCount--;
				});
			}
		}
	}
}

void TranslucentFlyouts::Hooking::FunctionCallHook::Detach()
{
	for (const auto [offsetAddress, offset] : m_hookedMap)
	{
		WriteMemory(offsetAddress, [&]()
		{
			*offsetAddress = offset;
		});
	}
}
