// windbg-workaround.cpp : 定义 DLL 的导出函数。
//

#include "pch.h"
#pragma warning(disable : 6101)
#pragma warning(disable : 6054)
#pragma warning(disable : 6387)

extern "C" BOOL IMAGEAPI SymInitialize(
	_In_ HANDLE hProcess,
	_In_opt_ PCSTR UserSearchPath,
	_In_ BOOL fInvadeProcess
)
{
	return TRUE;
}
extern "C" DWORD IMAGEAPI SymSetOptions(
	_In_ DWORD   SymOptions
)
{
	return 0;
}
extern "C" BOOL IMAGEAPI SymRegisterCallbackW64(
	_In_ HANDLE hProcess,
	_In_ PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
	_In_ ULONG64 UserContext
)
{
	return TRUE;
}
extern "C" BOOL IMAGEAPI SymSetSearchPathW(
	_In_ HANDLE hProcess,
	_In_opt_ PCWSTR SearchPath
)
{
	return TRUE;
}
extern "C" BOOL IMAGEAPI SymCleanup(
	_In_ HANDLE hProcess
)
{
	return TRUE;
}
extern "C" BOOL IMAGEAPI SymUnloadModule64(
	_In_ HANDLE hProcess,
	_In_ DWORD64 BaseOfDll
)
{
	return TRUE;
}
extern "C" BOOL IMAGEAPI SymGetSymbolFileW(
	_In_opt_ HANDLE hProcess,
	_In_opt_ PCWSTR SymPath,
	_In_ PCWSTR ImageFile,
	_In_ DWORD Type,
	_Out_writes_(cSymbolFile) PWSTR SymbolFile,
	_In_ size_t cSymbolFile,
	_Out_writes_(cDbgFile) PWSTR DbgFile,
	_In_ size_t cDbgFile
)
{
	return TRUE;
}
extern "C" DWORD64 IMAGEAPI SymLoadModuleExW(
	_In_ HANDLE hProcess,
	_In_opt_ HANDLE hFile,
	_In_opt_ PCWSTR ImageName,
	_In_opt_ PCWSTR ModuleName,
	_In_ DWORD64 BaseOfDll,
	_In_ DWORD DllSize,
	_In_opt_ PMODLOAD_DATA Data,
	_In_opt_ DWORD Flags
)
{
	return TRUE;
}
extern "C" BOOL IMAGEAPI SymEnumSymbols(
	_In_ HANDLE hProcess,
	_In_ ULONG64 BaseOfDll,
	_In_opt_ PCSTR Mask,
	_In_ PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
	_In_opt_ PVOID UserContext
)
{
	return TRUE;
}
extern "C" PVOID IMAGEAPI ImageDirectoryEntryToData(
	_In_ PVOID Base,
	_In_ BOOLEAN MappedAsImage,
	_In_ USHORT DirectoryEntry,
	_Out_ PULONG Size
)
{
	return nullptr;
}
extern "C" BOOL WINAPI MiniDumpWriteDump(
	_In_ HANDLE hProcess,
	_In_ DWORD ProcessId,
	_In_ HANDLE hFile,
	_In_ MINIDUMP_TYPE DumpType,
	_In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	_In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	_In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
)
{
	return TRUE;
}
extern "C" DWORD IMAGEAPI UnDecorateSymbolName(
	_In_ PCSTR name,
	_Out_writes_(maxStringLength) PSTR outputString,
	_In_ DWORD maxStringLength,
	_In_ DWORD flags
)
{
	return 0;
}