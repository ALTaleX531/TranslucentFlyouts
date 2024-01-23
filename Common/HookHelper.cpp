#include "Utils.hpp"
#include "HookHelper.hpp"
#include "detours.h"

using namespace TranslucentFlyouts;

namespace TranslucentFlyouts::HookHelper::Detours
{
	// Begin to install hooks
	HRESULT Begin();
	// End attaching
	HRESULT End(bool commit = true);
}

[[maybe_unused]] size_t InvokeAccessViolation(void(*vtable_function)(void*))
{
	static struct
	{
		const void* vtable = nullptr;
	} FakeVtableContainer;

	auto exception_handler = [](size_t& offset, EXCEPTION_POINTERS* exception_pointers) -> int
	{
		offset = exception_pointers->ExceptionRecord->ExceptionInformation[1];
		return EXCEPTION_EXECUTE_HANDLER;
	};
	size_t offset = -1;
	__try { vtable_function(&FakeVtableContainer); }
	__except (exception_handler(offset, GetExceptionInformation())) {}

	return offset / sizeof(void*);
}

HookHelper::ThreadSnapshotExcludeSelf::ThreadSnapshotExcludeSelf()
{
	if (PssCaptureSnapshot(GetCurrentProcess(), PSS_CAPTURE_THREADS, 0, &m_snapshot))
	{
		return;
	}
}

HookHelper::ThreadSnapshotExcludeSelf::~ThreadSnapshotExcludeSelf()
{
	if (m_snapshot)
	{
		PssFreeSnapshot(GetCurrentProcess(), m_snapshot);
		m_snapshot = nullptr;
	}
}

void HookHelper::ThreadSnapshotExcludeSelf::Suspend()
{
	ForEach([](const PSS_THREAD_ENTRY& threadEntry)
	{
		wil::unique_handle threadHanle{ OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.ThreadId) };
		if (threadHanle)
		{
			SuspendThread(threadHanle.get());
		}

		return true;
	});
}
void HookHelper::ThreadSnapshotExcludeSelf::Resume()
{
	ForEach([](const PSS_THREAD_ENTRY& threadEntry)
	{
		wil::unique_handle threadHanle{ OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.ThreadId) };
		if (threadHanle)
		{
			ResumeThread(threadHanle.get());
		}

		return true;
	});
}

DWORD HookHelper::ThreadSnapshotExcludeSelf::GetMainThreadId()
{
	DWORD threadId{0};
	ForEach([&](const PSS_THREAD_ENTRY& threadEntry)
	{
		threadId = threadEntry.ThreadId;

		return false;
	});

	return threadId;
}

void HookHelper::ThreadSnapshotExcludeSelf::ForEach(const std::function<bool(const PSS_THREAD_ENTRY&)>&& callback)
{
	HPSSWALK walk{ nullptr };
	if (PssWalkMarkerCreate(nullptr, &walk))
	{
		return;
	}
	auto cleanUp = [&]
	{
		if (walk)
		{
			PssWalkMarkerFree(walk);
			walk = nullptr;
		}
	};

	PSS_THREAD_ENTRY threadEntry{};
	while (!PssWalkSnapshot(m_snapshot, PSS_WALK_THREADS, walk, &threadEntry, sizeof(threadEntry)))
	{
		if (threadEntry.ThreadId != GetCurrentThreadId())
		{
			if (!callback(threadEntry))
			{
				return;
			}
		}
	}
}

