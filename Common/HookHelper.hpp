#pragma once
#include "framework.h"
#include "cpprt.h"
#include "Utils.hpp"

namespace TranslucentFlyouts
{
	namespace HookHelper
	{
		class ThreadSnapshotExcludeSelf
		{
		public:
			ThreadSnapshotExcludeSelf();
			~ThreadSnapshotExcludeSelf();

			void Suspend();
			void Resume();
			DWORD GetMainThreadId();
			void ForEach(const std::function<bool(const PSS_THREAD_ENTRY&)>&& callback);
		private:
			HPSS m_snapshot{nullptr};
		};

		struct DllInfo
		{
			PCUNICODE_STRING FullDllName;   // The full path name of the DLL module.
			PCUNICODE_STRING BaseDllName;   // The base file name of the DLL module.
			PVOID DllBase;                  // A pointer to the base address for the DLL in memory.
			ULONG SizeOfImage;              // The size of the DLL image, in bytes.
		};
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
		constexpr UINT LDR_DLL_NOTIFICATION_REASON_LOADED{ 1 };
		constexpr UINT LDR_DLL_NOTIFICATION_REASON_UNLOADED{ 2 };

		inline const auto g_actualLdrRegisterDllNotification{ reinterpret_cast<NTSTATUS(NTAPI*)(ULONG, HookHelper::PLDR_DLL_NOTIFICATION_FUNCTION, PVOID, PVOID*)>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrRegisterDllNotification")) };
		inline const auto g_actualLdrUnregisterDllNotification{ reinterpret_cast<NTSTATUS(NTAPI*)(PVOID)>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrUnregisterDllNotification")) };

		void WriteMemory(PVOID memoryAddress, const std::function<void()>&& callback);
		PVOID InjectCallbackToThread(DWORD threadId, const std::function<void()>& callback);
		HMODULE GetProcessModule(HANDLE processHandle, std::wstring_view dllPath);

		void WalkIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName)> callback);
		void WalkDelayloadIAT(PVOID baseAddress, std::string_view dllName, std::function<bool(HMODULE* moduleHandle, PVOID* functionAddress, LPCSTR functionNameOrOrdinal, BOOL importedByName)> callback);
		
