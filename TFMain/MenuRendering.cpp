#include "pch.h"
#include "DXHelper.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuRendering.hpp"

using namespace std;
using namespace wil;
using namespace TranslucentFlyouts;

MenuRendering& MenuRendering::GetInstance()
{
	static MenuRendering instance{};
	return instance;
}

HRESULT MenuRendering::DoCustomThemeRendering(HDC hdc, bool darkMode, int partId, int stateId, const RECT& clipRect, const RECT& paintRect)
{
	RETURN_HR_IF(E_FAIL, !DXHelper::LazyD2D::EnsureInitialized());

	auto& lazyD2D{DXHelper::LazyD2D::GetInstance()};
	auto renderTarget{lazyD2D.GetRenderTarget()};

	COLORREF color{0};

	Utils::unique_ext_hdc dc{hdc};
	IntersectClipRect(dc.get(), paintRect.left, paintRect.top, paintRect.right, paintRect.bottom);
	RETURN_IF_WIN32_BOOL_FALSE(
		PatBlt(dc.get(), paintRect.left, paintRect.top, paintRect.right - paintRect.left, paintRect.bottom - paintRect.top, BLACKNESS)
	);

	if (partId == MENU_POPUPSEPARATOR)
	{
		DWORD itemDisabled
		{
			RegHelper::GetDword(
				L"Menu\\Separator",
				L"Disabled",
				0,
				false
			)
		};

		if (itemDisabled)
		{
			return E_NOTIMPL;
		}

		DWORD separatorWidth
		{
			RegHelper::GetDword(
				L"Menu\\Separator",
				L"Width",
				MenuHandler::separatorWidth,
				false
			)
		};

		if (darkMode)
		{
			color = RegHelper::GetDword(
						L"Menu\\Separator",
						L"DarkMode_Color",
						MenuHandler::darkMode_SeparatorColor,
						false
					);
		}
		else
		{
			color = RegHelper::GetDword(
						L"Menu\\Separator",
						L"LightMode_Color",
						MenuHandler::lightMode_SeparatorColor,
						false
					);
		}

		DWORD enableThemeColorization
		{
			RegHelper::GetDword(
				L"Menu\\Separator",
				L"EnableThemeColorization",
				0,
				false
			)
		};

		if (enableThemeColorization)
		{
			RETURN_IF_FAILED(Utils::GetDwmThemeColor(color));
		}

		com_ptr<ID2D1SolidColorBrush> brush{nullptr};
		RETURN_IF_FAILED(
			renderTarget->CreateSolidColorBrush(
				DXHelper::MakeColorF(color),
				&brush
			)
		);
		RETURN_IF_FAILED(renderTarget->BindDC(dc.get(), &paintRect));
		{
			renderTarget->BeginDraw();
			renderTarget->DrawLine(
				D2D1::Point2F(0.f, static_cast<float>(paintRect.bottom - paintRect.top) / 2.f),
				D2D1::Point2F(static_cast<float>(paintRect.right - paintRect.left), static_cast<float>(paintRect.bottom - paintRect.top) / 2.f),
				brush.get(),
				static_cast<float>(separatorWidth) / 1000.f
			);
			renderTarget->EndDraw();
		}

		return S_OK;
	}

	if (partId == MENU_POPUPITEMKBFOCUS)
	{
		DWORD itemDisabled
		{
			RegHelper::GetDword(
				L"Menu\\Focusing",
				L"Disabled",
				0,
				false
			)
		};

		if (itemDisabled)
		{
			return E_NOTIMPL;
		}

		DWORD cornerRadius
		{
			RegHelper::GetDword(
				L"Menu\\Focusing",
				L"CornerRadius",
				MenuHandler::cornerRadius,
				false
			)
		};
		DWORD focusingWidth
		{
			RegHelper::GetDword(
				L"Menu\\Focusing",
				L"Width",
				MenuHandler::focusingWidth,
				false
			)
		};

		if (darkMode)
		{
			color = RegHelper::GetDword(
						L"Menu\\Focusing",
						L"DarkMode_Color",
						MenuHandler::darkMode_FocusingColor,
						false
					);
		}
		else
		{
			color = RegHelper::GetDword(
						L"Menu\\Focusing",
						L"LightMode_Color",
						MenuHandler::lightMode_FocusingColor,
						false
					);
		}

		DWORD enableThemeColorization
		{
			RegHelper::GetDword(
				L"Menu\\Focusing",
				L"EnableThemeColorization",
				0,
				false
			)
		};

		if (enableThemeColorization)
		{
			RETURN_IF_FAILED(Utils::GetDwmThemeColor(color));
		}

		com_ptr<ID2D1SolidColorBrush> brush{nullptr};
		RETURN_IF_FAILED(
			renderTarget->CreateSolidColorBrush(
				DXHelper::MakeColorF(color),
				&brush
			)
		);
		RETURN_IF_FAILED(renderTarget->BindDC(dc.get(), &paintRect));
		{
			renderTarget->BeginDraw();
			renderTarget->DrawRoundedRectangle(
				D2D1::RoundedRect(
					D2D1::RectF(
						0.f, 0.f,
						static_cast<float>(paintRect.right - paintRect.left),
						static_cast<float>(paintRect.bottom - paintRect.top)
					),
					static_cast<float>(cornerRadius),
					static_cast<float>(cornerRadius)
				),
				brush.get(),
				static_cast<float>(focusingWidth) / 1000.f
			);
			renderTarget->EndDraw();
		}

		return S_OK;
	}

	if ((partId == MENU_POPUPITEM || partId == MENU_POPUPITEM_FOCUSABLE))
	{
		if (stateId == MPI_DISABLEDHOT)
		{
			DWORD itemDisabled
			{
				RegHelper::GetDword(
					L"Menu\\DisabledHot",
					L"Disabled",
					0,
					false
				)
			};

			if (itemDisabled)
			{
				return E_NOTIMPL;
			}

			DWORD cornerRadius
			{
				RegHelper::GetDword(
					L"Menu\\DisabledHot",
					L"CornerRadius",
					MenuHandler::cornerRadius,
					false
				)
			};

			if (darkMode)
			{
				color = RegHelper::GetDword(
							L"Menu\\DisabledHot",
							L"DarkMode_Color",
							MenuHandler::darkMode_DisabledHotColor,
							false
						);
			}
			else
			{
				color = RegHelper::GetDword(
							L"Menu\\DisabledHot",
							L"LightMode_Color",
							MenuHandler::lightMode_DisabledHotColor,
							false
						);
			}

			DWORD enableThemeColorization
			{
				RegHelper::GetDword(
					L"Menu\\DisabledHot",
					L"EnableThemeColorization",
					0,
					false
				)
			};

			if (enableThemeColorization)
			{
				RETURN_IF_FAILED(Utils::GetDwmThemeColor(color));
			}

			com_ptr<ID2D1SolidColorBrush> brush{nullptr};
			RETURN_IF_FAILED(
				renderTarget->CreateSolidColorBrush(
					DXHelper::MakeColorF(color),
					&brush
				)
			);
			RETURN_IF_FAILED(renderTarget->BindDC(dc.get(), &paintRect));
			{
				renderTarget->BeginDraw();
				renderTarget->FillRoundedRectangle(
					D2D1::RoundedRect(
						D2D1::RectF(
							0.f, 0.f,
							static_cast<float>(paintRect.right - paintRect.left),
							static_cast<float>(paintRect.bottom - paintRect.top)
						),
						static_cast<float>(cornerRadius),
						static_cast<float>(cornerRadius)
					),
					brush.get()
				);
				renderTarget->EndDraw();
			}

			return S_OK;
		}

		if (stateId == MPI_HOT)
		{
			DWORD itemDisabled
			{
				RegHelper::GetDword(
					L"Menu\\Hot",
					L"Disabled",
					0,
					false
				)
			};

			if (itemDisabled)
			{
				return E_NOTIMPL;
			}

			DWORD cornerRadius
			{
				RegHelper::GetDword(
					L"Menu\\Hot",
					L"CornerRadius",
					MenuHandler::cornerRadius,
					false
				)
			};

			if (darkMode)
			{
				color = RegHelper::GetDword(
							L"Menu\\Hot",
							L"DarkMode_Color",
							MenuHandler::darkMode_HotColor,
							false
				);
			}
			else
			{
				color = RegHelper::GetDword(
							L"Menu\\Hot",
							L"LightMode_Color",
							MenuHandler::lightMode_HotColor,
							false
				);
			}

			DWORD enableThemeColorization
			{
				RegHelper::GetDword(
					L"Menu\\Hot",
					L"EnableThemeColorization",
					0,
					false
				)
			};

			if (enableThemeColorization)
			{
				RETURN_IF_FAILED(Utils::GetDwmThemeColor(color));
			}

			com_ptr<ID2D1SolidColorBrush> brush{nullptr};
			RETURN_IF_FAILED(
				renderTarget->CreateSolidColorBrush(
					DXHelper::MakeColorF(color),
					&brush
				)
			);
			RETURN_IF_FAILED(renderTarget->BindDC(dc.get(), &paintRect));
			{
				renderTarget->BeginDraw();
				renderTarget->FillRoundedRectangle(
					D2D1::RoundedRect(
						D2D1::RectF(
							0.f, 0.f,
							static_cast<float>(paintRect.right - paintRect.left),
							static_cast<float>(paintRect.bottom - paintRect.top)
						),
						static_cast<float>(cornerRadius),
						static_cast<float>(cornerRadius)
					),
					brush.get()
				);
				renderTarget->EndDraw();
			}

			return S_OK;
		}
	}

	return E_NOTIMPL;
}

