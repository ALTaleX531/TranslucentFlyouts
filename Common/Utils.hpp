#pragma once
#include "framework.h"
#include "cpprt.h"
#include "wil.h"
#include <winrt/base.h>
#pragma warning(push)
#pragma warning(disable : 6388)

namespace TranslucentFlyouts::Utils
{
	struct external_dc
	{
		HDC dc{nullptr};
		int saved{-1};
		external_dc(HDC hdc = nullptr) : dc{hdc}
		{
			if (hdc)
			{
				saved = SaveDC(hdc);
			}
		}
		WI_NODISCARD inline operator HDC() const WI_NOEXCEPT
		{
			return dc;
		}
		static inline void close(external_dc pdc) WI_NOEXCEPT { if (pdc.dc)
		{
			::RestoreDC(pdc.dc, pdc.saved);
		}}
	};
	typedef wil::unique_any<HDC, decltype(&external_dc::close), external_dc::close, wil::details::pointer_access_noaddress, external_dc> unique_ext_hdc;

	template <typename T, typename Type>
	__forceinline constexpr T union_cast(Type pointer)
	{
		union
		{
			Type real_ptr;
			T fake_ptr;
		} rf{.real_ptr{pointer}};

		return rf.fake_ptr;
	}

	static auto to_error_wstring(HRESULT hr)
	{
		return winrt::hresult_error{hr}.message();
	}

	static std::wstring make_current_folder_file_str(std::wstring_view baseFileName)
	{
		static const auto s_current_module_path
		{
			[]() -> std::wstring
			{
				WCHAR filePath[MAX_PATH + 1] {};
				GetModuleFileNameW(wil::GetModuleInstanceHandle(), filePath, _countof(filePath));
				return std::wstring{ filePath };
			} ()
		};

		WCHAR filePath[MAX_PATH + 1] {L""};
		[&]()
		{
			wcscpy_s(filePath, s_current_module_path.c_str());
			RETURN_IF_FAILED(PathCchRemoveFileSpec(filePath, _countof(filePath)));
			RETURN_IF_FAILED(PathCchAppend(filePath, _countof(filePath), baseFileName.data()));
			return S_OK;
		}
		();

		return std::wstring{filePath};
	}

	inline std::wstring get_module_base_file_name(HMODULE moduleHandle)
	{
		WCHAR filePath[MAX_PATH + 1] {L""};
		[&]()
		{
			RETURN_LAST_ERROR_IF(GetModuleBaseNameW(GetCurrentProcess(), moduleHandle, filePath, _countof(filePath)) == 0);
			PathStripPathW(filePath);
			return S_OK;
		} ();

		return std::wstring{filePath};
	}

	inline std::wstring get_process_name()
	{
		static const auto s_processName{ get_module_base_file_name(nullptr) };
		return s_processName;
	}

