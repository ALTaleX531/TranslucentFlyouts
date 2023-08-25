#include "pch.h"
#include "TFMain.hpp"
#include "DXHelper.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuAnimation.hpp"
#include "DwmThumbnailAPI.hpp"
#include "EffectHelper.hpp"
#include "RegHelper.hpp"
#include "DXHelper.hpp"

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
    FadeOut(HWND hWnd, MENUBARINFO mbi, chrono::milliseconds animationDuration) try :
        AnimationInfo{animationDuration}
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
                     -GetSystemMetrics(SM_CXVIRTUALSCREEN),
                     -GetSystemMetrics(SM_CYVIRTUALSCREEN),
                     0,
                     0,
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
        THROW_IF_FAILED(
            DwmFlush()
        );
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
    bool							m_reverse{false};
    bool							m_started{false};
    bool							m_useSysDropShadow{false};
    float							m_cornerRadius{0.f};// 0.f, 4.f, 8.f
    float							m_ratio{0.f};
    DWORD							m_animationStyle{0};
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

        THROW_HR_IF_NULL(
            E_INVALIDARG,
            m_dcompTopTarget
        );
        THROW_HR_IF_NULL(
            E_INVALIDARG,
            m_dcompBottomTarget
        );
        THROW_HR_IF_NULL(
            E_INVALIDARG,
            m_thumbnailVisual
        );
        THROW_HR_IF_NULL(
            E_INVALIDARG,
            m_backdropThumbnailVisual
        );

        THROW_HR_IF(
            E_FAIL,
            m_started
        );

        m_reverse = reverse;

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
                DWM_TNP_VISIBLE | DWM_TNP_RECTSOURCE | DWM_TNP_RECTDESTINATION | DWM_TNP_SOURCECLIENTAREAONLY | DwmThumbnailAPI::DWM_TNP_ENABLE3D,
                {0, 0, size.cx, size.cy},
                {0, 0, size.cx, size.cy},
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
                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING
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

            // Synchronizing WAM and DirectComposition time such that when WAM Update is called,
            // the value reflects the DirectComposition value at the given time.
            DCOMPOSITION_FRAME_STATISTICS frameStatistics{};
            THROW_IF_FAILED(
                dcompDevice->GetFrameStatistics(&frameStatistics)
            );
            UI_ANIMATION_SECONDS timeNow{static_cast<double>(frameStatistics.nextEstimatedFrameTime.QuadPart) / static_cast<double>(frameStatistics.timeFrequency.QuadPart)};

            auto MakeAnimation = [&](std::function<com_ptr<IUIAnimationTransition2>(com_ptr<IUIAnimationVariable2> variable)> callback)
            {
                if (callback)
                {
                    com_ptr<IUIAnimationVariable2> variable{nullptr};
                    com_ptr<IUIAnimationStoryboard2> storyboard{nullptr};
                    THROW_IF_FAILED(
                        manager->CreateAnimationVariable(0., &variable)
                    );
                    THROW_IF_FAILED(
                        manager->CreateStoryboard(&storyboard)
                    );
                    com_ptr<IUIAnimationTransition2> transition{callback(variable)};
                    THROW_IF_FAILED(
                        storyboard->AddTransition(variable.get(), transition.get())
                    );
                    THROW_IF_FAILED(
                        manager->Update(timeNow)
                    );
                    THROW_IF_FAILED(
                        storyboard->Schedule(timeNow)
                    );

                    com_ptr<IDCompositionAnimation> dcompAnimation{nullptr};
                    THROW_IF_FAILED(
                        dcompDevice->CreateAnimation(&dcompAnimation)
                    );
                    THROW_IF_FAILED(
                        variable->GetCurve(dcompAnimation.get())
                    );

                    return dcompAnimation;
                }

                return com_ptr<IDCompositionAnimation> {nullptr};
            };

            float width
            {
                static_cast<float>(windowRect.right - windowRect.left)
            };
            float height
            {
                static_cast<float>(windowRect.bottom - windowRect.top)
            };

            if (m_animationStyle == 0)
            {
                constexpr double endValue{0.};
                double beginValue{0.};

                if (reverse)
                {
                    beginValue = static_cast<double>(size.cy) * (1. - static_cast<double>(m_ratio));
                }
                else
                {
                    beginValue = static_cast<double>(size.cy) * (-1. + static_cast<double>(m_ratio));
                }

                com_ptr<IDCompositionAnimation> dcompAnimation
                {
                    MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                    {
                        com_ptr<IUIAnimationTransition2> transition{nullptr};
                        THROW_IF_FAILED(
                            library->CreateCubicBezierLinearTransition(static_cast<double>(m_popupInDuration.count()) / 1000., endValue, 0, 0, 0, 1, &transition)
                        );
                        THROW_IF_FAILED(
                            transition->SetInitialValue(beginValue)
                        );

                        return transition;
                    })
                };

                THROW_IF_FAILED(m_thumbnailVisual->SetOffsetX(0.f));
                THROW_IF_FAILED(m_thumbnailVisual->SetOffsetY(dcompAnimation.get()));

                THROW_IF_FAILED(m_backdropThumbnailVisual->SetOffsetX(0.f));
                THROW_IF_FAILED(m_backdropThumbnailVisual->SetOffsetY(dcompAnimation.get()));
            }

            if (m_animationStyle == 1)
            {
                POINT clientPoint{m_cursorPos};
                THROW_IF_WIN32_BOOL_FALSE(ScreenToClient(m_menuWindow, &clientPoint));

                D2D1_POINT_2F startPoint
                {
                    static_cast<float>(min(max(0., static_cast<double>(clientPoint.x)), width)),
                    static_cast<float>(
                        abs(height - static_cast<double>(clientPoint.y)) >
                        abs(0. - static_cast<double>(clientPoint.y))
                        ? 0. : height
                    )
                };
                double maxRadius{sqrt(width * width + height * height)};

                com_ptr<IDCompositionRectangleClip> clip{nullptr};
                THROW_IF_FAILED(dcompDevice->CreateRectangleClip(&clip));
                THROW_IF_FAILED(
                    clip->SetTop(
                        MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                {
                    com_ptr<IUIAnimationTransition2> transition{nullptr};
                    THROW_IF_FAILED(
                        library->CreateCubicBezierLinearTransition(
                            static_cast<double>(m_popupInDuration.count()) / 1000.,
                            static_cast<double>(startPoint.y) - static_cast<double>(maxRadius),
                            0, 0, 0, 1, &transition
                        )
                    );
                    THROW_IF_FAILED(
                        transition->SetInitialValue(static_cast<double>(startPoint.y) - static_cast<double>(maxRadius) * m_ratio)
                    );

                    return transition;
                }).get()
                    )
                );
                THROW_IF_FAILED(
                    clip->SetBottom(
                        MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                {
                    com_ptr<IUIAnimationTransition2> transition{nullptr};
                    THROW_IF_FAILED(
                        library->CreateCubicBezierLinearTransition(
                            static_cast<double>(m_popupInDuration.count()) / 1000.,
                            static_cast<double>(startPoint.y) + static_cast<double>(maxRadius),
                            0, 0, 0, 1, &transition
                        )
                    );
                    THROW_IF_FAILED(
                        transition->SetInitialValue(static_cast<double>(startPoint.y) + static_cast<double>(maxRadius) * m_ratio)
                    );

                    return transition;
                }).get()
                    )
                );
                THROW_IF_FAILED(
                    clip->SetLeft(
                        MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                {
                    com_ptr<IUIAnimationTransition2> transition{nullptr};
                    THROW_IF_FAILED(
                        library->CreateCubicBezierLinearTransition(
                            static_cast<double>(m_popupInDuration.count()) / 1000.,
                            static_cast<double>(startPoint.x) - static_cast<double>(maxRadius),
                            0, 0, 0, 1, &transition
                        )
                    );
                    THROW_IF_FAILED(
                        transition->SetInitialValue(static_cast<double>(startPoint.x) - static_cast<double>(maxRadius) * m_ratio)
                    );

                    return transition;
                }).get()
                    )
                );
                THROW_IF_FAILED(
                    clip->SetRight(
                        MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                {
                    com_ptr<IUIAnimationTransition2> transition{nullptr};
                    THROW_IF_FAILED(
                        library->CreateCubicBezierLinearTransition(
                            static_cast<double>(m_popupInDuration.count()) / 1000.,
                            static_cast<double>(startPoint.x) + static_cast<double>(maxRadius),
                            0, 0, 0, 1, &transition
                        )
                    );
                    THROW_IF_FAILED(
                        transition->SetInitialValue(static_cast<double>(startPoint.x) + static_cast<double>(maxRadius) * m_ratio)
                    );

                    return transition;
                }).get()
                    )
                );

                auto dcompAnimation
                {
                    MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                    {
                        com_ptr<IUIAnimationTransition2> transition{nullptr};
                        THROW_IF_FAILED(
                            library->CreateCubicBezierLinearTransition(
                                static_cast<double>(m_popupInDuration.count()) / 1000.,
                                static_cast<double>(maxRadius),
                                0, 0, 0, 1, &transition
                            )
                        );
                        THROW_IF_FAILED(
                            transition->SetInitialValue(0.)
                        );

                        return transition;
                    })
                };
                THROW_IF_FAILED(clip->SetBottomLeftRadiusX(dcompAnimation.get()));
                THROW_IF_FAILED(clip->SetBottomLeftRadiusY(dcompAnimation.get()));
                THROW_IF_FAILED(clip->SetBottomRightRadiusX(dcompAnimation.get()));
                THROW_IF_FAILED(clip->SetBottomRightRadiusY(dcompAnimation.get()));
                THROW_IF_FAILED(clip->SetTopLeftRadiusX(dcompAnimation.get()));
                THROW_IF_FAILED(clip->SetTopLeftRadiusY(dcompAnimation.get()));
                THROW_IF_FAILED(clip->SetTopRightRadiusX(dcompAnimation.get()));
                THROW_IF_FAILED(clip->SetTopRightRadiusY(dcompAnimation.get()));

                THROW_IF_FAILED(m_thumbnailVisual->SetClip(clip.get()));
                THROW_IF_FAILED(m_backdropThumbnailVisual->SetClip(clip.get()));
            }

            if (m_animationStyle == 2)
            {
                POINT clientPoint{m_cursorPos};
                THROW_IF_WIN32_BOOL_FALSE(ScreenToClient(m_menuWindow, &clientPoint));

                D2D1_POINT_2F startPoint
                {
                    abs(width - static_cast<float>(clientPoint.x)) >
                    abs(0.f - static_cast<float>(clientPoint.x))
                    ? 0.f : width,
                    abs(height - static_cast<float>(clientPoint.y)) >
                    abs(0.f - static_cast<float>(clientPoint.y))
                    ? 0.f : height
                };

                com_ptr<IDCompositionRectangleClip> clip{nullptr};
                THROW_IF_FAILED(dcompDevice->CreateRectangleClip(&clip));

                THROW_IF_FAILED(
                    clip->SetTop(
                        MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                {
                    com_ptr<IUIAnimationTransition2> transition{nullptr};
                    THROW_IF_FAILED(
                        library->CreateSmoothStopTransition(
                            static_cast<double>(m_popupInDuration.count()) / 1000.,
                            static_cast<double>(startPoint.y) - static_cast<double>(height),
                            &transition
                        )
                    );
                    THROW_IF_FAILED(
                        transition->SetInitialValue(static_cast<double>(startPoint.y) - static_cast<double>(height) * m_ratio)
                    );

                    return transition;
                }).get()
                    )
                );
                THROW_IF_FAILED(
                    clip->SetBottom(
                        MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                {
                    com_ptr<IUIAnimationTransition2> transition{nullptr};
                    THROW_IF_FAILED(
                        library->CreateSmoothStopTransition(
                            static_cast<double>(m_popupInDuration.count()) / 1000.,
                            static_cast<double>(startPoint.y) + static_cast<double>(height),
                            &transition
                        )
                    );
                    THROW_IF_FAILED(
                        transition->SetInitialValue(static_cast<double>(startPoint.y) + static_cast<double>(height) * m_ratio)
                    );

                    return transition;
                }).get()
                    )
                );
                THROW_IF_FAILED(
                    clip->SetLeft(
                        MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                {
                    com_ptr<IUIAnimationTransition2> transition{nullptr};
                    THROW_IF_FAILED(
                        library->CreateSmoothStopTransition(
                            static_cast<double>(m_popupInDuration.count()) / 1000.,
                            static_cast<double>(startPoint.x) - static_cast<double>(width),
                            &transition
                        )
                    );
                    THROW_IF_FAILED(
                        transition->SetInitialValue(static_cast<double>(startPoint.x) - static_cast<double>(width) * m_ratio)
                    );

                    return transition;
                }).get()
                    )
                );
                THROW_IF_FAILED(
                    clip->SetRight(
                        MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                {
                    com_ptr<IUIAnimationTransition2> transition{nullptr};
                    THROW_IF_FAILED(
                        library->CreateSmoothStopTransition(
                            static_cast<double>(m_popupInDuration.count()) / 1000.,
                            static_cast<double>(startPoint.x) + static_cast<double>(width),
                            &transition
                        )
                    );
                    THROW_IF_FAILED(
                        transition->SetInitialValue(static_cast<double>(startPoint.x) + static_cast<double>(width) * m_ratio)
                    );

                    return transition;
                }).get()
                    )
                );

                THROW_IF_FAILED(clip->SetBottomLeftRadiusX(m_cornerRadius));
                THROW_IF_FAILED(clip->SetBottomLeftRadiusY(m_cornerRadius));
                THROW_IF_FAILED(clip->SetBottomRightRadiusX(m_cornerRadius));
                THROW_IF_FAILED(clip->SetBottomRightRadiusY(m_cornerRadius));
                THROW_IF_FAILED(clip->SetTopLeftRadiusX(m_cornerRadius));
                THROW_IF_FAILED(clip->SetTopLeftRadiusY(m_cornerRadius));
                THROW_IF_FAILED(clip->SetTopRightRadiusX(m_cornerRadius));
                THROW_IF_FAILED(clip->SetTopRightRadiusY(m_cornerRadius));

                THROW_IF_FAILED(m_thumbnailVisual->SetClip(clip.get()));
                THROW_IF_FAILED(m_backdropThumbnailVisual->SetClip(clip.get()));

                com_ptr<IDCompositionAnimation> dcompAnimation
                {
                    MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                    {
                        com_ptr<IUIAnimationTransition2> transition{nullptr};
                        THROW_IF_FAILED(
                            library->CreateCubicBezierLinearTransition(static_cast<double>(m_popupInDuration.count()) / 1000., 0., 0, 0, 0, 1, &transition)
                        );
                        THROW_IF_FAILED(
                            transition->SetInitialValue(
                                static_cast<double>(-width) * (1. - static_cast<double>(m_ratio)) +
                                static_cast<double>(startPoint.x)
                            )
                        );

                        return transition;
                    })
                };

                THROW_IF_FAILED(m_thumbnailVisual->SetOffsetX(dcompAnimation.get()));
            }

            if (m_animationStyle == 3)
            {
                com_ptr<IDCompositionAnimation> dcompAnimation
                {
                    MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                    {
                        com_ptr<IUIAnimationTransition2> transition{nullptr};
                        THROW_IF_FAILED(
                            library->CreateCubicBezierLinearTransition(static_cast<double>(m_popupInDuration.count()) / 1000., 1., 0, 0, 0, 1, &transition)
                        );
                        THROW_IF_FAILED(
                            transition->SetInitialValue(
                                0.85
                            )
                        );

                        return transition;
                    })
                };

                com_ptr<IDCompositionScaleTransform3D> transform{nullptr};
                THROW_IF_FAILED(
                    dcompDevice->CreateScaleTransform3D(&transform)
                );

                THROW_IF_FAILED(
                    transform->SetCenterX(static_cast<float>(width / 2.))
                );
                THROW_IF_FAILED(
                    transform->SetCenterY(static_cast<float>(width / 2.))
                );
                THROW_IF_FAILED(
                    transform->SetScaleX(dcompAnimation.get())
                );
                THROW_IF_FAILED(
                    transform->SetScaleY(dcompAnimation.get())
                );

                THROW_IF_FAILED(m_thumbnailVisual.query<IDCompositionVisual3>()->SetTransform(transform.get()));
                THROW_IF_FAILED(m_backdropThumbnailVisual.query<IDCompositionVisual3>()->SetTransform(transform.get()));
            }

            {
                com_ptr<IDCompositionAnimation> dcompAnimation
                {
                    MakeAnimation([&](com_ptr<IUIAnimationVariable2> variable)
                    {
                        com_ptr<IUIAnimationTransition2> transition{nullptr};
                        THROW_IF_FAILED(
                            library->CreateLinearTransition(static_cast<double>(m_fadeInDuration.count()) / 1000., 1., &transition)
                        );
                        THROW_IF_FAILED(
                            transition->SetInitialValue(0.)
                        );

                        return transition;
                    })
                };

                THROW_IF_FAILED(m_thumbnailVisual.query<IDCompositionVisual3>()->SetOpacity(dcompAnimation.get()));
                THROW_IF_FAILED(m_backdropThumbnailVisual.query<IDCompositionVisual3>()->SetOpacity(dcompAnimation.get()));
            }
        }

        THROW_IF_FAILED(
            dcompDevice->Commit()
        );

        if (m_useSysDropShadow)
        {
            SetClassLongPtr(window, GCL_STYLE, GetClassLongPtr(window, GCL_STYLE) | CS_DROPSHADOW);
        }
        THROW_IF_WIN32_BOOL_FALSE(
            SetWindowPos(
                window, nullptr,
                windowRect.left, windowRect.top, size.cx, size.cy,
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING
            )
        );
        Utils::CloakWindow(window, FALSE);
        if (m_useSysDropShadow)
        {
            SetClassLongPtr(window, GCL_STYLE, GetClassLongPtr(window, GCL_STYLE) & ~CS_DROPSHADOW);
        }

        Restart(m_totDuration);

        m_started = true;
    }
    catch (...)
    {
        Finish();
        LOG_CAUGHT_EXCEPTION();
        return;
    }

    void Finish()
    {
        Utils::CloakWindow(m_menuWindow, FALSE);
        Utils::CloakWindow(window, TRUE);
        // Wait for DWM
        DwmFlush();
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
                LRESULT lr{DefSubclassProc(hWnd, uMsg, wParam, lParam)};

                HWND backdropWindow{GetWindow(hWnd, GW_HWNDNEXT)};
                if (Utils::IsWindowClass(backdropWindow, L"SysShadow") && IsWindowVisible(backdropWindow))
                {
                    popupInAnimation.m_useSysDropShadow = true;
                }

                popupInAnimation.Start(
                    abs(popupInAnimation.m_cursorPos.y - windowPos.y) >
                    abs(popupInAnimation.m_cursorPos.y - (windowPos.y + windowPos.cy))
                    ? true : false
                );

                return lr;
            }

            if (windowPos.flags & SWP_HIDEWINDOW)
            {
                // If the animation is still running, then we need to stop it right now!
                // Interupt the running animation
                popupInAnimation.SetDuration(0ms);
                popupInAnimation.Detach();
            }
        }

        if (uMsg == MenuHandler::MN_SELECTITEM)
        {
            auto position{static_cast<int>(wParam)};
            if (position != -1)
            {
                if (
                    RegHelper::GetDword(
                        L"Menu\\Animation",
                        L"EnableImmediateInterupting",
                        0,
                        false
                    )
                )
                {
                    // If the animation is still running, then we need to stop it right now!
                    // Interupt the running animation
                    popupInAnimation.SetDuration(0ms);
                }
            }
        }

        if (uMsg == WM_DESTROY)
        {
            // If the animation is still running, then we need to stop it right now!
            // Interupt the running animation
            popupInAnimation.SetDuration(0ms);
            popupInAnimation.Detach();
        }

        // ~PopupIn() sends WM_MAFINISHED to the SubclassProc
        if (uMsg == WM_MAFINISHED)
        {
            popupInAnimation.Detach();
        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
public:
#pragma push_macro("max")
#undef max
    PopupIn(HWND hWnd, float startPosRatio, std::chrono::milliseconds popupInDuration, std::chrono::milliseconds fadeInDuration, DWORD animationStyle) try :
        AnimationInfo {chrono::milliseconds::max()}, m_totDuration {max(popupInDuration, fadeInDuration)}, m_menuWindow {hWnd}, m_fadeInDuration {fadeInDuration}, m_popupInDuration {popupInDuration}, m_ratio {startPosRatio}, m_animationStyle {animationStyle}
#pragma pop_macro("max")
    {
        THROW_HR_IF(
            E_FAIL,
            !DXHelper::LazyDComposition::EnsureInitialized()
        );

        Attach();

        Utils::CloakWindow(m_menuWindow, TRUE);

        m_backdropWindow = CreateWindowExW(
            WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"Static",
            L"PopupIn Animation Backdrop",
            WS_POPUP,
            -GetSystemMetrics(SM_CXVIRTUALSCREEN),
            -GetSystemMetrics(SM_CYVIRTUALSCREEN),
            0,
            0,
            Utils::GetCurrentMenuOwner(),
            nullptr,
            nullptr,
            nullptr
        );
        THROW_LAST_ERROR_IF_NULL(m_backdropWindow);

        window = CreateWindowExW(
            WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"Static",
            L"PopupIn Animation",
            WS_POPUP,
            -GetSystemMetrics(SM_CXVIRTUALSCREEN),
            -GetSystemMetrics(SM_CYVIRTUALSCREEN),
            1,
            1,
            Utils::GetCurrentMenuOwner(),
            nullptr,
            nullptr,
            nullptr
        );
        THROW_LAST_ERROR_IF_NULL(window);

        auto info{MenuHandler::GetMenuRenderingInfo(m_menuWindow)};
        if (info.useUxTheme)
        {
            DWORD cornerType
            {
                RegHelper::GetDword(
                    L"Menu",
                    L"CornerType",
                    3
                )
            };

            // We are on Windows 10 or the corner is not round
            if (
                FAILED(TFMain::ApplyRoundCorners(L"Menu"sv, window)) ||
                cornerType == 1
            )
            {
                m_cornerRadius = 0.f;

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
            else
            {
                switch (cornerType)
                {
                case 1:
                {
                    m_cornerRadius = 0.f;
                    break;
                }
                case 2:
                {
                    m_cornerRadius = 8.f;
                    break;
                }
                case 0:
                case 3:
                {
                    m_cornerRadius = 4.f;
                    break;
                }
                default:
                    break;
                }
            }

            TFMain::ApplySysBorderColors(L"Menu"sv, window, info.useDarkMode, info.borderColor, info.borderColor);
            EffectHelper::EnableWindowDarkMode(window, info.useDarkMode);

            TFMain::ApplyBackdropEffect(L"Menu"sv, m_backdropWindow, info.useDarkMode, TFMain::darkMode_GradientColor, TFMain::lightMode_GradientColor);
            TFMain::ApplyRoundCorners(L"Menu"sv, m_backdropWindow);
            COLORREF color{DWMWA_COLOR_NONE};
            DwmSetWindowAttribute(m_backdropWindow, DWMWA_BORDER_COLOR, &color, sizeof(color));
            EffectHelper::EnableWindowDarkMode(m_backdropWindow, info.useDarkMode);
        }
        else
        {
            m_cornerRadius = 0.f;
        }

        THROW_IF_WIN32_BOOL_FALSE(SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA));
        THROW_IF_WIN32_BOOL_FALSE(SetLayeredWindowAttributes(m_backdropWindow, 0, 0, LWA_ALPHA));
        Utils::CloakWindow(window, TRUE);
        THROW_IF_WIN32_BOOL_FALSE(
            SetWindowPos(m_backdropWindow, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING)
        );

        THROW_IF_WIN32_BOOL_FALSE(
            GetCursorPos(&m_cursorPos)
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

        if (m_started)
        {
            Start(m_reverse);
        }

        THROW_IF_FAILED(
            dcompDevice->Commit()
        );
        THROW_IF_FAILED(
            DwmFlush()
        );
    }
    catch (...)
    {
        Finish();
        if (m_menuWindow)
        {
            // DO NOT USE SendMessage HERE OTHERWISE IT WILL CAUSE DEAD LOCK
            SendNotifyMessageW(m_menuWindow, WM_MAFINISHED, 0, 0);
            m_menuWindow = nullptr;
        }
        if (window)
        {
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
        if (m_backdropThumbnail)
        {
            DwmUnregisterThumbnail(m_backdropThumbnail);
            m_backdropThumbnail = nullptr;
        }
    }
    virtual ~PopupIn() noexcept
    {
        Finish();
        if (m_menuWindow)
        {
            // DO NOT USE SendMessage HERE OTHERWISE IT WILL CAUSE DEAD LOCK
            SendNotifyMessageW(m_menuWindow, WM_MAFINISHED, 0, 0);
            m_menuWindow = nullptr;
        }
        if (window)
        {
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
        if (m_backdropThumbnail)
        {
            DwmUnregisterThumbnail(m_backdropThumbnail);
            m_backdropThumbnail = nullptr;
        }
    }

    void Animator(ULONGLONG currentTimeStamp) override;
};
}

DWORD WINAPI MenuAnimation::AnimationWorker::ThreadProc(LPVOID lpThreadParameter)
{
    auto cleanUp{get_module_reference_for_thread()};
    auto& animationWorker{*reinterpret_cast<AnimationWorker*>(lpThreadParameter)};

    // We can use it to reuse thread
    constexpr ULONGLONG maxDelayExitingTicks{1000};
    ULONGLONG delayExitingStartTimeStamp{0};
    ULONGLONG delayExitingTicks{0};

    vector<shared_ptr<AnimationInfo>> animationStorage{};
    while (true)
    {
        {
            auto cleanUp{ animationWorker.m_lock.lock_exclusive() };
            animationStorage.insert(animationStorage.end(), animationWorker.m_animationStorage.begin(), animationWorker.m_animationStorage.end());
            animationWorker.m_animationStorage.clear();
        }

        {
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

    LOG_IF_WIN32_BOOL_FALSE(
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
        )
    );
}

void MenuAnimation::PopupIn::Animator(ULONGLONG currentTimeStamp) try
{
    if (m_started)
    {
        THROW_HR_IF(E_INVALIDARG, !IsWindow(window));
        THROW_HR_IF(E_INVALIDARG, !IsWindow(m_menuWindow));
        THROW_HR_IF(E_INVALIDARG, !IsWindow(m_backdropWindow));
        THROW_HR_IF_NULL(E_INVALIDARG, window);
        THROW_HR_IF_NULL(E_INVALIDARG, m_menuWindow);
        THROW_HR_IF_NULL(E_INVALIDARG, m_backdropWindow);

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
                DWM_TNP_VISIBLE | DWM_TNP_RECTSOURCE | DWM_TNP_RECTDESTINATION | DWM_TNP_SOURCECLIENTAREAONLY | DwmThumbnailAPI::DWM_TNP_ENABLE3D,
                {0, 0, size.cx, size.cy},
                {0, 0, size.cx, size.cy},
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
                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING
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

        THROW_IF_WIN32_BOOL_FALSE(
            SetWindowPos(
                window, nullptr,
                windowRect.left, windowRect.top, size.cx, size.cy,
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING
            )
        );
    }
}
CATCH_LOG_RETURN()

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
    std::chrono::milliseconds fadeInDuration,
    DWORD animationStyle
) try
{
    auto& animationWorker{AnimationWorker::GetInstance()};
    animationWorker.Schedule(make_shared<PopupIn>(hWnd, startPosRatio, popupInDuration, fadeInDuration, animationStyle));

    return S_OK;
}
catch (...)
{
    return ResultFromCaughtException();
}