optional<shared_hbitmap> MenuRendering::PromiseAlpha(HBITMAP bitmap)
{
	if (SUCCEEDED(Utils::PrepareAlpha(bitmap)))
	{
		return nullopt;
	}

	return shared_hbitmap{ThemeHelper::ConvertTo32BPP(bitmap)};
}

HRESULT MenuRendering::BltWithAlpha(
	HDC   hdcDest,
	int   xDest,
	int   yDest,
	int   wDest,
	int   hDest,
	HDC   hdcSrc,
	int   xSrc,
	int   ySrc,
	int   wSrc,
	int   hSrc
)
{
	RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdcDest);
	RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdcSrc);
	HBITMAP hBitmap{reinterpret_cast<HBITMAP>(GetCurrentObject(hdcSrc, OBJ_BITMAP))};
	RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hBitmap);
	RETURN_HR_IF_EXPECTED(
		E_NOTIMPL,
		ThemeHelper::IsOemBitmap(hBitmap)
	);

	auto bitmap{PromiseAlpha(hBitmap)};
	RETURN_LAST_ERROR_IF(bitmap && !bitmap.value().get());
	auto selectedObject
	{
		(bitmap ? wil::SelectObject(hdcSrc, bitmap.value().get()) : optional<unique_select_object>{nullopt})
	};
	RETURN_IF_WIN32_BOOL_FALSE(
		GdiAlphaBlend(
			hdcDest,
			xDest, yDest,
			wDest, hDest,
			hdcSrc,
			xSrc, ySrc,
			wSrc, hSrc,
			{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA}
		)
	);

	return S_OK;
}