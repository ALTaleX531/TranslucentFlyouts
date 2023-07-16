#include "pch.h"
#include "ThemeHelper.hpp"
#include "MenuAnimation.hpp"

namespace TranslucentFlyouts::MenuAnimation
{
	using namespace std;

	struct AnimationInfo
	{
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

		virtual void Animator(ULONGLONG currentTimeStamp) {}

		HWND window{nullptr};
		chrono::milliseconds duration{0};
		ULONGLONG startTimeStamp{GetTickCount64()};
		ULONGLONG endTimeStamp{startTimeStamp + duration.count()};
	};

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

		wil::srwlock m_lock{};
		DWORD m_threadId{};
		vector<shared_ptr<AnimationInfo>> m_animationStorage{};
	};

	class FadeOut : public AnimationInfo
	{
	public:
		FadeOut(HWND hWnd, MENUBARINFO mbi, chrono::milliseconds animationDuration) try : AnimationInfo(animationDuration)
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
									 WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,                          // Spy++
									 L"Static",
									 nullptr,
									 WS_POPUP,
									 destination.x,
									 destination.x,
									 size.cx,
									 size.cy,
									 Utils::GetCurrentMenuOwner(),
									 nullptr,
									 nullptr,
									 nullptr
			);
			THROW_LAST_ERROR_IF_NULL(window);

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
			LOG_CAUGHT_EXCEPTION();
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
}

DWORD WINAPI TranslucentFlyouts::MenuAnimation::AnimationWorker::ThreadProc(LPVOID lpThreadParameter)
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

void TranslucentFlyouts::MenuAnimation::AnimationWorker::Schedule(shared_ptr<AnimationInfo> animationInfo)
{
	auto cleanUp{m_lock.lock_exclusive()};
	m_animationStorage.push_back(animationInfo);

	// We have no animation worker thread, create one
	if (m_threadId == 0)
	{
		// Add ref count of current dll, in case it unloads while animation worker thread is still busy...
		HMODULE moduleHandle{nullptr};
		LOG_IF_WIN32_BOOL_FALSE(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(HINST_THISCOMPONENT), &moduleHandle));
		wil::unique_handle threadHandle{CreateThread(nullptr, 0, ThreadProc, this, 0, &m_threadId)};
	}
}

void TranslucentFlyouts::MenuAnimation::FadeOut::Animator(ULONGLONG currentTimeStamp)
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

HRESULT TranslucentFlyouts::MenuAnimation::CreateFadeOut(
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
	return wil::ResultFromCaughtException();
}
