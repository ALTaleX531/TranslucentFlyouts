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
				} }
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

		static inline auto MakeHRErrStr(HRESULT hr)
		{
			WCHAR szError[MAX_PATH + 1] {};
			FormatMessageW(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				hr,
				0,
				szError,
				_countof(szError),
				nullptr
			);

			return std::wstring{szError};
		}

		static inline std::wstring make_current_folder_file_str(std::wstring_view baseFileName)
		{
			WCHAR filePath[MAX_PATH + 1] {L""};
			[&]()
			{
				RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, HINST_THISCOMPONENT);
				RETURN_LAST_ERROR_IF(GetModuleFileNameW(HINST_THISCOMPONENT, filePath, _countof(filePath)) == 0);
				RETURN_IF_FAILED(PathCchRemoveFileSpec(filePath, _countof(filePath)));
				RETURN_IF_FAILED(PathCchAppend(filePath, _countof(filePath), baseFileName.data()));
				return S_OK;
			}
			();

			return std::wstring{filePath};
		}

		static inline std::wstring get_module_base_file_name(HMODULE moduleHandle)
		{
			WCHAR filePath[MAX_PATH + 1] {L""};
			[&]()
			{
				RETURN_LAST_ERROR_IF(GetModuleFileNameW(moduleHandle, filePath, _countof(filePath)) == 0);
				PathStripPathW(filePath);
				return S_OK;
			}
			();

			return std::wstring{filePath};
		}

		static inline std::optional<wil::unique_rouninitialize_call> RoInit(HRESULT* hresult = nullptr)
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

		static bool IsValidFlyout(HWND hWnd)
		{
			return
				IsWin32PopupMenu(hWnd) ||
				IsWindowClass(hWnd, L"DropDown") ||
				IsWindowClass(hWnd, L"Listviewpopup") ||
				IsWindowClass(hWnd, TOOLTIPS_CLASSW);
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

		static bool IsBitmapSupportAlpha(HBITMAP bitmap)
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
					auto pixelBits{ std::make_unique<std::byte[]>(bitmapInfo.bmiHeader.biSizeImage) };

					RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

					if (bitmapInfo.bmiHeader.biBitCount != 32)
					{
						return S_OK;
					}

					for (size_t i = 0; i < bitmapInfo.bmiHeader.biSizeImage; i += 4)
					{
						if (pixelBits[i + 3] != std::byte{ 0 })
						{
							hasAlpha = true;
							break;
						}
					}

					return S_OK;
				}
				()
			};

			LOG_IF_FAILED(hr);

			return hasAlpha;
		}

		static HBITMAP ConvertTo32BPP(HBITMAP bitmap) try
		{
			THROW_HR_IF_NULL(E_INVALIDARG, bitmap);
			THROW_HR_IF(E_INVALIDARG, GetObjectType(bitmap) != OBJ_BITMAP);

			BITMAPINFO bitmapInfo{ sizeof(bitmapInfo.bmiHeader) };
			auto hdc{ wil::GetDC(nullptr) };
			THROW_LAST_ERROR_IF_NULL(hdc);
			THROW_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) == 0);
			bitmapInfo.bmiHeader.biCompression = BI_RGB;
			auto pixelBits{ std::make_unique<std::byte[]>(bitmapInfo.bmiHeader.biSizeImage) };
			THROW_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, bitmapInfo.bmiHeader.biHeight, pixelBits.get(), &bitmapInfo, DIB_RGB_COLORS) == 0);

			std::byte* pixelBitsWithAlpha{ nullptr };
			BITMAPINFO bitmapWithAlphaInfo{ {sizeof(bitmapInfo.bmiHeader), bitmapInfo.bmiHeader.biWidth, -abs(bitmapInfo.bmiHeader.biHeight), 1, 32, BI_RGB} };
			wil::unique_hbitmap bitmapWithAlpha{ CreateDIBSection(nullptr, &bitmapWithAlphaInfo, DIB_RGB_COLORS, (void**)&pixelBitsWithAlpha, nullptr, 0) };
			THROW_LAST_ERROR_IF_NULL(bitmapWithAlpha);
			wil::unique_hdc memoryDC{ CreateCompatibleDC(hdc.get()) };
			THROW_LAST_ERROR_IF_NULL(memoryDC);

			size_t bitmapSize{ static_cast<size_t>(bitmapInfo.bmiHeader.biWidth) * static_cast<size_t>(bitmapInfo.bmiHeader.biHeight) * 4ull };
			for (size_t i = 0; i < bitmapSize; i += 4)
			{
				pixelBitsWithAlpha[i + 3] = std::byte{ 255 };	//Alpha
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
			LOG_CAUGHT_EXCEPTION();
			return nullptr;
		}

		struct ShaderEffect
		{
			virtual void Apply(LONG width, LONG height, std::shared_ptr<std::byte[]> bits) = 0;
		};
		class SpriteEffect : public ShaderEffect
		{
		public:
			SpriteEffect(DWORD color, DWORD distance) : m_color{color}, m_distance{distance} {}
			~SpriteEffect() = default;

			void Apply(LONG width, LONG height, std::shared_ptr<std::byte[]> bits) override
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
			void dfs(std::shared_ptr<bool[]> map, std::shared_ptr<std::byte[]> bits, LONG x, LONG y) const
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

				auto alpha{ GetAlpha(m_color) };
				auto rgb{ MakeCOLORREF(m_color) };

				auto distance
				{
					sqrt(
						pow(abs(std::to_integer<LONG>(b) - GetBValue(rgb)), 2) +
						pow(abs(std::to_integer<LONG>(g) - GetGValue(rgb)), 2) +
						pow(abs(std::to_integer<LONG>(r) - GetRValue(rgb)), 2) +
						pow(abs(std::to_integer<LONG>(a) - std::to_integer<LONG>(alpha)), 2)
					)
				};
				if (distance <= m_distance)
				{
					b = std::byte{ 0 };
					g = std::byte{ 0 };
					r = std::byte{ 0 };
					a = std::byte{ 0 };
				}
				else
				{
					OutputDebugStringW(
						std::format(L"distance: {}, bgra: [{}, {}, {}, {}]", distance, std::to_integer<DWORD>(b), std::to_integer<DWORD>(g), std::to_integer<DWORD>(r), std::to_integer<DWORD>(a)).c_str()
					);
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
		static HRESULT BitmapApplyEffect(HBITMAP bitmap, std::initializer_list<std::shared_ptr<ShaderEffect>> effectList)
		{
			RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, bitmap);

			BITMAPINFO bitmapInfo{ sizeof(bitmapInfo.bmiHeader) };
			auto hdc{ wil::GetDC(nullptr) };
			RETURN_LAST_ERROR_IF_NULL(hdc);

			RETURN_HR_IF_EXPECTED(E_INVALIDARG, GetObjectType(bitmap) != OBJ_BITMAP);
			RETURN_LAST_ERROR_IF(GetDIBits(hdc.get(), bitmap, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) == 0);

			bitmapInfo.bmiHeader.biCompression = BI_RGB;
			auto pixelBits{ std::make_shared<std::byte[]>(bitmapInfo.bmiHeader.biSizeImage) };

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
		[[maybe_unused]] static HRESULT PrepareAlpha(HBITMAP bitmap)
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
	}
}