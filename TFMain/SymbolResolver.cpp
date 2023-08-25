#include "pch.h"
#include "Utils.hpp"
#include "resource.h"
#include "SymbolResolver.hpp"

using namespace std;
using namespace wil;
using namespace TranslucentFlyouts;

BOOL CALLBACK SymbolResolver::SymCallback(
	HANDLE hProcess,
	ULONG ActionCode,
	ULONG64 CallbackData,
	ULONG64 UserContext
)
{
	if (ActionCode == CBA_EVENT)
	{
		if (UserContext)
		{
			using TranslucentFlyouts::TFMain::InteractiveIO;
			auto& symbolResolver{*reinterpret_cast<SymbolResolver*>(UserContext)};
			auto event{reinterpret_cast<PIMAGEHLP_CBA_EVENTW>(CallbackData)};

			if (wcsstr(event->desc, L"from http://"))
			{
				symbolResolver.m_printInfo = true;
			}
			if (wcsstr(event->desc, L"SYMSRV:  HTTPGET: /download/symbols/index2.txt"))
			{
				symbolResolver.m_io.OutputString(
					InteractiveIO::StringType::Notification,
					InteractiveIO::WaitType::NoWait,
					IDS_STRING106,
					std::format(L"[{}] ", symbolResolver.m_sessionName),
					L"\n"sv
				);
			}
			if (symbolResolver.m_printInfo)
			{
				symbolResolver.m_io.OutputString(
					InteractiveIO::StringType::Notification,
					InteractiveIO::WaitType::NoWait,
					0,
					L""sv,
					std::format(L"{}", event->desc)
				);
			}
			if (wcsstr(event->desc, L"copied"))
			{
				symbolResolver.m_printInfo = false;
			}
		}

		return TRUE;
	}
	return FALSE;
}

SymbolResolver::SymbolResolver(std::wstring_view sessionName, const TFMain::InteractiveIO& io) : m_sessionName{ sessionName }, m_io{io}
{
	try
	{
		THROW_IF_WIN32_BOOL_FALSE(SymInitialize(GetCurrentProcess(), nullptr, FALSE));

		SymSetOptions(SYMOPT_DEFERRED_LOADS);
		THROW_IF_WIN32_BOOL_FALSE(SymRegisterCallbackW64(GetCurrentProcess(), SymCallback, reinterpret_cast<ULONG64>(this)));

		WCHAR curDir[MAX_PATH + 1]{};
		THROW_LAST_ERROR_IF(GetModuleFileName(HINST_THISCOMPONENT, curDir, MAX_PATH) == 0);
		THROW_IF_FAILED(PathCchRemoveFileSpec(curDir, MAX_PATH));

		wstring symPath{format(L"SRV*{}\\symbols", curDir)};
		THROW_IF_WIN32_BOOL_FALSE(SymSetSearchPathW(GetCurrentProcess(), symPath.c_str()));
	}
	catch (...)
	{
		SymbolResolver::~SymbolResolver();
		throw;
	}
}

SymbolResolver::~SymbolResolver() noexcept
{
	SymCleanup(GetCurrentProcess());
}

HRESULT SymbolResolver::Walk(std::wstring_view dllName, string_view mask, function<bool(PSYMBOL_INFO, ULONG)> callback) try
{
	DWORD64 dllBase{0};
	WCHAR filePath[MAX_PATH + 1]{}, symFile[MAX_PATH + 1]{};
	MODULEINFO modInfo{};

	auto cleanUp = scope_exit([&]
	{
		if (dllBase != 0)
		{
			SymUnloadModule64(GetCurrentProcess(), dllBase);
			dllBase = 0;
		}
	});

	THROW_HR_IF(E_INVALIDARG, dllName.empty());

	unique_hmodule moduleHandle{LoadLibraryExW(dllName.data(), nullptr, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_SEARCH_SYSTEM32)};
	THROW_LAST_ERROR_IF_NULL(moduleHandle);
	THROW_LAST_ERROR_IF(GetModuleFileNameW(moduleHandle.get(), filePath, MAX_PATH) == 0);
	THROW_IF_WIN32_BOOL_FALSE(GetModuleInformation(GetCurrentProcess(), moduleHandle.get(), &modInfo, sizeof(modInfo)));
	
	if (SymGetSymbolFileW(GetCurrentProcess(), nullptr, filePath, sfPdb, symFile, MAX_PATH, symFile, MAX_PATH) == FALSE)
	{
		m_requireInternet = true;
		DWORD lastError{GetLastError()};
		THROW_WIN32_IF(lastError, lastError != ERROR_FILE_NOT_FOUND);

		WCHAR curDir[MAX_PATH + 1]{};
		THROW_LAST_ERROR_IF(GetModuleFileName(HINST_THISCOMPONENT, curDir, MAX_PATH) == 0);
		THROW_IF_FAILED(PathCchRemoveFileSpec(curDir, MAX_PATH));

		wstring symPath{format(L"SRV*{}\\symbols*http://msdl.microsoft.com/download/symbols", curDir)};
		
		DWORD options = SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG);
		
		auto cleanUp = scope_exit([&]
		{
			SymSetOptions(options);
		});

		THROW_IF_WIN32_BOOL_FALSE(SymGetSymbolFileW(GetCurrentProcess(), symPath.c_str(), filePath, sfPdb, symFile, MAX_PATH, symFile, MAX_PATH));
		
	}
	m_symbolsOK = true;

	dllBase = SymLoadModuleExW(GetCurrentProcess(), nullptr, filePath, nullptr, reinterpret_cast<DWORD64>(modInfo.lpBaseOfDll), modInfo.SizeOfImage, nullptr, 0);
	THROW_LAST_ERROR_IF(dllBase == 0);
	THROW_IF_WIN32_BOOL_FALSE(SymEnumSymbols(GetCurrentProcess(), dllBase, mask.empty() ? nullptr : mask.data(), SymbolResolver::EnumSymbolsCallback, &callback));
	
	return S_OK;
}
CATCH_LOG_RETURN_HR(ResultFromCaughtException())

bool SymbolResolver::GetSymbolStatus()
{
	return m_symbolsOK;
}

bool SymbolResolver::GetSymbolSource()
{
	return m_requireInternet;
}

BOOL SymbolResolver::EnumSymbolsCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
	auto& callback{*reinterpret_cast<function<bool(PSYMBOL_INFO symInfo, ULONG symbolSize)>*>(UserContext)};

	if (callback)
	{
		return static_cast<BOOL>(callback(pSymInfo, SymbolSize));
	}

	return TRUE;
}