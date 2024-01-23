#pragma once
#include "framework.h"
#include "cpprt.h"

namespace TranslucentFlyouts
{
	namespace RegHelper
	{
		constexpr std::wstring_view g_regPath{ L"Software\\TranslucentFlyouts" };
#ifdef _WIN64
		constexpr std::wstring_view g_internalRegPath{ L"Software\\TranslucentFlyouts_Internals" };
#else
		constexpr std::wstring_view g_internalRegPath{ L"Software\\TranslucentFlyouts_Internals(x86)" };
#endif // _WIN64

		template <typename T, bool reverse>
		inline std::optional<T> GetValueInternal(std::wstring_view root, std::vector<std::wstring_view> keyTree, std::wstring_view valueName, size_t maxFallThrough)
		{
			std::optional<T> value{};
			std::wstring keyName{ root };
			maxFallThrough += 1;

			if (!keyTree.empty())
			{
				for (auto index{ 0 }; index < keyTree.size(); index++)
				{
					keyName = root;
					for (auto i{ keyTree.size() - 1 }; i >= index && maxFallThrough > 0; i--, maxFallThrough--)
					{
						if (!keyTree[i].empty())
						{
							keyName += L"\\";
							keyName += keyTree[i];
						}
					}

					if constexpr (reverse)
					{
						value = wil::reg::try_get_value<T>(HKEY_LOCAL_MACHINE, keyName.c_str(), valueName.data());
						if (value)
						{
							break;
						}

						value = wil::reg::try_get_value<T>(HKEY_CURRENT_USER, keyName.c_str(), valueName.data());
						if (value)
						{
							break;
						}
					}
					else
					{
						value = wil::reg::try_get_value<T>(HKEY_CURRENT_USER, keyName.c_str(), valueName.data());
						if (value)
						{
							break;
						}

						value = wil::reg::try_get_value<T>(HKEY_LOCAL_MACHINE, keyName.c_str(), valueName.data());
						if (value)
						{
							break;
						}
					}
				}
			}
			else
			{
				if constexpr (reverse)
				{
					value = wil::reg::try_get_value<T>(HKEY_LOCAL_MACHINE, keyName.c_str(), valueName.data());
					if (value)
					{
						return value;
					}

					value = wil::reg::try_get_value<T>(HKEY_CURRENT_USER, keyName.c_str(), valueName.data());
					if (value)
					{
						return value;
					}
				}
				else
				{
					value = wil::reg::try_get_value<T>(HKEY_CURRENT_USER, keyName.c_str(), valueName.data());
					if (value)
					{
						return value;
					}

					value = wil::reg::try_get_value<T>(HKEY_LOCAL_MACHINE, keyName.c_str(), valueName.data());
					if (value)
					{
						return value;
					}
				}
			}

			return value;
		}
		template <typename T, bool reverse>
		inline HRESULT SetValueInternal(std::wstring_view root, std::vector<std::wstring_view> keyTree, std::wstring_view valueName, T value, size_t maxFallThrough)
		{
			std::wstring keyName{ root };
			maxFallThrough += 1;

			if (!keyTree.empty())
			{
				for (auto index{ 0 }; index < keyTree.size(); index++)
				{
					keyName = root;
					for (auto i{ keyTree.size() - 1 }; i >= index && maxFallThrough > 0; i--, maxFallThrough--)
					{
						if (!keyTree[i].empty())
						{
							keyName += L"\\";
							keyName += keyTree[i];
						}
					}

					if constexpr (reverse)
					{
						if (FAILED(wil::reg::set_value_nothrow<T>(HKEY_LOCAL_MACHINE, keyName.c_str(), valueName.data(), value)))
						{
							wil::reg::set_value<T>(HKEY_CURRENT_USER, keyName.c_str(), valueName.data(), value);
							break;
						}
					}
					else
					{

						if (FAILED(wil::reg::set_value_nothrow<T>(HKEY_CURRENT_USER, keyName.c_str(), valueName.data(), value)))
						{
							wil::reg::set_value<T>(HKEY_LOCAL_MACHINE, keyName.c_str(), valueName.data(), value);
							break;
						}
					}
				}
			}
			else
			{
				if constexpr (reverse)
				{
					if (FAILED(wil::reg::set_value_nothrow<T>(HKEY_LOCAL_MACHINE, keyName.c_str(), valueName.data(), value)))
					{
						return wil::reg::set_value_nothrow<T>(HKEY_CURRENT_USER, keyName.c_str(), valueName.data(), value);
					}
				}
				else
				{

					if (FAILED(wil::reg::set_value_nothrow<T>(HKEY_CURRENT_USER, keyName.c_str(), valueName.data(), value)))
					{
						return wil::reg::set_value_nothrow<T>(HKEY_LOCAL_MACHINE, keyName.c_str(), valueName.data(), value);
					}
				}
			}

			return S_OK;
		}

		template <typename T>
		inline T Get(std::vector<std::wstring_view> keyTree, std::wstring_view valueName, const T& defaultValue, size_t maxFallThrough = 0)
		{
			auto result{ TryGet<T>(keyTree, valueName, maxFallThrough) };
			return result.has_value() ? result.value() : defaultValue;
		}
		template <typename T>
		inline std::optional<T> TryGet(std::vector<std::wstring_view> keyTree, std::wstring_view valueName, size_t maxFallThrough = 0)
		{
			return GetValueInternal<T, false>(g_regPath.data(), keyTree, valueName, maxFallThrough);
		}

		template <typename T>
		inline T __Get(std::vector<std::wstring_view> keyTree, std::wstring_view valueName, const T& defaultValue, size_t maxFallThrough = 0)
		{
			auto result{ __TryGet<T>(keyTree, valueName, maxFallThrough) };
			return result.has_value() ? result.value() : defaultValue;
		}
		template <typename T>
		inline std::optional<T> __TryGet(std::vector<std::wstring_view> keyTree, std::wstring_view valueName, size_t maxFallThrough = 0)
		{
			return GetValueInternal<T, true>(g_internalRegPath.data(), keyTree, valueName, maxFallThrough);
		}
		template <typename T>
		inline HRESULT __Set(std::vector<std::wstring_view> keyTree, std::wstring_view valueName, const T& value, size_t maxFallThrough = 0)
		{
			return SetValueInternal<T, true>(g_internalRegPath.data(), keyTree, valueName, value, maxFallThrough);
		}
	}
}

