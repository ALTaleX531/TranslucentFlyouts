#include "pch.h"
#include "Utils.hpp"
#include "ThemeHelper.hpp"
#include "DXHelper.hpp"
#include "MenuRendering.hpp"

using namespace TranslucentFlyouts;

bool MenuRendering::HandlePopupMenuNonClientBorderColors(HDC hdc, const RECT& paintRect)
{
	// Border color is enabled.
	if (!MenuHandler::g_menuContext.border.colorUseNone)
	{
		if (MenuHandler::g_menuContext.border.colorUseDefault)
		{
			return false;
		}

		wil::unique_hbrush brush{ Utils::CreateSolidColorBrushWithAlpha(Utils::MakeCOLORREF(MenuHandler::g_menuContext.border.color), Utils::GetAlphaFromARGB(MenuHandler::g_menuContext.border.color)) };
		if (brush)
		{
			FrameRect(hdc, &paintRect, brush.get());
		}
	}
	else
	{
		FrameRect(hdc, &paintRect, GetStockBrush(BLACK_BRUSH));
	}

	return true;
}

bool MenuRendering::HandleCustomRendering(HDC hdc, int partId, int stateId, const RECT& clipRect, const RECT& paintRect)
{
	if (!DXHelper::LazyD2D::EnsureInitialized())
	{
		return false;
	}

	auto& lazyD2D{ DXHelper::LazyD2D::GetInstance() };
	auto renderTarget{ lazyD2D.GetRenderTarget() };

	Utils::unique_ext_hdc dc{ hdc };
	IntersectClipRect(dc.get(), paintRect.left, paintRect.top, paintRect.right, paintRect.bottom);
	PatBlt(dc.get(), paintRect.left, paintRect.top, paintRect.right - paintRect.left, paintRect.bottom - paintRect.top, BLACKNESS);

	if (partId == MENU_POPUPSEPARATOR)
	{
		if (MenuHandler::g_menuContext.customRendering.separator_disabled)
		{
			return false;
		}

		try
		{
			wil::com_ptr<ID2D1SolidColorBrush> brush{ nullptr };
			THROW_IF_FAILED(
				renderTarget->CreateSolidColorBrush(
					DXHelper::MakeColorF(MenuHandler::g_menuContext.customRendering.separator_color),
					&brush
				)
			);
			THROW_IF_FAILED(renderTarget->BindDC(dc.get(), &paintRect));
			{
				renderTarget->BeginDraw();
				renderTarget->DrawLine(
					D2D1::Point2F(0.f, static_cast<float>(wil::rect_height(paintRect)) / 2.f),
					D2D1::Point2F(static_cast<float>(wil::rect_width(paintRect)), static_cast<float>(wil::rect_height(paintRect)) / 2.f),
					brush.get(),
					static_cast<float>(MenuHandler::g_menuContext.customRendering.separator_width) / 1000.f
				);
				THROW_IF_FAILED(renderTarget->EndDraw());
			}
		}
		catch (...)
		{
			LOG_CAUGHT_EXCEPTION();
			return false;
		}

		return true;
	}

	if (partId == MENU_POPUPITEMKBFOCUS)
	{
		if (MenuHandler::g_menuContext.customRendering.focusing_disabled)
		{
			return false;
		}

		try
		{
			wil::com_ptr<ID2D1SolidColorBrush> brush{ nullptr };
			THROW_IF_FAILED(
				renderTarget->CreateSolidColorBrush(
					DXHelper::MakeColorF(MenuHandler::g_menuContext.customRendering.focusing_color),
					&brush
				)
			);
			THROW_IF_FAILED(renderTarget->BindDC(dc.get(), &paintRect));
			{
				renderTarget->BeginDraw();
				renderTarget->DrawRoundedRectangle(
					D2D1::RoundedRect(
						D2D1::RectF(
							0.f, 0.f,
							static_cast<float>(wil::rect_width(paintRect)),
							static_cast<float>(wil::rect_height(paintRect))
						),
						static_cast<float>(MenuHandler::g_menuContext.customRendering.focusing_cornerRadius),
						static_cast<float>(MenuHandler::g_menuContext.customRendering.focusing_cornerRadius)
					),
					brush.get(),
					static_cast<float>(MenuHandler::g_menuContext.customRendering.focusing_width) / 1000.f
				);
				THROW_IF_FAILED(renderTarget->EndDraw());
			}
		}
		catch (...)
		{
			LOG_CAUGHT_EXCEPTION();
			return false;
		}

		return true;
	}

	if ((partId == MENU_POPUPITEM || partId == MENU_POPUPITEM_FOCUSABLE))
	{
		if (stateId == MPI_DISABLEDHOT)
		{
			if (MenuHandler::g_menuContext.customRendering.disabledHot_disabled)
			{
				return false;
			}

			try
			{
				wil::com_ptr<ID2D1SolidColorBrush> brush{ nullptr };
				THROW_IF_FAILED(
					renderTarget->CreateSolidColorBrush(
						DXHelper::MakeColorF(MenuHandler::g_menuContext.customRendering.disabledHot_color),
						&brush
					)
				);
				THROW_IF_FAILED(renderTarget->BindDC(dc.get(), &paintRect));
				{
					renderTarget->BeginDraw();
					renderTarget->FillRoundedRectangle(
						D2D1::RoundedRect(
							D2D1::RectF(
								0.f, 0.f,
								static_cast<float>(wil::rect_width(paintRect)),
								static_cast<float>(wil::rect_height(paintRect))
							),
							static_cast<float>(MenuHandler::g_menuContext.customRendering.disabledHot_cornerRadius),
							static_cast<float>(MenuHandler::g_menuContext.customRendering.disabledHot_cornerRadius)
						),
						brush.get()
					);
					THROW_IF_FAILED(renderTarget->EndDraw());
				}
			}
			catch (...)
			{
				LOG_CAUGHT_EXCEPTION();
				return false;
			}

			return true;
		}

		if (stateId == MPI_HOT)
		{
			if (MenuHandler::g_menuContext.customRendering.hot_disabled)
			{
				return false;
			}

			try
			{
				wil::com_ptr<ID2D1SolidColorBrush> brush{ nullptr };
				THROW_IF_FAILED(
					renderTarget->CreateSolidColorBrush(
						DXHelper::MakeColorF(MenuHandler::g_menuContext.customRendering.hot_color),
						&brush
					)
				);
				THROW_IF_FAILED(renderTarget->BindDC(dc.get(), &paintRect));
				{
					renderTarget->BeginDraw();
					renderTarget->FillRoundedRectangle(
						D2D1::RoundedRect(
							D2D1::RectF(
								0.f, 0.f,
								static_cast<float>(wil::rect_width(paintRect)),
								static_cast<float>(wil::rect_height(paintRect))
							),
							static_cast<float>(MenuHandler::g_menuContext.customRendering.hot_cornerRadius),
							static_cast<float>(MenuHandler::g_menuContext.customRendering.hot_cornerRadius)
						),
						brush.get()
					);
					THROW_IF_FAILED(renderTarget->EndDraw());
				}
			}
			catch (...)
			{
				LOG_CAUGHT_EXCEPTION();
				return false;
			}

			return true;
		}
	}

	return false;
}

