#include "pch.h"
#include "DXHelper.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuAnimation.hpp"
#include "DwmThumbnailAPI.hpp"
#include "EffectHelper.hpp"
#include "RegHelper.hpp"

using namespace std;
using namespace wil;
using namespace TranslucentFlyouts;

namespace TranslucentFlyouts::MenuAnimation
{
	struct AnimationInfo
	{
		static UINT WM_MAFINISHED;

		AnimationInfo() = default;
		AnimationInfo(chrono::milliseconds animationDuration) : duration{animationDuration}, endTimeStamp{startTimeStamp + duration.count()}
		{
		}
		virtual ~AnimationInfo() noexcept = default;

		void SetDuration(chrono::milliseconds animationDuration)
		{
			duration = animationDuration;
			endTimeStamp = startTimeStamp + duration.count();
		}
		void Restart(chrono::milliseconds animationDuration)
		{
			startTimeStamp = GetTickCount64(); 
			duration = animationDuration;
			endTimeStamp = startTimeStamp + duration.count();
		}

		virtual void Animator(ULONGLONG currentTimeStamp) {}

		HWND window{nullptr};
		chrono::milliseconds duration{0};
		ULONGLONG startTimeStamp{GetTickCount64()};
		ULONGLONG endTimeStamp{startTimeStamp + duration.count()};
	};
	UINT AnimationInfo::WM_MAFINISHED{RegisterWindowMessageW(L"TranslucentFlyouts.MenuAnimation.Finished")};

	class AnimationWorker
	{
	public:
		static AnimationWorker& GetInstance()
		{
			static AnimationWorker instance{};
			return instance;
		};
		AnimationWorker(const AnimationWorker&) = delete;
		~AnimationWorker() noexcept = default;
		AnimationWorker& operator=(const AnimationWorker&) = delete;

		void Schedule(shared_ptr<AnimationInfo> animationInfo);
	private:
		static DWORD WINAPI ThreadProc(LPVOID lpThreadParameter);
		AnimationWorker() = default;

		srwlock m_lock{};
		DWORD m_threadId{};
		vector<shared_ptr<AnimationInfo>> m_animationStorage{};
	};

	class FadeOut : public AnimationInfo
	{
	public:
		FadeOut(HWND hWnd, MENUBARINFO mbi, chrono::milliseconds animationDuration) try : AnimationInfo{animationDuration}
		{
			HRESULT hr{S_OK};
			POINT source{0, 0};
			SIZE size{mbi.rcBar.right - mbi.rcBar.left, mbi.rcBar.bottom - mbi.rcBar.top};
			POINT destination
			{
				mbi.rcBar.left,
				mbi.rcBar.top
			};
			RECT paintRect
			{
				destination.x, destination.y,
				destination.x + size.cx, destination.y + size.cy
			};

			RECT windowRect{};
			THROW_IF_WIN32_BOOL_FALSE(GetWindowRect(hWnd, &windowRect));

			window = CreateWindowExW(
						 WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
						 L"Static",
						 L"FadeOut Animation",
						 WS_POPUP,
						 0,
						 0,
						 0,
						 0,
						 Utils::GetCurrentMenuOwner(),
						 nullptr,
						 nullptr,
						 nullptr
					 );
			THROW_LAST_ERROR_IF_NULL(window);
			if (GetClassLongPtr(window, GCL_STYLE) & (CS_DROPSHADOW | CS_SAVEBITS))
			{
				SetClassLongPtr(window, GCL_STYLE, GetClassLongPtr(window, GCL_STYLE) &~(CS_DROPSHADOW | CS_SAVEBITS));
			}

			auto menuDC{wil::GetWindowDC(hWnd)};
			THROW_LAST_ERROR_IF_NULL(menuDC);
			auto screenDC{wil::GetDC(nullptr)};
			THROW_LAST_ERROR_IF_NULL(screenDC);

			auto f = [&](HDC memoryDC, HPAINTBUFFER, RGBQUAD*, int)
			{
				THROW_IF_WIN32_BOOL_FALSE(
					BitBlt(
						memoryDC,
						paintRect.left,
						paintRect.top,
						size.cx,
						size.cy,
						menuDC.get(),
						destination.x - windowRect.left,
						destination.y - windowRect.top,
						SRCCOPY
					)
				);

				BLENDFUNCTION blendFunction = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
				THROW_IF_WIN32_BOOL_FALSE(
					UpdateLayeredWindow(
						window,
						screenDC.get(),
						&destination,
						&size,
						memoryDC,
						&source,
						0,
						&blendFunction,
						ULW_ALPHA
					)
				);

				ShowWindow(window, SW_SHOWNA);
			};

			// BeginBufferedPaint: It's not just for buffered painting any more!
			// https://devblogs.microsoft.com/oldnewthing/20110520-00/?p=10613
			hr = ThemeHelper::DrawThemeContent(
					 screenDC.get(),
					 paintRect,
					 nullptr,
					 nullptr,
					 BPPF_NOCLIP,
					 f,
					 std::byte{0},
					 false,
					 false
				 );
			THROW_IF_FAILED(hr);
		}
		catch (...)
		{
			if (window)
			{
				DestroyWindow(window);
				window = nullptr;
			}
		}
		~FadeOut() noexcept
		{
			if (window)
			{
				SendNotifyMessageW(window, WM_CLOSE, 0, 0);
				window = nullptr;
			}
		}