		PVOID* GetIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal);
		std::pair<HMODULE*, PVOID*> GetDelayloadIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, bool resolveAPI = false);
		PVOID WriteIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, PVOID detourFunction);
		std::pair<HMODULE, PVOID> WriteDelayloadIAT(PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal, PVOID detourFunction, std::optional<HMODULE> newModuleHandle = std::nullopt);
		void ResolveDelayloadIAT(const std::pair<HMODULE*, PVOID*>& info, PVOID baseAddress, std::string_view dllName, LPCSTR targetFunctionNameOrOrdinal);

		PUCHAR MatchAsmCode(PUCHAR startAddress, std::pair<UCHAR*, size_t> asmCodeInfo, std::vector<std::pair<size_t, size_t>> asmSliceInfo, size_t maxSearchBytes);
		template <typename T>
		[[maybe_unused]] __forceinline size_t GetVTableIndex(T member_function)
		{
			size_t InvokeAccessViolation(void(*vtable_function)(void*));
			return InvokeAccessViolation(Utils::union_cast<void(*)(void*)>(member_function));
		}
		__forceinline void** GetObjectVTable(void* This)
		{
			return reinterpret_cast<void**>(*reinterpret_cast<void**>(This));
		}
		std::pair<UCHAR*, size_t> GetTextSectionInfo(PVOID baseAddress);

		template <typename T>
		struct MemberFunction;

		template <typename ReturnValue, typename Interface, typename... Args>
		struct MemberFunction<ReturnValue(Interface::*)(Args...)>
		{
			using type = ReturnValue(Interface* This, Args... args);
		};

		class ImageMapper
		{
			wil::unique_hfile m_fileHandle{ nullptr };
			wil::unique_handle m_fileMapping{ nullptr };
			wil::unique_mapview_ptr<void> m_baseAddress{nullptr};
		public:
			ImageMapper(std::wstring_view dllPath);
			~ImageMapper() noexcept = default;
			ImageMapper(const ImageMapper&) = delete;
			ImageMapper& operator=(const ImageMapper&) = delete;

			template <typename T = HMODULE>
			__forceinline T GetBaseAddress() const { return reinterpret_cast<T>(m_baseAddress.get()); };
		};

		struct OffsetStorage
		{
			LONGLONG value{0};

			inline bool IsValid() const { return value != 0; }
			template <typename T=PVOID, typename T2=PVOID>
			inline T To(T2 baseAddress) const { if (baseAddress == 0 || !IsValid()) { return 0; } return reinterpret_cast<T>(RVA_TO_ADDR(baseAddress, value)); }
			template <typename T=PVOID>
			static inline auto From(T baseAddress, T targetAddress) { return OffsetStorage{ LONGLONG(targetAddress) - LONGLONG(baseAddress) }; }
			static inline auto From(PVOID baseAddress, PVOID targetAddress) { if (!baseAddress || !targetAddress) { return OffsetStorage{0}; } return OffsetStorage{ reinterpret_cast<LONGLONG>(targetAddress) - reinterpret_cast<LONGLONG>(baseAddress) }; }
		};

		namespace Detours
		{
			// Call single or multiple Attach/Detach in the callback
			HRESULT Write(const std::function<void()>&& callback);
			// Install an inline hook using Detours
			void Attach(std::string_view dllName, std::string_view funcName, PVOID* realFuncAddr, PVOID hookFuncAddr) noexcept(false);
			void Attach(PVOID* realFuncAddr, PVOID hookFuncAddr) noexcept(false);
			// Uninstall an inline hook using Detours
			void Detach(PVOID* realFuncAddr, PVOID hookFuncAddr) noexcept(false);
		}

		static inline UINT TFM_REMOVESUBCLASS{RegisterWindowMessageW(L"TranslucentFlyouts.RemoveWindowSublcass")};
		static inline UINT TFM_ATTACH{ RegisterWindowMessageW(L"TranslucentFlyouts.Attach") };
		static inline UINT TFM_DETACH{ RegisterWindowMessageW(L"TranslucentFlyouts.Detach") };
		inline constexpr std::wstring_view subclassPropPrefix{ L"TranslucentFlyouts.Token" };

		namespace HwndProp
		{
			inline constexpr std::wstring_view propPrefix{ L"TranslucentFlyouts.HwndProp" };
			void Set(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name, HANDLE data = reinterpret_cast<HANDLE>(HANDLE_FLAG_INHERIT));
			void Unset(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name);
			template <typename T=HANDLE>
			T Get(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name)
			{
				SetLastError(ERROR_SUCCESS);
				auto propName{ std::format(L"{}.{}.{}", propPrefix, propNamespace, name) };
				auto result{ GetPropW(hWnd, propName.c_str()) };
				return reinterpret_cast<T>(result);
			}
			void ClearAll(HWND hWnd, std::wstring_view propNamespace);
		}

		namespace HwndRef
		{
			inline constexpr std::wstring_view propPrefix{ L"HwndRef" };
			size_t Increase(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name, long long delta);
			void Clear(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name);
			void ClearAll(HWND hWnd, std::wstring_view propNamespace);
		}

		void HwndCallOnce(HWND hWnd, std::wstring_view propNamespace, std::wstring_view name, const std::function<void()>&& callback, HANDLE data = reinterpret_cast<HANDLE>(HANDLE_FLAG_INHERIT));

		namespace Subclass
		{
			using namespace std::literals;

			template <SUBCLASSPROC subclassProc>
			std::wstring GetNamespace()
			{
				static const auto result{ std::format(L"{}", reinterpret_cast<void*>(subclassProc)) };
				return result;
			}

			template <SUBCLASSPROC subclassProc>
			struct Storage;

			template <SUBCLASSPROC subclassProc>
			bool IsAlreadyAttached(HWND hwnd)
			{
				DWORD_PTR refData{0};
				return static_cast<bool>(GetWindowSubclass(hwnd, Storage<subclassProc>::Wrapper, 0, &refData));
			}
			template <SUBCLASSPROC subclassProc>
			void Attach(HWND hwnd, bool attach, bool windowDestroyed = false)
			{
				auto& windowList{ Storage<subclassProc>::s_windowList };
				auto threadId = GetWindowThreadProcessId(hwnd, nullptr);

				if (attach)
				{
					auto worker = [&]
					{
						if (SetWindowSubclass(hwnd, Storage<subclassProc>::Wrapper, 0, 0))
						{
							windowList.push_back(hwnd);
							Storage<subclassProc>::Wrapper(hwnd, TFM_ATTACH, 0, 0, 0, 0);
						}
					};

					if (threadId == GetCurrentThreadId())
					{
						worker();
					}
					// SetWindowSubclass cannot work for the window which is not in the same thread!
					// So here we have to use a tricky way to achieve the goal...
					else
					{
						InjectCallbackToThread(threadId, worker);
					}
				}
				else
				{
					if (threadId == GetCurrentThreadId())
					{
						Storage<subclassProc>::Wrapper(hwnd, TFM_DETACH, windowDestroyed, 0, 0, 0);
						if (RemoveWindowSubclass(hwnd, Storage<subclassProc>::Wrapper, 0))
						{
							windowList.erase(std::remove(windowList.begin(), windowList.end(), hwnd), windowList.end());
							HwndProp::ClearAll(hwnd, GetNamespace<subclassProc>());
						}
					}
					else
					{
						SendMessageW(hwnd, TFM_REMOVESUBCLASS, 0, 0);
					}
				}
			}
			template <SUBCLASSPROC subclassProc>
			void DetachAll()
			{
				auto windowList{ Storage<subclassProc>::s_windowList };
				for (auto hwnd : windowList) { Attach<subclassProc>(hwnd, false); }
			}

			template <SUBCLASSPROC subclassProc>
			struct Storage
			{
				static inline std::vector<HWND> s_windowList{};
				static LRESULT CALLBACK Wrapper(
					HWND hWnd,
					UINT uMsg,
					WPARAM wParam,
					LPARAM lParam,
					UINT_PTR uIdSubclass,
					DWORD_PTR dwRefData
				)
				{
					if (uMsg == TFM_REMOVESUBCLASS)
					{
						Attach<subclassProc>(hWnd, false);
						return 0;
					}
					if (uMsg == WM_DESTROY)
					{
						Attach<subclassProc>(hWnd, false, true);
					}
					return subclassProc(hWnd, uMsg, wParam, lParam, uIdSubclass, dwRefData);
				}
			};
		}

		namespace ForceSubclass
		{
			inline constexpr std::wstring_view propName{ L"ForceSubclass.OriginalWndProc" };

			template <WNDPROC subclassProc>
			std::wstring GetNamespace()
			{
				static const auto result{ std::format(L"{}", reinterpret_cast<void*>(subclassProc)) };
				return result;
			}

			template <WNDPROC subclassProc>
			struct Storage;

			template <WNDPROC subclassProc>
			void Attach(HWND hwnd, bool attach, bool windowDestroyed = false)
			{
				auto& windowList{ Storage<subclassProc>::s_windowList };

				auto refCount{ HwndRef::Increase(hwnd, GetNamespace<subclassProc>(), L"ForceSubclass", attach ? 1ll : -1ll)};
				if (attach && refCount == 1)
				{
					auto value
					{
						reinterpret_cast<HANDLE>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Storage<subclassProc>::Wrapper)))
					};
					HwndProp::Set(hwnd, GetNamespace<subclassProc>(), propName, value);
					if (value)
					{
						windowList.push_back(hwnd);
						Storage<subclassProc>::Wrapper(hwnd, TFM_ATTACH, windowDestroyed, 0);
					}
				}
				if (!attach && refCount == 0)
				{
					Storage<subclassProc>::Wrapper(hwnd, TFM_DETACH, windowDestroyed, 0);
					if (SetWindowLongPtrW(hwnd, GWLP_WNDPROC, HwndProp::Get<LONG_PTR>(hwnd, GetNamespace<subclassProc>(), propName)))
					{
						windowList.erase(std::remove(windowList.begin(), windowList.end(), hwnd), windowList.end());
						HookHelper::HwndProp::Unset(hwnd, ForceSubclass::GetNamespace<subclassProc>(), propName);
						HwndProp::ClearAll(hwnd, GetNamespace<subclassProc>());
					}
				}
			}

			template <WNDPROC subclassProc>
			void DetachAll()
			{
				auto windowList{ Storage<subclassProc>::s_windowList };
				for (auto hwnd : windowList) { Attach<subclassProc>(hwnd, false); }
			}

			template <WNDPROC subclassProc>
			struct Storage
			{
				static inline std::vector<HWND> s_windowList{};

				static LRESULT CALLBACK Wrapper(
					HWND hWnd,
					UINT uMsg,
					WPARAM wParam,
					LPARAM lParam
				)
				{
					if (uMsg == WM_DESTROY)
					{
						Attach<subclassProc>(hWnd, false, true);
					}
					return subclassProc(hWnd, uMsg, wParam, lParam);
				}
				static LRESULT CALLBACK CallOriginalWndProc(
					HWND hWnd,
					UINT uMsg,
					WPARAM wParam,
					LPARAM lParam
				)
				{
					const auto originalWndProc{ HwndProp::Get<decltype(&CallOriginalWndProc)>(hWnd, GetNamespace<subclassProc>(), propName)};
					return CallWindowProcW(originalWndProc, hWnd, uMsg, wParam, lParam);
				}
			};
		}
	}
}