bool MenuRendering::HandleMenuBitmap(HBITMAP& source, wil::unique_hbitmap& target)
{
	if (source && !ThemeHelper::IsOemBitmap(source))
	{
		target.reset(Utils::Promise32BPP(source));
		if (!target)
		{
			return false;
		}
		if (MenuHandler::g_menuContext.iconBackgroundColorRemoval.enable)
		{
			Utils::BitmapApplyEffect(
				target.get(),
				{
					std::make_shared<Utils::SpriteEffect>(
						MenuHandler::g_menuContext.iconBackgroundColorRemoval.colorTreatAsTransparent,
						MenuHandler::g_menuContext.iconBackgroundColorRemoval.colorTreatAsTransparentThreshold
					)
				}
			);
		}

		source = target.get();
		return true;
	}

	return false;
}

bool MenuRendering::HandleDrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect,
	decltype(&DrawThemeBackground) actualDrawThemeBackground
)
{
	if (IsRectEmpty(pRect) == TRUE)
	{
		return false;
	}

	WCHAR themeClassName[MAX_PATH + 1]{};
	if (FAILED(ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)))
	{
		return false;
	}

	if (!_wcsicmp(themeClassName, L"Menu"))
	{
		RECT clipRect{ *pRect };
		if (pClipRect)
		{
			IntersectRect(&clipRect, &clipRect, pClipRect);
		}

		// Separator
		if (iPartId == MENU_POPUPSEPARATOR)
		{
			if (MenuHandler::g_menuContext.customRendering.enable)
			{
				if (MenuRendering::HandleCustomRendering(hdc, iPartId, iStateId, clipRect, *pRect))
				{
					return true;
				}
			}
		}
		// Focusing
		if (iPartId == MENU_POPUPITEMKBFOCUS)
		{
			if (MenuHandler::g_menuContext.customRendering.enable)
			{
				if (MenuRendering::HandleCustomRendering(hdc, iPartId, iStateId, clipRect, *pRect))
				{
					return true;
				}
			}
		}

		if ((iPartId == MENU_POPUPITEM || iPartId == MENU_POPUPITEM_FOCUSABLE))
		{
			if (iStateId == MPI_DISABLEDHOT)
			{
				if (MenuHandler::g_menuContext.customRendering.enable)
				{
					if (MenuRendering::HandleCustomRendering(hdc, iPartId, iStateId, clipRect, *pRect))
					{
						return true;
					}
				}
			}
			if (iStateId == MPI_HOT)
			{
				if (MenuHandler::g_menuContext.customRendering.enable)
				{
					if (MenuRendering::HandleCustomRendering(hdc, iPartId, iStateId, clipRect, *pRect))
					{
						return true;
					}
				}

				if (actualDrawThemeBackground)
				{
					actualDrawThemeBackground(
						hTheme,
						hdc,
						iPartId,
						iStateId,
						pRect,
						pClipRect
					);
				}
				return true;
			}
		}

		if (
			iPartId != MENU_POPUPBACKGROUND &&
			iPartId != MENU_POPUPBORDERS &&
			iPartId != MENU_POPUPGUTTER &&
			iPartId != MENU_POPUPITEM &&
			iPartId != MENU_POPUPITEM_FOCUSABLE
		)
		{
			return false;
		}

		PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS);
		return true;
	}

	return false;
}