void HookHelper::WriteMemory(PVOID memoryAddress, const std::function<void()>&& callback) try
{
	THROW_HR_IF_NULL(E_INVALIDARG, memoryAddress);

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
			PAGE_EXECUTE_READWRITE,
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
catch(...) {}

PVOID HookHelper::InjectCallbackToThread(DWORD threadId, const std::function<void()>& callback) try
{
#ifdef _WIN64
wil::unique_handle threadHandle{ OpenThread(THREAD_SET_CONTEXT | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, threadId) };
	THROW_LAST_ERROR_IF_NULL(threadHandle);

	THROW_LAST_ERROR_IF(SuspendThread(threadHandle.get()) == -1);
	auto cleanUp = wil::scope_exit([&]
	{
		THROW_LAST_ERROR_IF(ResumeThread(threadHandle.get()) == -1);
	});

	CONTEXT context{};
	context.ContextFlags = CONTEXT_FULL;
	THROW_LAST_ERROR_IF(!GetThreadContext(threadHandle.get(), &context));

	auto proxyCallback = [](ULONG_PTR parameter) -> void
	{
		auto callback_ptr{ reinterpret_cast<std::function<void()>*>(parameter) };
		auto callback{ *callback_ptr };
		delete callback_ptr;

		callback();
	};

	static BYTE s_shellCode[]
	{
		// sub rsp, 28h
		0x48, 0x83, 0xec, 0x28,
		// mov [rsp + 18], rax
		0x48, 0x89, 0x44, 0x24, 0x18,
		// mov [rsp + 10h], rcx
		0x48, 0x89, 0x4c, 0x24, 0x10,
		// mov rcx, function_parameter
		0x48, 0xb9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		// mov rax, function_pointer
		0x48, 0xb8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		// call rax
		0xff, 0xd0,
		// mov rcx, [rsp + 10h]
		0x48, 0x8b, 0x4c, 0x24, 0x10,
		// mov rax, [rsp + 18h]
		0x48, 0x8b, 0x44, 0x24, 0x18,
		// add rsp, 28h
		0x48, 0x83, 0xc4, 0x28,
		// mov r11, context.Rip
		0x49, 0xbb, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
		// jmp r11
		0x41, 0xff, 0xe3
	};
	*reinterpret_cast<PVOID*>(&s_shellCode[16]) = static_cast<void*>(new std::function<void()>{ callback });
	*reinterpret_cast<PVOID*>(&s_shellCode[26]) = static_cast<void*>(static_cast<void(WINAPI*)(ULONG_PTR)>(proxyCallback));
	*reinterpret_cast<ULONG_PTR*>(&s_shellCode[52]) = context.Rip;

	auto buffer
	{ 
		VirtualAlloc(
			nullptr,
			sizeof(s_shellCode),
			MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE
		) 
	};
	memcpy_s(buffer, sizeof(s_shellCode), s_shellCode, sizeof(s_shellCode));
	context.Rip = reinterpret_cast<ULONG_PTR>(buffer);
	THROW_LAST_ERROR_IF(!SetThreadContext(threadHandle.get(), &context));

	return buffer;
#else
	return nullptr;
#endif
}
catch(...) { return nullptr; }

HMODULE HookHelper::GetProcessModule(HANDLE processHandle, std::wstring_view dllPath)
{
	HMODULE targetModule{ nullptr };
	DWORD bytesNeeded{ 0 };
	if (!EnumProcessModules(processHandle, nullptr, 0, &bytesNeeded))
	{
		return targetModule;
	}
	DWORD moduleCount{ bytesNeeded / sizeof(HMODULE) };
	auto moduleList{ std::make_unique<HMODULE[]>(moduleCount) };
	if (!EnumProcessModules(processHandle, moduleList.get(), bytesNeeded, &bytesNeeded))
	{
		return targetModule;
	}

	for (DWORD i = 0; i < moduleCount; i++)
	{
		HMODULE moduleHandle{ moduleList[i] };
		WCHAR modulePath[MAX_PATH + 1];
		GetModuleFileNameExW(processHandle, moduleHandle, modulePath, MAX_PATH);

		if (!_wcsicmp(modulePath, dllPath.data()))
		{
			targetModule = moduleHandle;
			break;
		}
	}

	return targetModule;
}

void HookHelper::WalkIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName)> callback) try
{
	THROW_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(baseAddress));
	THROW_HR_IF(E_INVALIDARG, dllName.empty());
	THROW_HR_IF_NULL(E_INVALIDARG, baseAddress);
	THROW_HR_IF_NULL(E_INVALIDARG, callback);

	ULONG size{ 0ul };
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

	bool found = false;
	while (importDescriptor->Name)
	{
		auto moduleName = reinterpret_cast<LPCSTR>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->Name);

		if (!_stricmp(moduleName, dllName.data()))
		{
			found = true;
			break;
		}

		importDescriptor++;
	}

	THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !found);

	auto thunk{ reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->FirstThunk) };
	auto nameThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<UINT_PTR>(baseAddress) + importDescriptor->OriginalFirstThunk);

	bool result{ true };
	while (thunk->u1.Function)
	{
		LPCSTR functionName{ nullptr };
		auto functionAddress = reinterpret_cast<PVOID*>(&thunk->u1.Function);

		BOOL importedByName{ !IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal) };
		if (importedByName)
		{
			functionName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
				RVA_TO_ADDR(baseAddress, static_cast<RVA>(nameThunk->u1.AddressOfData))
				)->Name;
		}
		else
		{
			functionName = MAKEINTRESOURCEA(IMAGE_ORDINAL(nameThunk->u1.Ordinal));
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
catch (...) {}

void HookHelper::WalkDelayloadIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(HMODULE* moduleHandle, PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName)> callback) try
{
	THROW_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(baseAddress));
	THROW_HR_IF(E_INVALIDARG, dllName.empty());
	THROW_HR_IF_NULL(E_INVALIDARG, baseAddress);
	THROW_HR_IF_NULL(E_INVALIDARG, callback);

	ULONG size{ 0ul };
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

	bool found = false;
	while (importDescriptor->DllNameRVA)
	{
		auto moduleName = reinterpret_cast<LPCSTR>(
			RVA_TO_ADDR(baseAddress, importDescriptor->DllNameRVA)
			);

		if (!_stricmp(moduleName, dllName.data()))
		{
			found = true;
			break;
		}

		importDescriptor++;
	}

	THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !found);

	auto attributes{ importDescriptor->Attributes.RvaBased };
	THROW_WIN32_IF_MSG(ERROR_FILE_NOT_FOUND, attributes != 1, "Unsupported delay loaded dll![%hs]", dllName.data());

	auto moduleHandle = reinterpret_cast<HMODULE*>(
		RVA_TO_ADDR(baseAddress, importDescriptor->ModuleHandleRVA)
		);
	auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
		RVA_TO_ADDR(baseAddress, importDescriptor->ImportAddressTableRVA)
		);
	auto nameThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
		RVA_TO_ADDR(baseAddress, importDescriptor->ImportNameTableRVA)
		);

	bool result{ true };
	while (thunk->u1.Function)
	{
		LPCSTR functionName{ nullptr };
		auto functionAddress = reinterpret_cast<PVOID*>(&thunk->u1.Function);

		BOOL importedByName{ !IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal) };
		if (importedByName)
		{
			functionName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
				RVA_TO_ADDR(baseAddress, nameThunk->u1.AddressOfData)
				)->Name;
		}
		else
		{
			functionName = MAKEINTRESOURCEA(IMAGE_ORDINAL(nameThunk->u1.Ordinal));
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
catch (...) {}

PVOID* HookHelper::GetIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal)
{
	PVOID* originalFunction{ nullptr };
	WalkIAT(baseAddress, dllName, [&](PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName) -> bool
	{
		if (
			(importedByName == TRUE && targetFunctionNameOrOrdinal && !strcmp(functionNameOrOrdinal, targetFunctionNameOrOrdinal)) ||
			(importedByName == FALSE && functionNameOrOrdinal == targetFunctionNameOrOrdinal)
		)
		{
			originalFunction = functionAddress;

			return false;
		}

		return true;
	});

	return originalFunction;
}
std::pair<HMODULE*, PVOID*> HookHelper::GetDelayloadIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, bool resolveAPI)
{
	std::pair<HMODULE*, PVOID*> originalInfo{nullptr, nullptr};
	WalkDelayloadIAT(baseAddress, dllName, [&](HMODULE* moduleHandle, PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName) -> bool
	{
		if (
			(importedByName == TRUE && targetFunctionNameOrOrdinal && (reinterpret_cast<DWORD64>(targetFunctionNameOrOrdinal) & 0xFFFF0000) != 0 && !strcmp(functionNameOrOrdinal, targetFunctionNameOrOrdinal)) ||
			(importedByName == FALSE && functionNameOrOrdinal == targetFunctionNameOrOrdinal)
		)
		{
			originalInfo.first = moduleHandle;
			originalInfo.second = functionAddress;

			if (resolveAPI)
			{
				ResolveDelayloadIAT(originalInfo, baseAddress, dllName, targetFunctionNameOrOrdinal);
			}
			return false;
		}

		return true;
	});

	return originalInfo;
}

