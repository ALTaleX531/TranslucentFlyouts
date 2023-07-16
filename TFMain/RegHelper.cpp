#include "pch.h"
#include "RegHelper.hpp"

namespace TranslucentFlyouts::RegHelper
{
	using namespace std;

	wstring_view g_regPath{L"Software\\TranslucentFlyouts"};
}

DWORD TranslucentFlyouts::RegHelper::GetDword(std::wstring_view subItemName, std::wstring_view valueName, DWORD defaultValue, bool useFallback) try
{
	THROW_HR_IF(E_INVALIDARG, subItemName.empty() && !useFallback);

	HRESULT hr{S_OK};
	DWORD regValue{defaultValue};
	
	if (!subItemName.empty())
	{
		hr = wil::reg::get_value_dword_nothrow(
			HKEY_CURRENT_USER, format(L"{}\\{}", g_regPath, subItemName).c_str(), valueName.data(), &regValue
		);

		if (FAILED(hr))
		{
			hr = wil::reg::get_value_dword_nothrow(
				HKEY_LOCAL_MACHINE, format(L"{}\\{}", g_regPath, subItemName).c_str(), valueName.data(), &regValue
			);

			if (FAILED(hr))
			{
				THROW_HR_IF(hr, !useFallback);
			}
			else
			{
				return regValue;
			}
		}
		else
		{
			return regValue;
		}
	}

	hr = wil::reg::get_value_dword_nothrow(
			HKEY_CURRENT_USER, g_regPath.data(), valueName.data(), &regValue
	);
	if (FAILED(hr))
	{
		return wil::reg::get_value_dword(
			HKEY_LOCAL_MACHINE, g_regPath.data(), valueName.data()
		);
	}
	return regValue;
}
catch (...)
{
	LOG_CAUGHT_EXCEPTION();
	return defaultValue;
}