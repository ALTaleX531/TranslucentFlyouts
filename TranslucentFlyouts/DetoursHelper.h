#pragma once
#include "pch.h"
#include "..\Detours\detours.h"
#ifdef _WIN64
	#pragma comment(lib, "..\\Libraries\\x64\\detours.lib")
#else
	#pragma comment(lib, "..\\Libraries\\x86\\detours.lib")
#endif // _WIN64

namespace TranslucentFlyoutsLib
{
	class Detours
	{
	public:
		template <typename T, typename... Args>
		auto OldFunction(Args&&... args)
		{
			return reinterpret_cast<T*>(GetOldFunction())(args...);
		}

		template <typename... Args>
		static void Batch(BOOL bHookState, Args&&... args)
		{
			Batch(bHookState, {args...});
		}
		static void Batch(BOOL bHookState, const std::initializer_list<Detours>& args)
		{
			for (auto item : args)
			{
				item.SetHookState(TRUE);
			}
		}

		static void Begin()
		{
			DetourSetIgnoreTooSmall(TRUE);
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
		}
		static void Commit()
		{
			DetourTransactionCommit();
		}

		Detours(LPCSTR pszModule, LPCSTR pszFunction, PVOID NewAddr = nullptr)
		{
			PVOID OldAddr = DetourFindFunction(pszModule, pszFunction);
			pvOldAddr = OldAddr;
			pvNewAddr = NewAddr;
		}
		Detours(PVOID OldAddr, PVOID NewAddr = nullptr)
		{
			pvOldAddr = OldAddr;
			pvNewAddr = NewAddr;
		}

		PVOID GetOldFunction() const
		{
			return pvOldAddr;
		}
		void SetHookState(BOOL bHookState)
		{
			if (bHookState == TRUE)
			{
				DetourAttach(&(PVOID&)pvOldAddr, pvNewAddr);
				bHookInstalled = true;
			}
			else
			{
				DetourDetach(&(PVOID&)pvOldAddr, pvNewAddr);
				bHookInstalled = false;
			}
		}
		bool IsHookInstalled()
		{
			return bHookInstalled;
		}
	private:
		PVOID pvNewAddr;
		PVOID pvOldAddr;
		bool bHookInstalled;
	};
	using DetoursHook = Detours;
}