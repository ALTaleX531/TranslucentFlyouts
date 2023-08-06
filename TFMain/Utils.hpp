#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	namespace Utils
	{
		struct external_dc
		{
			HDC dc{nullptr};
			int saved{-1};
			external_dc(HDC hdc = nullptr) : dc{hdc} { if (hdc) { saved = SaveDC(hdc); } }
			WI_NODISCARD inline operator HDC() const WI_NOEXCEPT { return dc; }
			static inline void close(external_dc pdc) WI_NOEXCEPT { if (pdc.dc) { ::RestoreDC(pdc.dc, pdc.saved); } }
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

		static inline std::wstring make_current_folder_file_str(std::wstring_view baseFileName)
		{
			WCHAR filePath[MAX_PATH + 1]{L""};
			[&]()
			{
				RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, HINST_THISCOMPONENT);
				RETURN_LAST_ERROR_IF(GetModuleFileNameW(HINST_THISCOMPONENT, filePath, _countof(filePath)) == 0);
				RETURN_IF_FAILED(PathCchRemoveFileSpec(filePath, _countof(filePath)));
				RETURN_IF_FAILED(PathCchAppend(filePath, _countof(filePath), baseFileName.data()));
				return S_OK;
			} ();

			return std::wstring{filePath};
		}

		static inline std::optional<wil::unique_rouninitialize_call> RoInit()
		{
			HRESULT hr{::RoInitialize(RO_INIT_MULTITHREADED)};

			if (SUCCEEDED(hr) || hr == S_FALSE)
			{
				return wil::unique_rouninitialize_call{};
			}

			return std::nullopt;
		}

		static inline bool IsBadReadPtr(const void* ptr)
		{
			bool result{false};
			MEMORY_BASIC_INFORMATION mbi{};
			if (VirtualQuery(ptr, &mbi, sizeof(mbi)))
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

		static inline void CloakWindow(HWND hWnd, BOOL cloak)
		{
			DwmSetWindowAttribute(hWnd, DWMWA_CLOAK, &cloak, sizeof(cloak));
			DwmTransitionOwnedWindow(hWnd, DWMTRANSITION_OWNEDWINDOW_REPOSITION);
		}

		static inline bool IsWindowClass(HWND hWnd, std::wstring_view className = L"", std::wstring_view windowText = L"")
		{
			bool classNameOK{true};
			if (!className.empty())
			{
				WCHAR pszClass[MAX_PATH + 1]{};
				GetClassNameW(hWnd, pszClass, MAX_PATH);
				classNameOK = (!_wcsicmp(pszClass, className.data()));
			}

			bool windowTextOK{true};
			if (!windowText.empty())
			{
				WCHAR pszText[MAX_PATH + 1]{};
				InternalGetWindowText(hWnd, pszText, MAX_PATH);
				windowTextOK = (!_wcsicmp(pszText, windowText.data()));
			}

			return classNameOK && windowTextOK;
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

		static inline HWND GetCurrentMenuOwner()
		{
			GUITHREADINFO guiThreadInfo{sizeof(GUITHREADINFO)};
			LOG_IF_WIN32_BOOL_FALSE(GetGUIThreadInfo(GetCurrentThreadId(), &guiThreadInfo));

			return guiThreadInfo.hwndMenuOwner;
		}

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

		static inline HRESULT GetBrushColor(HBRUSH brush, COLORREF& color)
		{
			LOGBRUSH logBrush{};

			RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, brush);
			RETURN_LAST_ERROR_IF_EXPECTED(!GetObject(brush, sizeof(LOGBRUSH), &logBrush));
			RETURN_HR_IF_EXPECTED(E_INVALIDARG, logBrush.lbStyle != BS_SOLID);
			color = logBrush.lbColor;

			return S_OK;
		}

		static HBRUSH CreateSolidColorBrushWithAlpha(COLORREF color, std::byte alpha)
		{
			wil::unique_hbitmap bitmap{nullptr};
			std::byte* bits{nullptr};

			bitmap.reset(CreateDIB(1, -1, &bits));
			if (!bitmap)
			{
				LOG_LAST_ERROR();
				return nullptr;
			}

			bits[0] = PremultiplyColor(std::byte{GetBValue(color)}, alpha);
			bits[1] = PremultiplyColor(std::byte{GetGValue(color)}, alpha);
			bits[2] = PremultiplyColor(std::byte{GetRValue(color)}, alpha);
			bits[3] = alpha;

			return CreatePatternBrush(bitmap.get());
		}

		static HBRUSH CreateSolidColorBrushWithAlpha(HBRUSH brush, std::byte alpha)
		{
			COLORREF color{0};

			if (!brush)
			{
				return nullptr;
			}
			if (FAILED(GetBrushColor(brush, color)))
			{
				return nullptr;
			}

			return CreateSolidColorBrushWithAlpha(color, alpha);
		}

		static COLORREF MakeCOLORREF(DWORD argb)
		{
			auto r{argb >> 16 & 0xff};
			auto g{argb >> 8 & 0xff};
			auto b{argb & 0xff};

			return RGB(r, g, b);
		}

		static std::byte GetAlpha(DWORD argb)
		{
			return std::byte{argb >> 24};
		}

		// Adjust alpha channel of the bitmap, return E_NOTIMPL if the bitmap is unsupported
		static HRESULT PrepareAlpha(HBITMAP bitmap)
		{
			RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, bitmap);

			BITMAPINFO bitmapInfo{sizeof(bitmapInfo.bmiHeader)};
			auto hdc{wil::GetDC(nullptr)};
			RETURN_LAST_ERROR_IF_NULL(hdc);

			RETURN_HR_IF_EXPECTED(E_INVALIDARG, GetObjectType(bitmap) != OBJ_BITMAP);
			RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) == 0);

			bitmapInfo.bmiHeader.biCompression = BI_RGB;
			auto pixelBits{std::make_unique<std::byte[]>(bitmapInfo.bmiHeader.biSizeImage)};

			RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

			// Nothing we can do to a bitmap whose bit count isn't 32 :(
			RETURN_HR_IF_EXPECTED(E_NOTIMPL, bitmapInfo.bmiHeader.biBitCount != 32);

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

				RETURN_LAST_ERROR_IF(SetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);
			}

			return S_OK;
		}

		static inline HRESULT GetDwmThemeColor(DWORD& argb)
		{
			BOOL opaque{};
			HRESULT hr{DwmGetColorizationColor(&argb, &opaque)};

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