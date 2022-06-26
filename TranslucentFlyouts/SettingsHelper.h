#pragma once
#include "pch.h"
#include "tflapi.h"

namespace TranslucentFlyoutsLib
{
	class Settings
	{
	private:
		BYTE bOpacity = (BYTE)GetCurrentFlyoutOpacity();
		DWORD dwEffect = GetCurrentFlyoutEffect();
		DWORD dwBorder = GetCurrentFlyoutBorder();
		DWORD dwPolicy = GetCurrentFlyoutPolicy();
		DWORD dwColorizeOption = GetCurrentFlyoutColorizeOption();
	public:
		inline void Update()
		{
			bOpacity = (BYTE)GetCurrentFlyoutOpacity();
			dwEffect = GetCurrentFlyoutEffect();
			dwBorder = GetCurrentFlyoutBorder();
			dwPolicy = GetCurrentFlyoutPolicy();
			dwColorizeOption = GetCurrentFlyoutColorizeOption();
		}

		inline BYTE GetOpacity()
		{
			return bOpacity;
		}

		inline DWORD GetEffect()
		{
			return dwEffect;
		}

		inline DWORD GetBorder()
		{
			return dwBorder;
		}

		inline DWORD GetPolicy()
		{
			return dwPolicy;
		}

		inline DWORD GetColorizeOption()
		{
			return dwColorizeOption;
		}
	};
}