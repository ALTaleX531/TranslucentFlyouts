#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace Utils
	{
		struct external_dc
		{
			HDC dc{nullptr};
			external_dc(HDC hdc = nullptr) : dc{hdc} { if (hdc) { SaveDC(hdc); } }
			WI_NODISCARD inline operator HDC() const WI_NOEXCEPT { return dc; }
			static inline void close(external_dc pdc) WI_NOEXCEPT { if (pdc.dc) { ::RestoreDC(pdc.dc, -1); } }
		};
		typedef wil::unique_any<HDC, decltype(&external_dc::close), external_dc::close, wil::details::pointer_access_noaddress, external_dc> unique_ext_hdc;

		template <typename T, typename Type>
		T member_function_pointer_cast(Type pointer)
		{
			union
			{
				Type real_ptr;
				T fake_ptr;
			} rf{.real_ptr{pointer}};

			return rf.fake_ptr;
		}

		static inline std::optional<wil::unique_rouninitialize_call> RoInit()
		{
			HRESULT hr{RoInitialize(RO_INIT_MULTITHREADED)};

			if (SUCCEEDED(hr) || hr == S_FALSE || hr == RPC_E_CHANGED_MODE)
			{
				return wil::unique_rouninitialize_call{};
			}

			return std::nullopt;
		}

		static inline bool IsBadReadPtr(const void* ptr)
		{
			bool result{false};
			MEMORY_BASIC_INFORMATION mbi{};
			if (::VirtualQuery(ptr, &mbi, sizeof(mbi)))
			{
				DWORD mask{PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY};
				result = !(mbi.Protect & mask);

				// Check the page is not a guard page
				if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
				{
					result = true;
				}
			}
			else
			{
				result = true;
			}

			return result;
		}

		static inline BOOL IsTopLevelWindow(HWND hWnd)
		{
			static const auto pfnIsTopLevelWindow = (BOOL(WINAPI*)(HWND))GetProcAddress(GetModuleHandle(TEXT("User32")), "IsTopLevelWindow");

			if (pfnIsTopLevelWindow)
			{
				return pfnIsTopLevelWindow(hWnd);
			}
			else
			{
				LOG_HR_MSG(E_POINTER, "pfnIsTopLevelWindow is invalid!");
			}

			return FALSE;
		}

		static inline bool IsWindowClass(HWND hWnd, std::wstring_view className)
		{
			WCHAR pszClass[MAX_PATH + 1] {};
			GetClassNameW(hWnd, pszClass, MAX_PATH);

			return !_wcsicmp(pszClass, className.data());
		}

		static bool IsWin32PopupMenu(HWND hWnd)
		{
			WCHAR pszClass[MAX_PATH + 1] {};
			GetClassNameW(hWnd, pszClass, MAX_PATH);

			if (GetClassLongPtr(hWnd, GCW_ATOM) == 32768)
			{
				return true;
			}

			return false;
		}

		static inline PVOID GetModuleBase(HMODULE moduleHandle) try
		{
			MODULEINFO modInfo{};

			THROW_HR_IF_NULL(E_INVALIDARG, moduleHandle);
			THROW_IF_WIN32_BOOL_FALSE(GetModuleInformation(GetCurrentProcess(), moduleHandle, &modInfo, sizeof(modInfo)));

			return modInfo.lpBaseOfDll;
		}
		catch (...)
		{
			LOG_CAUGHT_EXCEPTION();
			return nullptr;
		}

		static inline HRESULT GetModuleFolder(HMODULE moduleHandle, LPWSTR filePath, DWORD size) try
		{
			THROW_HR_IF_NULL(E_INVALIDARG, moduleHandle);
			THROW_LAST_ERROR_IF(GetModuleFileNameW(moduleHandle, filePath, size) == 0);
			THROW_IF_FAILED(PathCchRemoveFileSpec(filePath, size));

			return S_OK;
		}
		CATCH_LOG_RETURN_HR(wil::ResultFromCaughtException())

		static inline HWND GetCurrentMenuOwner() try
		{
			GUITHREADINFO guiThreadInfo{sizeof(GUITHREADINFO)};

			THROW_IF_WIN32_BOOL_FALSE(GetGUIThreadInfo(GetCurrentThreadId(), &guiThreadInfo));

			return guiThreadInfo.hwndMenuOwner;
		}
		catch (...)
		{
			LOG_CAUGHT_EXCEPTION();
			return nullptr;
		}

		[[maybe_unused]] static void EnumProcessThreads(std::function<void(DWORD threadId)> callback) try
		{
			wil::unique_handle snapShot{nullptr};
			THROW_HR_IF_NULL(E_INVALIDARG, callback);

			snapShot.reset(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0));
			THROW_LAST_ERROR_IF(snapShot.get() == INVALID_HANDLE_VALUE);

			THREADENTRY32 entry{sizeof(THREADENTRY32)};
			THROW_LAST_ERROR_IF(Thread32First(snapShot.get(), &entry) == FALSE);

			if (entry.th32OwnerProcessID == GetCurrentProcessId())
			{
				callback(entry.th32ThreadID);
			}
			while (Thread32Next(snapShot.get(), &entry))
			{
				if (entry.th32OwnerProcessID == GetCurrentProcessId())
				{
					callback(entry.th32ThreadID);
				}
			}

			DWORD lastError{GetLastError()};
			if (lastError != ERROR_NO_MORE_FILES)
			{
				THROW_IF_WIN32_ERROR(lastError);
			}
		}
		CATCH_LOG_RETURN()

		static inline std::byte PremultiplyColor(std::byte color, std::byte alpha = std::byte{255})
		{
			return std::byte{static_cast<BYTE>(std::to_integer<int>(color) * (std::to_integer<int>(alpha) + 1) >> 8)};
		}

		static HBITMAP CreateDIB(
			LONG width = 1,
			LONG height = -1,
			std::byte** bits = nullptr
		)
		{
			BITMAPINFO bitmapInfo{{sizeof(bitmapInfo.bmiHeader), width, height, 1, 32, BI_RGB}};

			return CreateDIBSection(nullptr, &bitmapInfo, DIB_RGB_COLORS, reinterpret_cast<PVOID*>(bits), nullptr, 0);
		}

		static inline HRESULT GetBrushColor(HBRUSH brush, COLORREF& color) try
		{
			LOGBRUSH logBrush{};

			THROW_HR_IF_NULL(E_INVALIDARG, brush);
			THROW_LAST_ERROR_IF(!GetObject(brush, sizeof(LOGBRUSH), &logBrush));
			THROW_HR_IF(E_INVALIDARG, logBrush.lbStyle != BS_SOLID);
			color = logBrush.lbColor;

			return S_OK;
		}
		CATCH_LOG_RETURN_HR(wil::ResultFromCaughtException())

		static HBRUSH CreateSolidColorBrushWithAlpha(COLORREF color, std::byte alpha)
		{
			wil::unique_hbitmap bitmap{nullptr};
			std::byte* bits{nullptr};

			bitmap.reset(CreateDIB(1, -1, &bits));
			if (!bitmap)
			{
				return nullptr;
			}

			bits[0] = PremultiplyColor(std::byte{GetBValue(color)}, alpha);
			bits[1] = PremultiplyColor(std::byte{GetGValue(color)}, alpha);
			bits[2] = PremultiplyColor(std::byte{GetRValue(color)}, alpha);
			bits[3] = alpha;

			return CreatePatternBrush(bitmap.get());
		}

		static HBRUSH CreateSolidColorBrushWithAlpha(HBRUSH brush, std::byte alpha) try
		{
			COLORREF color{0};

			THROW_HR_IF_NULL(E_INVALIDARG, brush);
			THROW_IF_FAILED(GetBrushColor(brush, color));

			return CreateSolidColorBrushWithAlpha(color, alpha);
		}
		catch (...)
		{
			LOG_CAUGHT_EXCEPTION();
			return nullptr;
		}

		// Adjust alpha channel of the bitmap, return E_NOTIMPL if the bitmap is unsupported
		static HRESULT PrepareAlpha(HBITMAP bitmap) try
		{
			THROW_HR_IF_NULL(E_INVALIDARG, bitmap);

			BITMAPINFO bitmapInfo{sizeof(bitmapInfo.bmiHeader)};
			auto hdc{wil::GetDC(nullptr)};
			THROW_LAST_ERROR_IF_NULL(hdc);

			THROW_HR_IF(E_INVALIDARG, GetObjectType(bitmap) != OBJ_BITMAP);
			THROW_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) == 0);

			bitmapInfo.bmiHeader.biCompression = BI_RGB;
			auto pixelBits{std::make_unique<std::byte[]>(bitmapInfo.bmiHeader.biSizeImage)};

			THROW_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

			// Nothing we can do to a bitmap whose bit count isn't 32 :(
			THROW_HR_IF_MSG(E_NOTIMPL, bitmapInfo.bmiHeader.biBitCount != 32, "Unsuported bit count: %d!", bitmapInfo.bmiHeader.biBitCount);

			bool bHasAlpha{false};
			for (size_t i = 0; i < bitmapInfo.bmiHeader.biSizeImage; i += 4)
			{
				if (pixelBits[i + 3] != std::byte{0})
				{
					bHasAlpha = true;
					break;
				}
			}

			if (!bHasAlpha)
			{
				for (size_t i = 0; i < bitmapInfo.bmiHeader.biSizeImage; i += 4)
				{
					pixelBits[i] = Utils::PremultiplyColor(pixelBits[i]);			//Blue
					pixelBits[i + 1] = Utils::PremultiplyColor(pixelBits[i + 1]);	//Green
					pixelBits[i + 2] = Utils::PremultiplyColor(pixelBits[i + 2]);	//Red
					pixelBits[i + 3] = std::byte{255};								//Alpha
				}

				THROW_LAST_ERROR_IF(SetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);
			}

			return S_OK;
		}
		catch (...)
		{
			LOG_CAUGHT_EXCEPTION();
			return wil::ResultFromCaughtException();
		}

		static inline HRESULT GetDwmThemeColor(COLORREF& color, DWORD& opacity)
		{
			DWORD dwmColor{};
			BOOL opaque{};
			HRESULT hr{DwmGetColorizationColor(&dwmColor, &opaque)};

			if (SUCCEEDED(hr))
			{
				color = RGB(dwmColor >> 16, dwmColor >> 8, dwmColor);
				opacity = opaque ? 255 : dwmColor >> 24;
			}

			return hr;
		}

		static inline void StartupConsole()
		{
			if (GetConsoleWindow() || AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
			{
				FILE* fpstdin{stdin}, * fpstdout{stdout}, * fpstderr{stderr};
				freopen_s(&fpstdin, "CONIN$", "r", stdin);
				freopen_s(&fpstdout, "CONOUT$", "w", stdout);
				freopen_s(&fpstderr, "CONOUT$", "w", stderr);
				_wsetlocale(LC_ALL, L"chs");
			}
		}

		static inline void ShutdownConsole()
		{
			fclose(stdin);
			fclose(stdout);
			fclose(stderr);

			FreeConsole();
		}

		static inline bool ConsoleGetConfirmation()
		{
			do
			{
				char result{static_cast<char>(getchar())};

				if (result == 'Y' || result == 'y')
				{
					return true;
				}
				if (result == 'N' || result == 'n')
				{
					return false;
				}
			}
			while (true);

			return false;
		}

		static inline void OutputModuleString(UINT uId, HMODULE hModule = HINST_THISCOMPONENT)
		{
			WCHAR msgString[MAX_PATH + 1] {};
			LoadStringW(hModule, uId, msgString, MAX_PATH);
			wprintf_s(L"%s\n", msgString);
		}
	}
}