PVOID HookHelper::WriteIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, PVOID detourFunction)
{
	PVOID originalFunction{ nullptr };

	auto functionAddress{ GetIAT(baseAddress, dllName, targetFunctionNameOrOrdinal) };
	if (functionAddress)
	{
		originalFunction = *functionAddress;

		WriteMemory(functionAddress, [&]()
		{
			InterlockedExchangePointer(functionAddress, detourFunction);
		});
	}

	return originalFunction;
}
std::pair<HMODULE, PVOID> HookHelper::WriteDelayloadIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, PVOID detourFunction, std::optional<HMODULE> newModuleHandle)
{
	HMODULE originalModule{ nullptr };
	PVOID originalFunction{ nullptr };

	auto [moduleHandle, functionAddress]{GetDelayloadIAT(baseAddress, dllName, targetFunctionNameOrOrdinal, true)};
	if (functionAddress)
	{
		originalModule = *moduleHandle;
		originalFunction = *functionAddress;

		if (newModuleHandle)
		{
			WriteMemory(moduleHandle, [&]()
			{
				InterlockedExchangePointer(reinterpret_cast<PVOID*>(moduleHandle), newModuleHandle.value());
			});
		}
		WriteMemory(functionAddress, [&]()
		{
			InterlockedExchangePointer(functionAddress, detourFunction);
		});
	}

	return std::make_pair(originalModule, originalFunction);
}

