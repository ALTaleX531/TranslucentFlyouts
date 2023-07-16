#include "pch.h"
#include "DXHelper.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuRendering.hpp"

TranslucentFlyouts::MenuRendering& TranslucentFlyouts::MenuRendering::GetInstance()
{
	static MenuRendering instance{};
	return instance;
}

HRESULT TranslucentFlyouts::MenuRendering::DoCustomThemeRendering(HDC hdc, bool darkMode, int partId, int stateId, const RECT& clipRect, const RECT& paintRect)
{
	if (!DXHelper::LazyD2D::EnsureInitialized())
	{
		return E_FAIL;
	}

	auto& lazyD2D{DXHelper::LazyD2D::GetInstance()};
	auto renderTarget{lazyD2D.GetRenderTarget()};

	COLORREF color{0};
	DWORD opacity{0};

	Utils::unique_ext_hdc dc{hdc};
	IntersectClipRect(dc.get(), paintRect.left, paintRect.top, paintRect.right, paintRect.bottom);
	RETURN_IF_WIN32_BOOL_FALSE(
		PatBlt(dc.get(), clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS)
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
				L"SeparatorWidth",
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
			opacity = RegHelper::GetDword(
						  L"Menu\\Separator",
						  L"DarkMode_Opacity",
						  MenuHandler::darkMode_SeparatorOpacity,
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
			opacity = RegHelper::GetDword(
						  L"Menu\\Separator",
						  L"LightMode_Opacity",
						  MenuHandler::lightMode_SeparatorOpacity,
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
			Utils::GetDwmThemeColor(color, opacity);
		}

		wil::com_ptr<ID2D1SolidColorBrush> brush{nullptr};
		RETURN_IF_FAILED(
			renderTarget->CreateSolidColorBrush(
				DXHelper::COLORREF2ColorF(color, static_cast<std::byte>(opacity)),
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
				static_cast<float>(separatorWidth)
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
				L"FocusingWidth",
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
			opacity = RegHelper::GetDword(
						  L"Menu\\Focusing",
						  L"DarkMode_Opacity",
						  MenuHandler::darkMode_FocusingOpacity,
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
			opacity = RegHelper::GetDword(
						  L"Menu\\Focusing",
						  L"LightMode_Opacity",
						  MenuHandler::lightMode_FocusingOpacity,
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
			Utils::GetDwmThemeColor(color, opacity);
		}

		wil::com_ptr<ID2D1SolidColorBrush> brush{nullptr};
		RETURN_IF_FAILED(
			renderTarget->CreateSolidColorBrush(
				DXHelper::COLORREF2ColorF(color, static_cast<std::byte>(opacity)),
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
				static_cast<float>(focusingWidth)
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
				opacity = RegHelper::GetDword(
							  L"Menu\\DisabledHot",
							  L"DarkMode_Opacity",
							  MenuHandler::darkMode_DisabledHotOpacity,
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
				opacity = RegHelper::GetDword(
							  L"Menu\\DisabledHot",
							  L"LightMode_Opacity",
							  MenuHandler::lightMode_DisabledHotOpacity,
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
				Utils::GetDwmThemeColor(color, opacity);
			}

			wil::com_ptr<ID2D1SolidColorBrush> brush{nullptr};
			RETURN_IF_FAILED(
				renderTarget->CreateSolidColorBrush(
					DXHelper::COLORREF2ColorF(color, static_cast<std::byte>(opacity)),
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
				opacity = RegHelper::GetDword(
							  L"Menu\\Hot",
							  L"DarkMode_Opacity",
							  MenuHandler::darkMode_HotOpacity,
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
				opacity = RegHelper::GetDword(
							  L"Menu\\Hot",
							  L"LightMode_Opacity",
							  MenuHandler::lightMode_HotOpacity,
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
				Utils::GetDwmThemeColor(color, opacity);
			}

			wil::com_ptr<ID2D1SolidColorBrush> brush{nullptr};
			RETURN_IF_FAILED(
				renderTarget->CreateSolidColorBrush(
					DXHelper::COLORREF2ColorF(color, static_cast<std::byte>(opacity)),
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

	if (partId == MENU_POPUPBORDERS)
	{
		DWORD enableThemeColorization
		{
			RegHelper::GetDword(
				L"Menu\\Border",
				L"EnableThemeColorization",
				0,
				false
			)
		};

		RETURN_HR_IF(E_NOTIMPL, !enableThemeColorization);
		RETURN_IF_FAILED(Utils::GetDwmThemeColor(color, opacity));
		wil::unique_hbrush brush{Utils::CreateSolidColorBrushWithAlpha(color, static_cast<std::byte>(opacity))};
		RETURN_LAST_ERROR_IF_NULL(brush);
		RETURN_LAST_ERROR_IF(FrameRect(dc.get(), &clipRect, brush.get()) == 0);

		return S_OK;
	}

	return E_NOTIMPL;
}

std::optional<wil::shared_hbitmap> TranslucentFlyouts::MenuRendering::PromiseAlpha(HBITMAP bitmap)
{
	if (SUCCEEDED(Utils::PrepareAlpha(bitmap)))
	{
		return std::nullopt;
	}

	return wil::shared_hbitmap{ThemeHelper::ConvertTo32BPP(bitmap)};
}

HRESULT TranslucentFlyouts::MenuRendering::BltWithAlpha(
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
	RETURN_LAST_ERROR_IF_NULL(hBitmap);
	RETURN_HR_IF_EXPECTED(
		E_NOTIMPL,
		ThemeHelper::IsOemBitmap(hBitmap)
	);

	auto bitmap{PromiseAlpha(hBitmap)};
	RETURN_LAST_ERROR_IF(bitmap && !bitmap.value().get());
	auto selectedObject
	{
		(bitmap ? wil::SelectObject(hdcSrc, bitmap.value().get()) : std::optional<wil::unique_select_object>{std::nullopt})
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