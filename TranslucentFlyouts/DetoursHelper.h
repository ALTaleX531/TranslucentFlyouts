#pragma once
#include "pch.h"
#include "..\Detours\detours.h"
#ifdef _WIN64
	#pragma comment(lib, "..\\Libraries\\x64\\detours.lib")
#else
	#pragma comment(lib, "..\\Libraries\\x86\\detours.lib")
#endif // _WIN64

#define CallOldFunction(detours, caller, ...) ((decltype(caller)*)(detours.GetOldFunction()))(__VA_ARGS__)

class Detours
{
public:
	static void BeginHook();
	static void EndHook();

	Detours(LPCSTR pszModule, LPCSTR pszFunction, PVOID NewAddr = nullptr);
	Detours(PVOID OldAddr, PVOID NewAddr = nullptr);

	PVOID GetOldFunction() const;
	void SetHookState(BOOL bHookState);
private:
	PVOID pvNewAddr;
	PVOID pvOldAddr;
};