	inline bool IsRunningAsAdministrator()
	{
		wil::unique_handle token{ nullptr };
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.put()))
		{
			return false;
		}

		DWORD size{ 0 };
		TOKEN_ELEVATION elevation{ 0 };
		if (!GetTokenInformation(token.get(), TokenElevation, &elevation, sizeof(elevation), &size))
		{
			return false;
		}

		return elevation.TokenIsElevated;
	}

	static std::optional<wil::unique_rouninitialize_call> RoInit(HRESULT* hresult = nullptr)
	{
		HRESULT hr{::RoInitialize(RO_INIT_SINGLETHREADED)};

		if (SUCCEEDED(hr) || hr == S_FALSE)
		{
			if (hresult)
			{
				*hresult = S_OK;
			}
			return wil::unique_rouninitialize_call{};
		}

		if (hresult)
		{
			*hresult = hr;
		}

		return std::nullopt;
	}

	static bool IsBadReadPtr(const void* ptr)
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

	inline std::wstring GetVersion(HMODULE moduleHandle) try
	{
		WCHAR pszModuleName[MAX_PATH + 1];
		GetModuleFileNameW(moduleHandle, pszModuleName, MAX_PATH);
			
		DWORD handle{0};
		DWORD size{ GetFileVersionInfoSizeW(pszModuleName, &handle) };
		THROW_LAST_ERROR_IF(!size);

		auto data{ std::make_unique<BYTE[]>(size) };
		THROW_LAST_ERROR_IF(!GetFileVersionInfoW(pszModuleName, handle, size, data.get()));

		UINT len{0};
		VS_FIXEDFILEINFO* fileInfo{nullptr};
		THROW_IF_WIN32_BOOL_FALSE(VerQueryValueW(data.get(), L"\\", reinterpret_cast<PVOID*>(&fileInfo), &len));
		THROW_LAST_ERROR_IF(!len);

		THROW_WIN32_IF(ERROR_INVALID_IMAGE_HASH, fileInfo->dwSignature != VS_FFI_SIGNATURE);

		// Doesn't matter if you are on 32 bit or 64 bit,
		// DWORD is always 32 bits, so first two revision numbers
		// come from dwFileVersionMS, last two come from dwFileVersionLS
		return std::format(
			L"{}.{}.{}.{}",
			HIWORD(fileInfo->dwFileVersionMS),
			LOWORD(fileInfo->dwFileVersionMS),
			HIWORD(fileInfo->dwFileVersionLS),
			LOWORD(fileInfo->dwFileVersionLS)
		);
	}
	catch (...)
	{
		LOG_CAUGHT_EXCEPTION();
		return std::wstring{};
	}

	static void CloakWindow(HWND hWnd, BOOL cloak)
	{
		DwmSetWindowAttribute(hWnd, DWMWA_CLOAK, &cloak, sizeof(cloak));
		DwmTransitionOwnedWindow(hWnd, DWMTRANSITION_OWNEDWINDOW_REPOSITION);
	}

	inline bool IsWindowClass(HWND hWnd, std::wstring_view className = L"", std::wstring_view windowText = L"")
	{
		bool classNameOK{true};
		if (!className.empty())
		{
			WCHAR pszClass[MAX_PATH + 1] {};
			GetClassNameW(hWnd, pszClass, MAX_PATH);
			classNameOK = (!_wcsicmp(pszClass, className.data()));
		}

		bool windowTextOK{true};
		if (!windowText.empty())
		{
			WCHAR pszText[MAX_PATH + 1] {};
			InternalGetWindowText(hWnd, pszText, MAX_PATH);
			windowTextOK = (!_wcsicmp(pszText, windowText.data()));
		}

		return classNameOK && windowTextOK;
	}

	static bool IsPopupMenu(HWND hWnd)
	{
		WCHAR pszClass[MAX_PATH + 1] {};
		GetClassNameW(hWnd, pszClass, MAX_PATH);

		if (GetClassLongPtr(hWnd, GCW_ATOM) == 32768)
		{
			return true;
		}

		return false;
	}

	static HWND GetCurrentMenuOwner()
	{
		GUITHREADINFO guiThreadInfo{sizeof(GUITHREADINFO)};
		GetGUIThreadInfo(GetCurrentThreadId(), &guiThreadInfo);

		return guiThreadInfo.hwndMenuOwner;
	}

	static UCHAR PremultiplyColor(UCHAR color, UCHAR alpha = 255)
	{
		return static_cast<UCHAR>(color * (alpha + 1) >> 8);
	}

	static HBITMAP CreateDIB(
		LONG width = 1,
		LONG height = -1,
		UCHAR** bits = nullptr
	)
	{
		BITMAPINFO bitmapInfo{{sizeof(bitmapInfo.bmiHeader), width, height, 1, 32, BI_RGB}};

		return CreateDIBSection(nullptr, &bitmapInfo, DIB_RGB_COLORS, reinterpret_cast<PVOID*>(bits), nullptr, 0);
	}

	static HBRUSH CreateSolidColorBrushWithAlpha(COLORREF color, UCHAR alpha)
	{
		wil::unique_hbitmap bitmap{nullptr};
		UCHAR* bits{nullptr};

		bitmap.reset(CreateDIB(1, -1, &bits));
		if (!bitmap)
		{
			return nullptr;
		}

		bits[0] = PremultiplyColor(GetBValue(color), alpha);
		bits[1] = PremultiplyColor(GetGValue(color), alpha);
		bits[2] = PremultiplyColor(GetRValue(color), alpha);
		bits[3] = alpha;

		return CreatePatternBrush(bitmap.get());
	}

	static COLORREF MakeCOLORREF(DWORD argb)
	{
		auto r{argb >> 16 & 0xff};
		auto g{argb >> 8 & 0xff};
		auto b{argb & 0xff};

		return RGB(r, g, b);
	}

	static DWORD MakeArgb(UCHAR a, UCHAR r, UCHAR g, UCHAR b)
	{
		UCHAR colorBits[4]{b, g, r, a};
		return *reinterpret_cast<DWORD*>(colorBits);
	}

	static UCHAR GetAlphaFromARGB(DWORD argb)
	{
		return argb >> 24;
	}

	static std::array<UCHAR, 4> FromARGB(DWORD argb)
	{
		UCHAR* colorBits{ reinterpret_cast<UCHAR*>(&argb) };
		return { colorBits[3], colorBits[2], colorBits[1], colorBits[0] };
	}

	inline bool IsBitmapSupportAlpha(HBITMAP bitmap)
	{
		bool hasAlpha{ false };
		HRESULT hr
		{
			[&]()
			{
				RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, bitmap);

				BITMAPINFO bitmapInfo{ sizeof(bitmapInfo.bmiHeader) };
				auto hdc{ wil::GetDC(nullptr) };
				RETURN_LAST_ERROR_IF_NULL(hdc);

				RETURN_HR_IF_EXPECTED(E_INVALIDARG, GetObjectType(bitmap) != OBJ_BITMAP);
				RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) == 0);

				bitmapInfo.bmiHeader.biCompression = BI_RGB;
				auto pixelBits{ std::make_unique<UCHAR[]>(bitmapInfo.bmiHeader.biSizeImage) };

				RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

				if (bitmapInfo.bmiHeader.biBitCount != 32)
				{
					return S_OK;
				}

				for (size_t i = 0; i < bitmapInfo.bmiHeader.biSizeImage; i += 4)
				{
					if (pixelBits[i + 3] != 0)
					{
						hasAlpha = true;
						break;
					}
				}

				return S_OK;
			}
			()
		};

		return hasAlpha;
	}

	inline HBITMAP ConvertTo32BPP(HBITMAP bitmap) try
	{
		THROW_HR_IF_NULL(E_INVALIDARG, bitmap);
		THROW_HR_IF(E_INVALIDARG, GetObjectType(bitmap) != OBJ_BITMAP);

		BITMAPINFO bitmapInfo{ sizeof(bitmapInfo.bmiHeader) };
		auto hdc{ wil::GetDC(nullptr) };
		THROW_LAST_ERROR_IF_NULL(hdc);
		THROW_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) == 0);
		bitmapInfo.bmiHeader.biCompression = BI_RGB;
		auto pixelBits{ std::make_unique<UCHAR[]>(bitmapInfo.bmiHeader.biSizeImage) };
		THROW_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

		UCHAR* pixelBitsWithAlpha{ nullptr };
		BITMAPINFO bitmapWithAlphaInfo{ {sizeof(bitmapInfo.bmiHeader), bitmapInfo.bmiHeader.biWidth, -abs(bitmapInfo.bmiHeader.biHeight), 1, 32, BI_RGB} };
		wil::unique_hbitmap bitmapWithAlpha{ CreateDIBSection(nullptr, &bitmapWithAlphaInfo, DIB_RGB_COLORS, (void**)&pixelBitsWithAlpha, nullptr, 0) };
		THROW_LAST_ERROR_IF_NULL(bitmapWithAlpha);
		wil::unique_hdc memoryDC{ CreateCompatibleDC(hdc.get()) };
		THROW_LAST_ERROR_IF_NULL(memoryDC);

		size_t bitmapSize{ static_cast<size_t>(bitmapInfo.bmiHeader.biWidth) * static_cast<size_t>(bitmapInfo.bmiHeader.biHeight) * 4ull };
		for (size_t i = 0; i < bitmapSize; i += 4)
		{
			pixelBitsWithAlpha[i + 3] = 255;	//Alpha
		}

		{
			auto selectedObject{ wil::SelectObject(memoryDC.get(), bitmapWithAlpha.get()) };
			StretchDIBits(
				memoryDC.get(),
				0, 0, bitmapInfo.bmiHeader.biWidth, bitmapInfo.bmiHeader.biHeight,
				0, 0, bitmapInfo.bmiHeader.biWidth, bitmapInfo.bmiHeader.biHeight,
				pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS, SRCPAINT
			);
		}

		return bitmapWithAlpha.release();
	}
	catch (...)
	{
		return nullptr;
	}

	struct ShaderEffect
	{
		virtual void Apply(LONG width, LONG height, std::shared_ptr<UCHAR[]> bits) = 0;
	};
	class SpriteEffect : public ShaderEffect
	{
	public:
		SpriteEffect(DWORD color, DWORD distance) : m_color{color}, m_distance{distance} {}
		~SpriteEffect() = default;

		void Apply(LONG width, LONG height, std::shared_ptr<UCHAR[]> bits) override
		{
			m_width = width;
			m_height = height;

			auto map{std::make_shared<bool[]>(width * height)};
			dfs(map, bits, 0, 0);
			dfs(map, bits, width - 1, 0);
			dfs(map, bits, 0, height - 1);
			dfs(map, bits, width - 1, height - 1);
		}
	private:
		void dfs(std::shared_ptr<bool[]> map, std::shared_ptr<UCHAR[]> bits, LONG x, LONG y) const
		{
			if ((y >= m_height || y < 0) || (x >= m_width || x < 0))
			{
				return;
			}
			if (map[m_width * y + x])
			{
				return;
			}
				
			auto& b{ bits[4 * m_width * y + x * 4] };
			auto& g{ bits[4 * m_width * y + x * 4 + 1] };
			auto& r{ bits[4 * m_width * y + x * 4 + 2] };
			auto& a{ bits[4 * m_width * y + x * 4 + 3] };

			auto alpha{ GetAlphaFromARGB(m_color) };
			auto rgb{ MakeCOLORREF(m_color) };

			auto distance
			{
				sqrt(
					pow(abs(static_cast<LONG>(b - GetBValue(rgb))), 2) +
					pow(abs(static_cast<LONG>(g - GetGValue(rgb))), 2) +
					pow(abs(static_cast<LONG>(r - GetRValue(rgb))), 2) +
					pow(abs(static_cast<LONG>(a - alpha)), 2)
				)
			};
			if (distance <= m_distance)
			{
				b = 0;
				g = 0;
				r = 0;
				a = 0;
			}
			else
			{
#ifdef _DEBUG
				OutputDebugStringW(
					std::format(L"distance: {}, bgra: [{}, {}, {}, {}]", distance, b, g, r, a).c_str()
				);
#endif
				return;
			}

			map[m_width * y + x] = true;
			dfs(map, bits, x + 1, y);
			dfs(map, bits, x, y + 1);
			dfs(map, bits, x - 1, y);
			dfs(map, bits, x, y - 1);
		}

		LONG m_width{0};
		LONG m_height{0};

		DWORD m_color{0};
		DWORD m_distance{ 0 };
	};
	inline HRESULT BitmapApplyEffect(HBITMAP bitmap, std::initializer_list<std::shared_ptr<ShaderEffect>> effectList)
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, bitmap);

		BITMAPINFO bitmapInfo{ sizeof(bitmapInfo.bmiHeader) };
		auto hdc{ wil::GetDC(nullptr) };
		RETURN_LAST_ERROR_IF_NULL(hdc);

		RETURN_HR_IF_EXPECTED(E_INVALIDARG, GetObjectType(bitmap) != OBJ_BITMAP);
		RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) == 0);

		bitmapInfo.bmiHeader.biCompression = BI_RGB;
		auto pixelBits{ std::make_shared<UCHAR[]>(bitmapInfo.bmiHeader.biSizeImage) };

		RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

		// Nothing we can do to a bitmap whose bit count isn't 32 :(
		RETURN_HR_IF_EXPECTED(E_NOTIMPL, bitmapInfo.bmiHeader.biBitCount != 32);

		for (const auto effect : effectList)
		{
			effect->Apply(abs(bitmapInfo.bmiHeader.biWidth), abs(bitmapInfo.bmiHeader.biHeight), pixelBits);
		}

		RETURN_LAST_ERROR_IF(SetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

		return S_OK;
	}

	static HBITMAP Promise32BPP(HBITMAP bitmap)
	{
		HBITMAP bitmapWithAlpha{ nullptr };
		if (IsBitmapSupportAlpha(bitmap))
		{
			bitmapWithAlpha = reinterpret_cast<HBITMAP>(CopyImage(bitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE));
		}
		else
		{
			bitmapWithAlpha = ConvertTo32BPP(bitmap);
		}

		return bitmapWithAlpha;
	}

	// Adjust alpha channel of the bitmap, return E_NOTIMPL if the bitmap is unsupported
	[[maybe_unused]] inline HRESULT PrepareAlpha(HBITMAP bitmap)
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, bitmap);

		BITMAPINFO bitmapInfo{sizeof(bitmapInfo.bmiHeader)};
		auto hdc{wil::GetDC(nullptr)};
		RETURN_LAST_ERROR_IF_NULL(hdc);

		RETURN_HR_IF_EXPECTED(E_INVALIDARG, GetObjectType(bitmap) != OBJ_BITMAP);
		RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) == 0);

		bitmapInfo.bmiHeader.biCompression = BI_RGB;
		auto pixelBits{std::make_unique<UCHAR[]>(bitmapInfo.bmiHeader.biSizeImage)};

		RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

		// Nothing we can do to a bitmap whose bit count isn't 32 :(
		RETURN_HR_IF_EXPECTED(E_NOTIMPL, bitmapInfo.bmiHeader.biBitCount != 32);

		bool bHasAlpha{false};
		for (size_t i{0}; i < bitmapInfo.bmiHeader.biSizeImage; i += 4)
		{
			if (pixelBits[i + 3] != 0)
			{
				bHasAlpha = true;
				break;
			}
		}

		if (!bHasAlpha)
		{
			for (size_t i{0}; i < bitmapInfo.bmiHeader.biSizeImage; i += 4)
			{
				pixelBits[i] = Utils::PremultiplyColor(pixelBits[i]);			//Blue
				pixelBits[i + 1] = Utils::PremultiplyColor(pixelBits[i + 1]);	//Green
				pixelBits[i + 2] = Utils::PremultiplyColor(pixelBits[i + 2]);	//Red
				pixelBits[i + 3] = 255;											//Alpha
			}

			RETURN_LAST_ERROR_IF(SetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);
		}

		return S_OK;
	}

	static DWORD GetThemeColorizationColor()
	{
		static const auto s_GetImmersiveColorFromColorSetEx{reinterpret_cast<DWORD(WINAPI*)(DWORD dwImmersiveColorSet, DWORD dwImmersiveColorType, bool bIgnoreHighContrast, DWORD dwHighContrastCacheMode)>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(95)))};
		static const auto s_GetImmersiveColorTypeFromName{ reinterpret_cast<DWORD(WINAPI*)(LPCWSTR name)>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(96))) };
		static const auto s_GetImmersiveUserColorSetPreference{ reinterpret_cast<DWORD(WINAPI*)(bool bForceCheckRegistry, bool bSkipCheckOnFail)>(GetProcAddress(GetModuleHandleW(L"UxTheme.dll"), MAKEINTRESOURCEA(98))) };

		DWORD argb{0};
		if (s_GetImmersiveColorFromColorSetEx && s_GetImmersiveColorTypeFromName && s_GetImmersiveUserColorSetPreference) [[likely]]
		{
			DWORD abgr
			{
				s_GetImmersiveColorFromColorSetEx(
					s_GetImmersiveUserColorSetPreference(0, 0),
					s_GetImmersiveColorTypeFromName(L"ImmersiveStartHoverBackground"),
					true,
					0
				)
			};
			argb = MakeArgb(
				abgr >> 24,
				abgr & 0xff,
				abgr >> 8 & 0xff,
				abgr >> 16 & 0xff
			);
		}
		else
		{
			BOOL opaque{FALSE};
			DwmGetColorizationColor(&argb, &opaque);
		}

		return argb;
	}
}

#pragma warning(pop)