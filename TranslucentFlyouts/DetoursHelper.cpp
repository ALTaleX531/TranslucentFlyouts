#include "pch.h"
#include "DetoursHelper.h"

void Detours::BeginHook()
{
	DetourSetIgnoreTooSmall(TRUE);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
}

void Detours::EndHook()
{
	DetourTransactionCommit();
}

Detours::Detours(LPCSTR pszModule, LPCSTR pszFunction, PVOID NewAddr)
{
	PVOID OldAddr = DetourFindFunction(pszModule, pszFunction);
	pvOldAddr = OldAddr;
	pvNewAddr = NewAddr;
}
Detours::Detours(PVOID OldAddr, PVOID NewAddr)
{
	pvOldAddr = OldAddr;
	pvNewAddr = NewAddr;
}

PVOID Detours::GetOldFunction() const
{
	return pvOldAddr;
}

void Detours::SetHookState(BOOL bHookState)
{
	if (bHookState)
	{
		DetourAttach((PVOID*)&pvOldAddr, pvNewAddr);
	}
	else
	{
		DetourDetach((PVOID*)&pvOldAddr, pvNewAddr);
	}
}