		void Animator(ULONGLONG currentTimeStamp) override;
	};

	class PopupIn : public AnimationInfo
	{
	private:
		bool							m_started{false};
		float							m_ratio{0.f};
		POINT							m_cursorPos{};

		HWND							m_backdropWindow{nullptr};
		HWND							m_menuWindow{nullptr};

		chrono::milliseconds			m_fadeInDuration{0ms};
		chrono::milliseconds			m_popupInDuration{0ms};
		chrono::milliseconds			m_totDuration{0ms};

		com_ptr<IDCompositionTarget>	m_dcompTopTarget{nullptr};
		com_ptr<IDCompositionTarget>	m_dcompBottomTarget{nullptr};

		HTHUMBNAIL						m_thumbnail{nullptr};
		com_ptr<IDCompositionVisual2>	m_thumbnailVisual{nullptr};

		HTHUMBNAIL						m_backdropThumbnail{nullptr};
		com_ptr<IDCompositionVisual2>	m_backdropThumbnailVisual{nullptr};

		void Start(bool reverse) try
		{
			THROW_HR_IF(
				E_FAIL,
				!DXHelper::LazyDComposition::EnsureInitialized()
			);

			THROW_HR_IF(
				E_FAIL,
				m_started
			);

			RECT windowRect{};
			THROW_IF_WIN32_BOOL_FALSE(
				GetWindowRect(m_menuWindow, &windowRect)
			);

			SIZE size{};
			if (m_thumbnail)
			{
				THROW_IF_FAILED(
					DwmThumbnailAPI::g_actualDwmpQueryWindowThumbnailSourceSize(
						m_menuWindow,
						FALSE,
						&size
					)
				);
				DWM_THUMBNAIL_PROPERTIES thumbnailProperties
				{
					DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DWM_TNP_SOURCECLIENTAREAONLY | DwmThumbnailAPI::DWM_TNP_ENABLE3D,
					{0, 0, size.cx, size.cy},
					{},
					255,
					TRUE,
					TRUE
				};
				THROW_IF_FAILED(DwmUpdateThumbnailProperties(m_thumbnail, &thumbnailProperties));
			}
			if (m_backdropThumbnail)
			{
				THROW_IF_WIN32_BOOL_FALSE(
					SetWindowPos(
						m_backdropWindow, window,
						windowRect.left, windowRect.top, size.cx, size.cy,
						SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER
					)
				);
				DWM_THUMBNAIL_PROPERTIES thumbnailProperties
				{
					DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DwmThumbnailAPI::DWM_TNP_ENABLE3D,
					{0, 0, size.cx, size.cy},
					{},
					255,
					TRUE,
					FALSE
				};
				THROW_IF_FAILED(DwmUpdateThumbnailProperties(m_backdropThumbnail, &thumbnailProperties));
			}

			auto dcompDevice{DXHelper::LazyDComposition::GetInstance().GetDCompositionDevice()};
			com_ptr<IDCompositionAnimation> dcompAnimation{nullptr};
			THROW_IF_FAILED(
				dcompDevice->CreateAnimation(&dcompAnimation)
			);

			auto cleanUp{Utils::RoInit()};
			{
				com_ptr<IUIAnimationManager2> manager
				{
					wil::CoCreateInstance<IUIAnimationManager2>(CLSID_UIAnimationManager2)
				};
				com_ptr<IUIAnimationTransitionLibrary2> library
				{
					wil::CoCreateInstance<IUIAnimationTransitionLibrary2>(CLSID_UIAnimationTransitionLibrary2)
				};
				
				constexpr float endValue{0.f};
				float beginValue{0.f};

				if (reverse)
				{
					beginValue = float(size.cy) - float(size.cy) * m_ratio;
				}
				else
				{
					beginValue = -float(size.cy) + float(size.cy) * m_ratio;
				}

				// Synchronizing WAM and DirectComposition time such that when WAM Update is called, 
				// the value reflects the DirectComposition value at the given time.
				DCOMPOSITION_FRAME_STATISTICS frameStatistics{};
				THROW_IF_FAILED(
					dcompDevice->GetFrameStatistics(&frameStatistics)
				);
				UI_ANIMATION_SECONDS timeNow{static_cast<double>(frameStatistics.nextEstimatedFrameTime.QuadPart) / static_cast<double>(frameStatistics.timeFrequency.QuadPart)};

				{
					com_ptr<IUIAnimationVariable2> variable{nullptr};
					com_ptr<IUIAnimationStoryboard2> storyboard{nullptr};
					com_ptr<IUIAnimationTransition2> transition{nullptr};
					THROW_IF_FAILED(
						manager->CreateAnimationVariable(beginValue, &variable)
					);
					THROW_IF_FAILED(
						manager->CreateStoryboard(&storyboard)
					);
					THROW_IF_FAILED(
						library->CreateCubicBezierLinearTransition(static_cast<float>(m_popupInDuration.count()) / 1000.f, endValue, 0, 0, 0, 1, &transition)
					);
					THROW_IF_FAILED(
						storyboard->AddTransition(variable.get(), transition.get())
					);
					THROW_IF_FAILED(
						manager->Update(timeNow)
					);
					THROW_IF_FAILED(
						storyboard->Schedule(timeNow)
					);
					THROW_IF_FAILED(
						variable->GetCurve(dcompAnimation.get())
					);

					THROW_IF_FAILED(m_thumbnailVisual->SetOffsetX(0.f));
					THROW_IF_FAILED(m_thumbnailVisual->SetOffsetY(dcompAnimation.get()));

					THROW_IF_FAILED(m_backdropThumbnailVisual->SetOffsetX(0.f));
					THROW_IF_FAILED(m_backdropThumbnailVisual->SetOffsetY(dcompAnimation.get()));
				}

				{
					com_ptr<IUIAnimationVariable2> variable{nullptr};
					com_ptr<IUIAnimationStoryboard2> storyboard{nullptr};
					com_ptr<IUIAnimationTransition2> transition{nullptr};
					THROW_IF_FAILED(
						manager->CreateAnimationVariable(0.f, &variable)
					);
					THROW_IF_FAILED(
						manager->CreateStoryboard(&storyboard)
					);
					THROW_IF_FAILED(
						library->CreateLinearTransition(static_cast<float>(m_fadeInDuration.count()) / 1000.f, 1.f, &transition)
					);
					THROW_IF_FAILED(
						storyboard->AddTransition(variable.get(), transition.get())
					);
					THROW_IF_FAILED(
						manager->Update(timeNow)
					);
					THROW_IF_FAILED(
						storyboard->Schedule(timeNow)
					);
					THROW_IF_FAILED(
						variable->GetCurve(dcompAnimation.get())
					);

					THROW_IF_FAILED(m_thumbnailVisual.query<IDCompositionVisual3>()->SetOpacity(dcompAnimation.get()));
					THROW_IF_FAILED(m_backdropThumbnailVisual.query<IDCompositionVisual3>()->SetOpacity(dcompAnimation.get()));
				}
			}

			THROW_IF_FAILED(
				dcompDevice->Commit()
			);

			THROW_IF_WIN32_BOOL_FALSE(
				SetWindowPos(
					window, nullptr,
					windowRect.left, windowRect.top, size.cx, size.cy,
					SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
				)
			);
			Utils::CloakWindow(window, FALSE);
			
			Restart(m_totDuration);

			m_started = true;
		}
		CATCH_LOG_RETURN()