void HookHelper::ResolveDelayloadIAT(const std::pair<HMODULE*, PVOID*>& info, PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal)
{
	auto& [moduleHandle, functionAddress]{info};
	if (DetourGetContainingModule(*functionAddress) == baseAddress || DetourGetContainingModule(DetourCodeFromPointer(*functionAddress, nullptr)) == baseAddress)
	{
		if (!(*moduleHandle))
		{
			HMODULE importModule{ LoadLibraryA(dllName.data()) };

			WriteMemory(moduleHandle, [&]()
			{
				InterlockedExchangePointer(reinterpret_cast<PVOID*>(moduleHandle), importModule);
			});
			WriteMemory(functionAddress, [&]()
			{
				InterlockedExchangePointer(reinterpret_cast<PVOID*>(functionAddress), GetProcAddress(importModule, targetFunctionNameOrOrdinal));
			});
		}
		else
		{
			WriteMemory(functionAddress, [&]()
			{
				InterlockedExchangePointer(reinterpret_cast<PVOID*>(functionAddress), GetProcAddress(*moduleHandle, targetFunctionNameOrOrdinal));
			});
		}
	}
}

PUCHAR HookHelper::MatchAsmCode(PUCHAR startAddress, std::pair<UCHAR*, size_t> asmCodeInfo, std::vector<std::pair<size_t, size_t>> asmSliceInfo, size_t maxSearchBytes)
{
	PUCHAR matchedAddress{ nullptr };
	if (!startAddress)
	{
		return matchedAddress;
	}

	size_t currentMaxSearchBytes{ 0 };
	size_t matchedBytesLength{ 0 };
	size_t sliceIndex{ 0 };

	auto [asmCode, asmCodeLength] { asmCodeInfo };
	while (matchedBytesLength < asmCodeLength)
	{
		auto [section_length, gap_length] { asmSliceInfo[sliceIndex] };
		
		if (section_length)
		{
			if (sliceIndex == 0)
			{
				while (memcmp(startAddress, &asmCode[matchedBytesLength], section_length) != 0)
				{
					startAddress++;
					maxSearchBytes--;
					if (!maxSearchBytes)
					{
						goto leave;
					}

					continue;
				}
			}
			else if (memcmp(startAddress, &asmCode[matchedBytesLength], section_length) != 0)
			{
				startAddress = matchedAddress + 1;
				maxSearchBytes--;
				matchedAddress = nullptr;
				matchedBytesLength = 0;
				sliceIndex = 0;
				maxSearchBytes = currentMaxSearchBytes;
				if (!maxSearchBytes)
				{
					goto leave;
				}

				continue;
			}

			if (sliceIndex == 0)
			{
				matchedAddress = startAddress;
				currentMaxSearchBytes = maxSearchBytes;
			}
		}
		startAddress += (section_length + gap_length);
		maxSearchBytes -= (section_length + gap_length);
		matchedBytesLength += (section_length + gap_length);
		sliceIndex++;

		if (!maxSearchBytes || sliceIndex >= asmSliceInfo.size())
		{
			goto leave;
		}
	}

leave:
	return matchedAddress;
}

