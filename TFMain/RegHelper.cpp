#include "pch.h"
#include "RegHelper.hpp"

using namespace std;
using namespace wil;
using namespace::TranslucentFlyouts;

wstring_view g_regPath{ L"Software\\TranslucentFlyouts" };
#ifdef _WIN64
	wstring_view g_internalRegPath{ L"Software\\TranslucentFlyouts_Internals" };
#else
	wstring_view g_internalRegPath{ L"Software\\TranslucentFlyouts_Internals(x86)" };
#endif // _WIN64


DWORD RegHelper::GetDword(std::wstring_view subItemName, std::wstring_view valueName, DWORD defaultValue, bool useFallback)
{
	DWORD regValue{defaultValue};

	[&]()
	{
		HRESULT hr{S_OK};
		if (subItemName.empty() && !useFallback)
		{
			return;
		}

		if (!subItemName.empty())
		{
			auto subKeyName
			{
				format(L"{}\\{}", g_regPath, subItemName)
			};
			hr = reg::get_value_dword_nothrow(
					 HKEY_CURRENT_USER, subKeyName.c_str(), valueName.data(), &regValue
				 );

			if (FAILED(hr))
			{
				hr = reg::get_value_dword_nothrow(
						 HKEY_LOCAL_MACHINE, subKeyName.c_str(), valueName.data(), &regValue
					 );
			}
			LOG_HR_IF(hr, FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

			if (!useFallback || SUCCEEDED(hr))
			{
				return;
			}
		}

		hr = reg::get_value_dword_nothrow(
				 HKEY_CURRENT_USER, g_regPath.data(), valueName.data(), &regValue
			 );
		if (FAILED(hr))
		{
			hr = reg::get_value_dword_nothrow(
				HKEY_LOCAL_MACHINE, g_regPath.data(), valueName.data(), &regValue
			);
		}

		LOG_HR_IF(hr, FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
	}
	();

	return regValue;
}

std::optional<DWORD> RegHelper::TryGetDword(std::wstring_view subItemName, std::wstring_view valueName, bool useFallback)
{
	HRESULT hr{ S_OK };
	DWORD regValue{ 0 };

	if (subItemName.empty() && !useFallback)
	{
		return nullopt;
	}

	if (!subItemName.empty())
	{
		auto subKeyName
		{
			format(L"{}\\{}", g_regPath, subItemName)
		};
		hr = reg::get_value_dword_nothrow(
			HKEY_CURRENT_USER, subKeyName.c_str(), valueName.data(), &regValue
		);

		if (FAILED(hr))
		{
			hr = reg::get_value_dword_nothrow(
				HKEY_LOCAL_MACHINE, subKeyName.c_str(), valueName.data(), &regValue
			);
		}
		LOG_HR_IF(hr, FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

		if (!useFallback)
		{
			if (SUCCEEDED(hr))
			{
				return regValue;
			}
			else
			{
				return nullopt;
			}
		}
	}

	hr = reg::get_value_dword_nothrow(
		HKEY_CURRENT_USER, g_regPath.data(), valueName.data(), &regValue
	);
	if (FAILED(hr))
	{
		hr = reg::get_value_dword_nothrow(
			HKEY_LOCAL_MACHINE, g_regPath.data(), valueName.data(), &regValue
		);

		if (FAILED(hr))
		{
			return nullopt;
		}
	}

	LOG_HR_IF(hr, FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
	return regValue;
}

DWORD64 RegHelper::_GetQword(std::wstring_view valueName, DWORD64 defaultValue)
{
	HRESULT hr{ S_OK };
	DWORD64 regValue{ 0 };

	hr = reg::get_value_qword_nothrow(
		HKEY_LOCAL_MACHINE, g_internalRegPath.data(), valueName.data(), &regValue
	);
	if (FAILED(hr))
	{
		hr = reg::get_value_qword_nothrow(
			HKEY_CURRENT_USER, g_internalRegPath.data(), valueName.data(), &regValue
		);
	}

	LOG_HR_IF(hr, FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

	return regValue;
}

void RegHelper::_SetQword(std::wstring_view valueName, DWORD64 value) try
{
	if (FAILED(reg::set_value_qword_nothrow(HKEY_LOCAL_MACHINE, g_internalRegPath.data(), valueName.data(), value)))
	{
		reg::set_value_qword(HKEY_CURRENT_USER, g_internalRegPath.data(), valueName.data(), value);
	}
}
CATCH_LOG_RETURN()

std::optional<wstring> RegHelper::_TryGetString(wstring_view valueName) try
{
	std::optional<wstring> result{};

	result = reg::try_get_value_string(
		HKEY_LOCAL_MACHINE, g_internalRegPath.data(), valueName.data()
	);
	if (!result)
	{
		result = reg::try_get_value_string(
			HKEY_CURRENT_USER, g_internalRegPath.data(), valueName.data()
		);
	}

	return result;
}
catch (...)
{
	LOG_CAUGHT_EXCEPTION();
	return nullopt;
}

void RegHelper::_SetString(std::wstring_view valueName, std::wstring_view value) try
{
	if (FAILED(reg::set_value_string_nothrow(HKEY_LOCAL_MACHINE, g_internalRegPath.data(), valueName.data(), value.data())))
	{
		reg::set_value_string(HKEY_CURRENT_USER, g_internalRegPath.data(), valueName.data(), value.data());
	}
}
CATCH_LOG_RETURN()