		// The animation is still running, but we need to stop it right now!
		void Abort()
		{
			Utils::CloakWindow(m_menuWindow, FALSE);
			Utils::CloakWindow(window, TRUE);
			// Wait for DWM
			DwmFlush();
#pragma push_macro("max")
#undef max
			// Interupt the animation
			SetDuration(0ms);
#pragma pop_macro("max")
		}

		void Attach()
		{
			THROW_HR_IF(
				E_FAIL,
				!SetWindowSubclass(m_menuWindow, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this))
			);
		}
		void Detach()
		{
			if (m_menuWindow)
			{
				if (RemoveWindowSubclass(m_menuWindow, SubclassProc, 0))
				{
					m_menuWindow = nullptr;
				}
			}
		}

		static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
		{
			PopupIn& popupInAnimation{*reinterpret_cast<PopupIn*>(dwRefData)};
			
			if (uMsg == WM_WINDOWPOSCHANGED)
			{
				const auto& windowPos{*reinterpret_cast<WINDOWPOS*>(lParam)};
				if (windowPos.flags & SWP_SHOWWINDOW)
				{
					popupInAnimation.Start(
						abs(popupInAnimation.m_cursorPos.y - windowPos.y) >
						abs(popupInAnimation.m_cursorPos.y - (windowPos.y + windowPos.cy))
						? true : false
					);
				}

				if (windowPos.flags & SWP_HIDEWINDOW)
				{
					popupInAnimation.Abort();
				}
			}

			if (uMsg == WM_NCDESTROY)
			{
				popupInAnimation.Abort();
			}

			// ~PopupIn() sends WM_MAFINISHED to the SubclassProc
			if (uMsg == WM_MAFINISHED)
			{
				popupInAnimation.Abort();
				popupInAnimation.Detach();
			}

			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}
	public:
#pragma push_macro("max")
#undef max
		PopupIn(HWND hWnd, float startPosRatio, std::chrono::milliseconds popupInDuration, std::chrono::milliseconds fadeInDuration) try : AnimationInfo{chrono::milliseconds::max()}, m_totDuration{max(popupInDuration, fadeInDuration)}, m_menuWindow{hWnd}, m_fadeInDuration{fadeInDuration}, m_popupInDuration{popupInDuration}, m_ratio{startPosRatio}
#pragma pop_macro("max")
		{
			THROW_HR_IF(
				E_FAIL,
				!DXHelper::LazyDComposition::EnsureInitialized()
			);

			Attach();

			Utils::CloakWindow(hWnd, TRUE);

			window = CreateWindowExW(
						 WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_PALETTEWINDOW,
						 L"Static",
						 L"PopupIn Animation",
						 WS_POPUP,
						 0,
						 0,
						 1,
						 1,
						 Utils::GetCurrentMenuOwner(),
						 nullptr,
						 nullptr,
						 nullptr
					 );
			THROW_LAST_ERROR_IF_NULL(window);

			m_backdropWindow = CreateWindowExW(
						 WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
						 L"Static",
						 L"PopupIn Animation Backdrop",
						 WS_POPUP,
						 0,
						 0,
						 1,
						 1,
						 Utils::GetCurrentMenuOwner(),
						 nullptr,
						 nullptr,
						 nullptr
			);
			THROW_LAST_ERROR_IF_NULL(m_backdropWindow);

			auto& menuHandler{MenuHandler::GetInstance()};
			auto info{menuHandler.GetMenuRenderingInfo(m_menuWindow)};
			if (info.useUxTheme)
			{
				// We are on Windows 10 or the corner is not round
				if (
					FAILED(menuHandler.HandleRoundCorners(L"Menu"sv, window)) || 
					RegHelper::GetDword(
						L"Menu",
						L"CornerType",
						3
					) == 1
				)
				{
					SetClassLongPtr(window, GCL_STYLE, GetClassLongPtr(window, GCL_STYLE) | (CS_DROPSHADOW | CS_SAVEBITS));
					
					DWORD effectType
					{
						RegHelper::GetDword(
							L"Menu",
							L"EffectType",
							static_cast<DWORD>(EffectHelper::EffectType::ModernAcrylicBlur)
						)
					};
					if (
						RegHelper::GetDword(
							L"Menu",
							L"EnableDropShadow",
							0
						) &&
						(effectType == 4 || effectType == 5)
					)
					{
						EffectHelper::SetWindowBackdrop(window, TRUE, 0, 4);
					}
				}

				menuHandler.HandleSysBorderColors(L"Menu"sv, window, info.useDarkMode, info.borderColor);
				EffectHelper::EnableWindowDarkMode(window, info.useDarkMode);
				
				menuHandler.ApplyEffect(L"Menu"sv, m_backdropWindow, info.useDarkMode);
				EffectHelper::EnableWindowDarkMode(m_backdropWindow, info.useDarkMode);
			}
			else
			{
				// Windows 2000 style
				SetClassLongPtr(window, GCL_STYLE, GetClassLongPtr(window, GCL_STYLE) | (CS_DROPSHADOW | CS_SAVEBITS));
			}

			THROW_IF_WIN32_BOOL_FALSE(SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA));
			THROW_IF_WIN32_BOOL_FALSE(SetLayeredWindowAttributes(m_backdropWindow, 0, 0, LWA_ALPHA));
			THROW_IF_WIN32_BOOL_FALSE(
				SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW)
			);
			Utils::CloakWindow(window, TRUE);
			THROW_IF_WIN32_BOOL_FALSE(
				SetWindowPos(m_backdropWindow, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW)
			);

			auto dcompDevice{DXHelper::LazyDComposition::GetInstance().GetDCompositionDevice()};

			THROW_IF_FAILED(
				dcompDevice->CreateTargetForHwnd(window, TRUE, &m_dcompTopTarget)
			);
			THROW_IF_FAILED(
				dcompDevice->CreateTargetForHwnd(window, FALSE, &m_dcompBottomTarget)
			);

			DWM_THUMBNAIL_PROPERTIES thumbnailProperties
			{
				DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DWM_TNP_SOURCECLIENTAREAONLY | DwmThumbnailAPI::DWM_TNP_ENABLE3D,
				{},
				{},
				255,
				TRUE,
				TRUE
			};
			THROW_IF_FAILED(
				DwmThumbnailAPI::g_actualDwmpCreateSharedThumbnailVisual(
					window,
					m_menuWindow,
					0,
					&thumbnailProperties,
					dcompDevice.get(),
					m_thumbnailVisual.put_void(),
					&m_thumbnail
				)
			);
			thumbnailProperties.fSourceClientAreaOnly = FALSE;
			// Create backdrop thumbnail
			THROW_IF_FAILED(
				DwmThumbnailAPI::g_actualDwmpCreateSharedThumbnailVisual(
					window,
					m_backdropWindow,
					0,
					&thumbnailProperties,
					dcompDevice.get(),
					m_backdropThumbnailVisual.put_void(),
					&m_backdropThumbnail
				)
			);

			THROW_IF_FAILED(
				m_dcompTopTarget->SetRoot(m_thumbnailVisual.get())
			);
			THROW_IF_FAILED(
				m_dcompBottomTarget->SetRoot(m_backdropThumbnailVisual.get())
			);

			THROW_IF_FAILED(
				dcompDevice->Commit()
			);

			THROW_IF_WIN32_BOOL_FALSE(
				GetCursorPos(&m_cursorPos)
			);
		}
		catch (...)
		{
			if (m_menuWindow)
			{
				SendMessageW(m_menuWindow, WM_MAFINISHED, 0, 0);
			}
			if (window)
			{
				if (GetClassLongPtr(window, GCL_STYLE) & (CS_DROPSHADOW | CS_SAVEBITS))
				{
					SetClassLongPtr(window, GCL_STYLE, GetClassLongPtr(window, GCL_STYLE) & ~(CS_DROPSHADOW | CS_SAVEBITS));
				}
				DestroyWindow(window);
				window = nullptr;
			}
			if (m_backdropWindow)
			{
				DestroyWindow(m_backdropWindow);
				m_backdropWindow = nullptr;
			}
			if (m_thumbnail)
			{
				DwmUnregisterThumbnail(m_thumbnail);
				m_thumbnail = nullptr;
			}
		}

		~PopupIn() noexcept
		{
			if (m_menuWindow)
			{
				SendMessageW(m_menuWindow, WM_MAFINISHED, 0, 0);
			}
			if (window)
			{
				if (GetClassLongPtr(window, GCL_STYLE) & (CS_DROPSHADOW | CS_SAVEBITS))
				{
					SetClassLongPtr(window, GCL_STYLE, GetClassLongPtr(window, GCL_STYLE) & ~(CS_DROPSHADOW | CS_SAVEBITS));
				}
				SendNotifyMessageW(window, WM_CLOSE, 0, 0);
				window = nullptr;
			}
			if (m_backdropWindow)
			{
				SendNotifyMessageW(m_backdropWindow, WM_CLOSE, 0, 0);
				m_backdropWindow = nullptr;
			}
			if (m_thumbnail)
			{
				DwmUnregisterThumbnail(m_thumbnail);
				m_thumbnail = nullptr;
			}
		}

		void Animator(ULONGLONG currentTimeStamp) override;
	};
}

