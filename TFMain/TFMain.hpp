#pragma once
#include "pch.h"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "MenuHandler.hpp"
#include "UxThemePatcher.hpp"
#include "ImmersiveContextMenuPatcher.hpp"

namespace TranslucentFlyouts
{
	class MainDLL
	{
	public:
		static MainDLL& GetInstance();
		~MainDLL() noexcept = default;
		MainDLL(const MainDLL&) = delete;
		MainDLL& operator=(const MainDLL&) = delete;

		static inline bool IsHookGlobalInstalled()
		{
			return g_hHook != nullptr;
		}
		static HRESULT InstallHook();
		static HRESULT UninstallHook();
		static bool IsCurrentProcessInBlockList();
		void Startup();
		void Shutdown();

		using Callback = std::function<void(HWND, DWORD)>;

		void AddCallback(Callback callback);
		void DeleteCallback(Callback callback);
	private:
		MainDLL();
		static void CALLBACK HandleWinEvent(
			HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
			LONG idObject, LONG idChild,
			DWORD dwEventThread, DWORD dwmsEventTime
		);
		static HWINEVENTHOOK g_hHook;

		bool m_startup{false};
		std::vector<Callback> m_callbackList{};

		MenuHandler& m_menuHandler{MenuHandler::GetInstance()};
		UxThemePatcher& m_uxthemePatcher{UxThemePatcher::GetInstance()};
		ImmersiveContextMenuPatcher& m_immersiveContextMenuPatcher{ImmersiveContextMenuPatcher::GetInstance()};
	};
}