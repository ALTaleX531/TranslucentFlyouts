#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace Hooking
	{
		void WriteMemory(PVOID memoryAddress, std::function<void()> callback);

		// Return the number of functions written
		size_t WriteIAT(PVOID baseAddress, std::string_view dllName, std::unordered_map<LPCSTR, PVOID> hookMap);
		size_t WriteDelayloadIAT(PVOID baseAddress, std::string_view dllName, std::unordered_map<LPCSTR, PVOID> hookMap);

		void WalkIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName)> callback);
		void WalkDeleyloadIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(HMODULE* moduleHandle, PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName)> callback);

		namespace Detours
		{
			// Call single or multiple Attach/Detach in the callback
			HRESULT Write(const std::function<void()> callback);
			// Install an inline hook using Detours
			void Attach(const std::string_view dllName, std::string_view funcName, PVOID* realFuncAddr, PVOID hookFuncAddr) noexcept(false);
			void Attach(PVOID* realFuncAddr, PVOID hookFuncAddr) noexcept(false);
			// Uninstall an inline hook using Detours
			void Detach(PVOID* realFuncAddr, PVOID hookFuncAddr) noexcept(false);
		}

		class DllNotifyRoutine
		{
		public:
			static DllNotifyRoutine& GetInstance();
			DllNotifyRoutine();
			~DllNotifyRoutine() noexcept;
			DllNotifyRoutine(const DllNotifyRoutine&) = delete;
			DllNotifyRoutine& operator=(const DllNotifyRoutine&) = delete;

			struct DllInfo
			{
				PCUNICODE_STRING FullDllName;   // The full path name of the DLL module.
				PCUNICODE_STRING BaseDllName;   // The base file name of the DLL module.
				PVOID DllBase;                  // A pointer to the base address for the DLL in memory.
				ULONG SizeOfImage;              // The size of the DLL image, in bytes.
			};
			using Callback = std::function<void(bool load, DllInfo info)>;

			void AddCallback(Callback callback);
			void DeleteCallback(Callback callback);
		private:
			typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
			{
				ULONG Flags;                    // Reserved.
				PCUNICODE_STRING FullDllName;   // The full path name of the DLL module.
				PCUNICODE_STRING BaseDllName;   // The base file name of the DLL module.
				PVOID DllBase;                  // A pointer to the base address for the DLL in memory.
				ULONG SizeOfImage;              // The size of the DLL image, in bytes.
			} LDR_DLL_LOADED_NOTIFICATION_DATA, * PLDR_DLL_LOADED_NOTIFICATION_DATA;
			typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA
			{
				ULONG Flags;                    // Reserved.
				PCUNICODE_STRING FullDllName;   // The full path name of the DLL module.
				PCUNICODE_STRING BaseDllName;   // The base file name of the DLL module.
				PVOID DllBase;                  // A pointer to the base address for the DLL in memory.
				ULONG SizeOfImage;              // The size of the DLL image, in bytes.
			} LDR_DLL_UNLOADED_NOTIFICATION_DATA, * PLDR_DLL_UNLOADED_NOTIFICATION_DATA;
			typedef union _LDR_DLL_NOTIFICATION_DATA
			{
				LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
				LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
			} LDR_DLL_NOTIFICATION_DATA, * PLDR_DLL_NOTIFICATION_DATA;
			typedef LDR_DLL_NOTIFICATION_DATA* PCLDR_DLL_NOTIFICATION_DATA;
			typedef VOID(CALLBACK* PLDR_DLL_NOTIFICATION_FUNCTION)(
				ULONG NotificationReason,
				PCLDR_DLL_NOTIFICATION_DATA NotificationData,
				PVOID Context
			);
			static VOID CALLBACK LdrDllNotification(
				ULONG NotificationReason,
				PCLDR_DLL_NOTIFICATION_DATA NotificationData,
				PVOID Context
			);
			static constexpr UINT LDR_DLL_NOTIFICATION_REASON_LOADED{1};
			static constexpr UINT LDR_DLL_NOTIFICATION_REASON_UNLOADED{2};

			bool m_internalError{false};
			PVOID m_cookie{nullptr};
			NTSTATUS(NTAPI* m_actualLdrRegisterDllNotification)(ULONG, PLDR_DLL_NOTIFICATION_FUNCTION, PVOID, PVOID*);
			NTSTATUS(NTAPI* m_actualLdrUnregisterDllNotification)(PVOID);
			std::vector<Callback> m_callbackList{};
		};

		class MsgHooks
		{
		public:
			static MsgHooks& GetInstance();
			MsgHooks() = default;
			~MsgHooks() noexcept;
			MsgHooks(const MsgHooks&) = delete;
			MsgHooks& operator=(const MsgHooks&) = delete;

			using Callback = std::function<void(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT lResult, bool returnResultValid)>;

			void Install(HWND hWnd);
			void Uninstall(HWND hWnd);

			void AddCallback(Callback callback);
			void DeleteCallback(Callback callback);

			void UninstallAll();
		private:
			static LRESULT CALLBACK CallWndHookProc(int code, WPARAM wParam, LPARAM lParam);
			static LRESULT CALLBACK CallWndRetHookProc(int code, WPARAM wParam, LPARAM lParam);

			struct HookInfo
			{
				struct
				{
					wil::unique_hhook callWindowHook{nullptr};
					wil::unique_hhook callWindowRetHook{nullptr};
				};
				std::vector<HWND> windowList{};
			};
			// threadId, hook info
			std::unordered_map<DWORD, HookInfo> m_hookMap{};
			std::vector<Callback> m_callbackList{};
		};

		class FunctionCallHook
		{
		public:
			FunctionCallHook() = default;
			~FunctionCallHook();

			int Attach(PVOID targetAddress, PVOID detourAddress, PVOID detourDestination, int callersCount);
			void Detach();
		private:
			void InitJumpRegion(PVOID startAddress);
			// Return the index of jump region
			std::byte* AllocJmpCode(PVOID detourDestination);

			DWORD m_size{0};
			wil::unique_virtualalloc_ptr<std::byte> m_jumpRegion{nullptr};
			std::byte* m_current{nullptr}, * m_end{nullptr};
			std::unordered_map<LONG*, LONG> m_hookedMap{};
		};

		PVOID SearchData(PVOID address, BYTE opCode[], size_t len);
	}
}