DWORD WINAPI MenuAnimation::AnimationWorker::ThreadProc(LPVOID lpThreadParameter)
{
	auto& animationWorker{*reinterpret_cast<AnimationWorker*>(lpThreadParameter)};

	// We can use it to reuse thread
	constexpr ULONGLONG maxDelayExitingTicks{1000};
	ULONGLONG delayExitingStartTimeStamp{0};
	ULONGLONG delayExitingTicks{0};

	while (true)
	{
		{
			auto cleanUp{animationWorker.m_lock.lock_exclusive()};
			auto& animationStorage{animationWorker.m_animationStorage};

			auto currentTimeStamp{GetTickCount64()};
			delayExitingTicks = currentTimeStamp - delayExitingStartTimeStamp;

			// No animation tasks left, now leaving
			if (animationStorage.empty())
			{
				if (delayExitingTicks >= maxDelayExitingTicks)
				{
					animationWorker.m_threadId = 0;
					break;
				}
			}
			else
			{
				delayExitingStartTimeStamp = currentTimeStamp;

				// Now it is time to process animation...
				for (auto it = animationStorage.begin(); it != animationStorage.end();)
				{
					auto animationInfo{*it};
					currentTimeStamp = GetTickCount64();

					if (currentTimeStamp <= animationInfo->endTimeStamp)
					{
						animationInfo->Animator(currentTimeStamp);
						it++;
					}
					else
					{
						// Animation already completed, erase it
						it = animationStorage.erase(it);
					}
				}
			}

		}
		// We have completed several animation tasks, let's wait for the next batch!
		if (FAILED(DwmFlush()))
		{
			Sleep(10);
		}
	}

	FreeLibraryAndExitThread(HINST_THISCOMPONENT, 0);
	// We will never get here...
	return 0;
}