std::pair<UCHAR*, size_t> HookHelper::GetTextSectionInfo(PVOID baseAddress)
{
	auto ntHeader{ ImageNtHeader(baseAddress) };
	auto sectionCount{ ntHeader->FileHeader.NumberOfSections };
	auto section{ IMAGE_FIRST_SECTION(ntHeader) };
	for (WORD i = 0; i < sectionCount; i++)
	{
		if (!strcmp(reinterpret_cast<const char*>(section->Name), ".text"))
		{
			return std::make_pair<UCHAR*, size_t>(
				reinterpret_cast<UCHAR*>(reinterpret_cast<ULONG_PTR>(baseAddress) + section->VirtualAddress),
				section->Misc.VirtualSize
			);
		}
		section++;
	}

	return std::make_pair<UCHAR*, size_t>(nullptr, 0);
}

HookHelper::ImageMapper::ImageMapper(std::wstring_view dllPath) : 
	m_fileHandle{
		CreateFileW(
			dllPath.data(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		)
	},
	m_fileMapping{
		CreateFileMappingFromApp(
			m_fileHandle.get(),
			nullptr,
			PAGE_READONLY | SEC_IMAGE_NO_EXECUTE,
			0,
			nullptr
		)
	},
	m_baseAddress{
		MapViewOfFileFromApp(
			m_fileMapping.get(),
			FILE_MAP_READ,
			0,
			0
		)
	}
{
}

HRESULT HookHelper::Detours::Begin()
{
	DetourSetIgnoreTooSmall(TRUE);
	RETURN_IF_WIN32_ERROR(DetourTransactionBegin());
	RETURN_IF_WIN32_ERROR(DetourUpdateThread(GetCurrentThread()));
	return S_OK;
}

HRESULT HookHelper::Detours::End(bool commit)
{
	return ( commit ? HRESULT_FROM_WIN32(DetourTransactionCommit()) : HRESULT_FROM_WIN32(DetourTransactionAbort()) );
}

HRESULT HookHelper::Detours::Write(const std::function<void()>&& callback) try
{
	HRESULT hr{ HookHelper::Detours::Begin() };
	if (FAILED(hr))
	{
		return hr;
	}

	callback();

	return HookHelper::Detours::End(true);
}
catch (...)
{
	LOG_CAUGHT_EXCEPTION();
	LOG_IF_FAILED(HookHelper::Detours::End(false));
	return wil::ResultFromCaughtException();
}

void HookHelper::Detours::Attach(std::string_view dllName, std::string_view funcName, PVOID* realFuncAddr, PVOID hookFuncAddr)
{
	THROW_HR_IF_NULL(E_INVALIDARG, realFuncAddr);
	THROW_HR_IF_NULL(E_INVALIDARG, *realFuncAddr);
	*realFuncAddr = DetourFindFunction(dllName.data(), funcName.data());
	THROW_LAST_ERROR_IF_NULL(*realFuncAddr);

	Attach(realFuncAddr, hookFuncAddr);
}

void HookHelper::Detours::Attach(PVOID* realFuncAddr, PVOID hookFuncAddr)
{
	THROW_HR_IF_NULL(E_INVALIDARG, realFuncAddr);
	THROW_HR_IF_NULL(E_INVALIDARG, *realFuncAddr);
	THROW_HR_IF_NULL(E_INVALIDARG, hookFuncAddr);

	THROW_IF_WIN32_ERROR(DetourAttach(realFuncAddr, hookFuncAddr));
}

void HookHelper::Detours::Detach(PVOID* realFuncAddr, PVOID hookFuncAddr)
{
	THROW_HR_IF_NULL(E_INVALIDARG, realFuncAddr);
	THROW_HR_IF_NULL(E_INVALIDARG, *realFuncAddr);
	THROW_HR_IF_NULL(E_INVALIDARG, hookFuncAddr);

	THROW_IF_WIN32_ERROR(DetourDetach(realFuncAddr, hookFuncAddr));
}

size_t HookHelper::HwndRef::Increase(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name, long long delta)
{
	auto propName{ std::format(L"{}.{}.{}.{}", HwndProp::propPrefix, propNamespace, propPrefix, name) };
	size_t refCount{ static_cast<size_t>(reinterpret_cast<long long>(GetPropW(hWnd, propName.c_str())) + delta)};
	SetPropW(hWnd, propName.c_str(), reinterpret_cast<HANDLE>(refCount));
	if (refCount == 0)
	{
		RemovePropW(hWnd, propName.c_str());
	}

	return refCount;
}
void HookHelper::HwndRef::Clear(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name)
{
	auto propName{ std::format(L"{}.{}.{}.{}", HwndProp::propPrefix, propNamespace, propPrefix, name) };
	RemovePropW(hWnd, propName.c_str());
}
void HookHelper::HwndRef::ClearAll(HWND hWnd, std::wstring_view propNamespace)
{
	auto propNameShort{ std::format(L"{}.{}.{}", HwndProp::propPrefix, propNamespace, propPrefix) };
	EnumPropsExW(hWnd, [](HWND hwnd, LPWSTR lpString, HANDLE hData, ULONG_PTR lParam)
	{
		if (HIWORD(lpString) && wcsstr(lpString, (*reinterpret_cast<std::wstring*>(lParam)).c_str()))
		{
			RemovePropW(hwnd, lpString);
		}
		return TRUE;
	}, reinterpret_cast<LPARAM>(&propNameShort));
}

void HookHelper::HwndProp::Set(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name, HANDLE data)
{
	auto propName{ std::format(L"{}.{}.{}", propPrefix, propNamespace, name) };
	SetPropW(hWnd, propName.c_str(), data);
}
void HookHelper::HwndProp::Unset(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name)
{
	auto propName{ std::format(L"{}.{}.{}", propPrefix, propNamespace, name) };
	RemovePropW(hWnd, propName.c_str());
}
void HookHelper::HwndProp::ClearAll(HWND hWnd, std::wstring_view propNamespace)
{
	auto propNameShort{ std::format(L"{}.{}", propPrefix, propNamespace) };
	EnumPropsExW(hWnd, [](HWND hwnd, LPWSTR lpString, HANDLE hData, ULONG_PTR lParam)
	{
		if (HIWORD(lpString) && wcsstr(lpString, (*reinterpret_cast<std::wstring*>(lParam)).c_str()))
		{
			RemovePropW(hwnd, lpString);
		}
		return TRUE;
	}, reinterpret_cast<LPARAM>(&propNameShort));
}

void HookHelper::HwndCallOnce(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name, const std::function<void()>&& callback, HANDLE data)
{
	auto propName{ std::format(L"{}.{}.{}", HwndProp::propPrefix, propNamespace, name) };
	if (!GetPropW(hWnd, propName.c_str()))
	{
		callback();
		if (data != nullptr)
		{
			SetPropW(hWnd, propName.c_str(), reinterpret_cast<HANDLE>(HANDLE_FLAG_INHERIT));
		}
	}
}