void MenuAnimation::AnimationWorker::Schedule(shared_ptr<AnimationInfo> animationInfo)
{
	auto cleanUp{m_lock.lock_exclusive()};
	m_animationStorage.push_back(animationInfo);

	// We have no animation worker thread, create one
	if (m_threadId == 0)
	{
		// Add ref count of current dll, in case it unloads while animation worker thread is still busy...
		HMODULE moduleHandle{nullptr};
		LOG_IF_WIN32_BOOL_FALSE(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(HINST_THISCOMPONENT), &moduleHandle));
		unique_handle threadHandle{CreateThread(nullptr, 0, ThreadProc, this, 0, &m_threadId)};
	}
}

void MenuAnimation::FadeOut::Animator(ULONGLONG currentTimeStamp)
{
	BLENDFUNCTION blendFunction
	{
		AC_SRC_OVER,
		0,
		static_cast<BYTE>(
			255 - static_cast<int>(
				round(
					(
						static_cast<float>((currentTimeStamp - startTimeStamp)) /
						static_cast<float>(duration.count())
					) * 255.f
				)
			)
		),
		AC_SRC_ALPHA
	};

	UpdateLayeredWindow(
		window,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		0,
		&blendFunction,
		ULW_ALPHA
	);
}

void MenuAnimation::PopupIn::Animator(ULONGLONG currentTimeStamp)
{

}

HRESULT MenuAnimation::CreateFadeOut(
	HWND hWnd,
	MENUBARINFO mbi,
	std::chrono::milliseconds duration
) try
{
	auto& animationWorker{AnimationWorker::GetInstance()};
	animationWorker.Schedule(make_shared<FadeOut>(hWnd, mbi, duration));

	return S_OK;
}
catch (...)
{
	return ResultFromCaughtException();
}

HRESULT MenuAnimation::CreatePopupIn(
	HWND hWnd,
	float startPosRatio,
	std::chrono::milliseconds popupInDuration,
	std::chrono::milliseconds fadeInDuration
) try
{
	auto& animationWorker{AnimationWorker::GetInstance()};
	animationWorker.Schedule(make_shared<PopupIn>(hWnd, startPosRatio, popupInDuration, fadeInDuration));

	return S_OK;
}
catch (...)
{
	return ResultFromCaughtException();
}