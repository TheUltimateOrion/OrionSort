#include "core/app.h"

#include "core/logging/logging.h"
#include "core/platform/display.h"
#include "sort/exchange/bubble.h"
#include "utils/common.h"

#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <chrono>
#include <ratio>

#include <cstdio>

using namespace std::literals::chrono_literals;

namespace Core
{
    App::~App()
    {
        LOG_INFO("Joining and Destroying Threads");

        Utils::terminateThread(m_sortThread);
        Utils::terminateThread(m_audioThread);

        if (m_imguiInitialized)
        {
            LOG_INFO("Shutting down ImGui renderer");
            ImGui_ImplSDLRenderer3_Shutdown();

            LOG_INFO("Shutting down ImGui");
            ImGui_ImplSDL3_Shutdown();

            LOG_INFO("Destroying ImGui context");
            ImGui::DestroyContext();
        }

        Renderer::RenderContext::DestroyContext(m_ctx);

        LOG_INFO("Deinitializing SDL_ttf");
        TTF_Quit();

        LOG_INFO("Quitting...");
        SDL_Quit();
    }

    void App::setImGuiStyle() const noexcept
    {
        constexpr bool              kAntiAliasedFill            = true;
        constexpr bool              kAntiAliasedLines           = true;
        constexpr bool              kAntiAliasedLinesUseTexture = true;

        constexpr ImVec2            kButtonTextAlign            = ImVec2(0.50f, 0.50f);

        constexpr ImVec2            kCellPadding                = ImVec2(4.00f, 2.00f);
        constexpr float             kChildBorderSize            = 1.00f;
        constexpr float             kChildRounding              = 10.00f;
        constexpr float             kCircleTessellationMaxError = 0.30f;
        constexpr ImGuiDir          kColorsButtonPosition       = ImGuiDir::ImGuiDir_Right;
        constexpr float             kCurveTessellationTolerance = 1.25f;

        constexpr float             kDisabledAlpha              = 0.60f;
        constexpr ImVec2            kDisplaySafeAreaPadding     = ImVec2(3.00f, 3.00f);
        constexpr ImVec2            kDisplayWindowPadding       = ImVec2(30.00f, 30.00f);
        constexpr float             kDockingSeparatorSize       = 3.00f;

        constexpr float             kFontScaleMain              = 1.00f;
        constexpr float             kFontSizeBase               = 13.00f;
        constexpr float             kFrameBorderSize            = 0.00f;
        constexpr ImVec2            kFramePadding               = ImVec2(5.00f, 5.00f);
        constexpr float             kFrameRounding              = 4.00f;

        constexpr float             kGlobalAlpha                = 1.00f;
        constexpr float             kGrabMinimumSize            = 5.00f;
        constexpr float             kGrabRounding               = 3.00f;

        constexpr ImGuiHoveredFlags kHoverFlagsForTooltipMouse  = ImGuiHoveredFlags_DelayShort
                                                               | ImGuiHoveredFlags_Stationary;

        constexpr ImGuiHoveredFlags kHoverFlagsForTooltipNav = ImGuiHoveredFlags_DelayNormal
                                                             | ImGuiHoveredFlags_NoSharedDelay;

        constexpr float    kImageBorderSize                  = 0.00f;
        constexpr float    kIndentSpacing                    = 25.00f;
        constexpr ImVec2   kItemInnerSpacing                 = ImVec2(4.00f, 4.00f);
        constexpr ImVec2   kItemSpacing                      = ImVec2(8.00f, 4.00f);

        constexpr float    kLogSliderDeadzone                = 4.00f;

        constexpr float    kPopupBorderSize                  = 1.0f;
        constexpr float    kPopupRounding                    = 0.0f;

        constexpr float    kScrollbarSize                    = 15.00f;
        constexpr float    kScrollbarRounding                = 12.0f;
        constexpr ImVec2   kSelectableTextAlign              = ImVec2(0.00f, 0.00f);
        constexpr ImVec2   kSeparatorTextAlign               = ImVec2(0.00f, 0.50f);
        constexpr float    kSeparatorTextBorderSize          = 3.00f;
        constexpr ImVec2   kSeparatorTextPadding             = ImVec2(20.00f, 3.00f);

        constexpr float    kTabBarBorderSize                 = 1.00f;
        constexpr float    kTabBorderSize                    = 0.00f;
        constexpr float    kTabBarOverlineSize               = 1.00f;
        constexpr float    kTabCloseButtonMinWidthSelected   = -1.00f;
        constexpr float    kTabCloseButtonMinWidthUnselected = 0.00f;
        constexpr float    kTabMinWidthBase                  = 1.00f;
        constexpr float    kTabMinWidthShrink                = 80.00f;
        constexpr float    kTabRounding                      = 5.00f;
        constexpr float    kTableAngledHeadersAngle          = 35.00f;
        constexpr ImVec2   kTableAngledHeadersTextAlign      = ImVec2(0.50f, 0.00f);

        constexpr float    kWindowBorderHoverPadding         = 4.00f;
        constexpr float    kWindowBorderSize                 = 1.00f;
        constexpr ImVec2   kWindowPadding                    = ImVec2(15.00f, 15.00f);
        constexpr float    kWindowRounding                   = 10.00f;
        constexpr ImGuiDir kWindowMenuButtonPosition         = ImGuiDir::ImGuiDir_Right;
        constexpr ImVec2   kWindowTitleAlign                 = ImVec2(0.50f, 0.50f);

        ImGuiStyle&        style                             = ImGui::GetStyle();

        // General Styling

        style.AntiAliasedFill                                = kAntiAliasedFill;
        style.AntiAliasedLines                               = kAntiAliasedLines;
        style.AntiAliasedLinesUseTex                         = kAntiAliasedLinesUseTexture;
        style.Alpha                                          = kGlobalAlpha;

        style.ButtonTextAlign                                = kButtonTextAlign;

        style.ChildBorderSize                                = kChildBorderSize;
        style.ChildRounding                                  = kChildRounding;
        style.CircleTessellationMaxError                     = kCircleTessellationMaxError;
        style.ColorButtonPosition                            = kColorsButtonPosition;
        style.CurveTessellationTol                           = kCurveTessellationTolerance;
        style.CellPadding                                    = kCellPadding;

        style.DisabledAlpha                                  = kDisabledAlpha;
        style.DisplayWindowPadding                           = kDisplayWindowPadding;
        style.DisplaySafeAreaPadding                         = kDisplaySafeAreaPadding;
        style.DockingSeparatorSize                           = kDockingSeparatorSize;

        style.FontScaleMain                                  = kFontScaleMain;
        style.FontSizeBase                                   = kFontSizeBase;
        style.FramePadding                                   = kFramePadding;
        style.FrameRounding                                  = kFrameRounding;
        style.FrameBorderSize                                = kFrameBorderSize;

        style.GrabMinSize                                    = kGrabMinimumSize;
        style.GrabRounding                                   = kGrabRounding;

        style.HoverFlagsForTooltipMouse                      = kHoverFlagsForTooltipMouse;
        style.HoverFlagsForTooltipNav                        = kHoverFlagsForTooltipNav;

        style.ImageBorderSize                                = kImageBorderSize;
        style.IndentSpacing                                  = kIndentSpacing;
        style.ItemInnerSpacing                               = kItemInnerSpacing;
        style.ItemSpacing                                    = kItemSpacing;

        style.LogSliderDeadzone                              = kLogSliderDeadzone;

        style.PopupBorderSize                                = kPopupBorderSize;
        style.PopupRounding                                  = kPopupRounding;

        style.ScrollbarSize                                  = kScrollbarSize;
        style.ScrollbarRounding                              = kScrollbarRounding;
        style.SelectableTextAlign                            = kSelectableTextAlign;
        style.SeparatorTextAlign                             = kSeparatorTextAlign;
        style.SeparatorTextBorderSize                        = kSeparatorTextBorderSize;
        style.SeparatorTextPadding                           = kSeparatorTextPadding;

        style.TabBarBorderSize                               = kTabBarBorderSize;
        style.TabBorderSize                                  = kTabBorderSize;
        style.TabBarOverlineSize                             = kTabBarOverlineSize;
        style.TabCloseButtonMinWidthSelected                 = kTabCloseButtonMinWidthSelected;
        style.TabCloseButtonMinWidthUnselected               = kTabCloseButtonMinWidthUnselected;
        style.TabMinWidthBase                                = kTabMinWidthBase;
        style.TabMinWidthShrink                              = kTabMinWidthShrink;
        style.TabRounding                                    = kTabRounding;
        style.TableAngledHeadersAngle                        = kTableAngledHeadersAngle;
        style.TableAngledHeadersTextAlign                    = kTableAngledHeadersTextAlign;

        style.WindowBorderHoverPadding                       = kWindowBorderHoverPadding;
        style.WindowBorderSize                               = kWindowBorderSize;
        style.WindowMenuButtonPosition                       = kWindowMenuButtonPosition;
        style.WindowPadding                                  = kWindowPadding;
        style.WindowRounding                                 = kWindowRounding;
        style.WindowTitleAlign                               = kWindowTitleAlign;

        // Color Styling

        ImVec4* colors                                       = style.Colors;

        colors[ImGuiCol_Text]                                = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
        colors[ImGuiCol_TextDisabled]                        = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
        colors[ImGuiCol_WindowBg]                            = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        colors[ImGuiCol_ChildBg]                             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                             = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        colors[ImGuiCol_Border]                              = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
        colors[ImGuiCol_BorderShadow]                        = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
        colors[ImGuiCol_FrameBg]                             = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]                      = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
        colors[ImGuiCol_FrameBgActive]                       = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBg]                             = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive]                       = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]                    = ImVec4(0.18f, 0.16f, 0.27f, 0.59f);
        colors[ImGuiCol_MenuBarBg]                           = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]                         = ImVec4(0.08f, 0.07f, 0.10f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]                       = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
        colors[ImGuiCol_ScrollbarGrabHovered]                = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]                 = ImVec4(0.23f, 0.19f, 0.27f, 1.00f);
        colors[ImGuiCol_CheckMark]                           = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
        colors[ImGuiCol_SliderGrab]                          = ImVec4(0.96f, 0.96f, 1.00f, 0.31f);
        colors[ImGuiCol_SliderGrabActive]                    = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        colors[ImGuiCol_Button]                              = ImVec4(0.15f, 0.13f, 0.18f, 1.00f);
        colors[ImGuiCol_ButtonHovered]                       = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
        colors[ImGuiCol_ButtonActive]                        = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        colors[ImGuiCol_Header]                              = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        colors[ImGuiCol_HeaderHovered]                       = ImVec4(0.24f, 0.24f, 0.29f, 1.00f);
        colors[ImGuiCol_HeaderActive]                        = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        colors[ImGuiCol_Separator]                           = ImVec4(0.67f, 0.67f, 0.78f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]                    = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive]                     = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip]                          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_ResizeGripHovered]                   = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]                    = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        colors[ImGuiCol_InputTextCursor]                     = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TabHovered]                          = ImVec4(0.49f, 0.43f, 0.71f, 0.80f);
        colors[ImGuiCol_Tab]                                 = ImVec4(0.17f, 0.16f, 0.29f, 1.00f);
        colors[ImGuiCol_TabSelected]                         = ImVec4(0.42f, 0.33f, 0.71f, 1.00f);
        colors[ImGuiCol_TabSelectedOverline]                 = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TabDimmed]                           = ImVec4(0.18f, 0.17f, 0.24f, 0.97f);
        colors[ImGuiCol_TabDimmedSelected]                   = ImVec4(0.30f, 0.28f, 0.39f, 1.00f);
        colors[ImGuiCol_TabDimmedSelectedOverline]           = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
        colors[ImGuiCol_DockingPreview]                      = ImVec4(0.43f, 0.26f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]                      = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines]                           = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        colors[ImGuiCol_PlotLinesHovered]                    = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram]                       = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
        colors[ImGuiCol_PlotHistogramHovered]                = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]                       = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]                   = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]                    = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]                          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]                       = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextLink]                            = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]                      = ImVec4(0.58f, 0.45f, 1.00f, 0.43f);
        colors[ImGuiCol_TreeLines]                           = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_DragDropTarget]                      = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavCursor]                           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]               = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]                   = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.53f);
    }

    // void App::createAudioThread()
    // {
    //     m_audioThread = std::make_optional<std::thread>(
    //         [this]()
    //         {
    //             while (true)
    //             {
    //                 if (!m_sorter)
    //                 {
    //                     std::this_thread::sleep_for(100ms);
    //                     continue;
    //                 }

    // if (m_sorter->getFlags().hasQuit) { break; }

    // if (m_sorter->elems.empty() || m_sorter->getFirst() >= m_sorter->elems.size())
    // {
    //     std::this_thread::sleep_for(100ms);
    //     continue;
    // }

    // size_t index = m_sorter->getFirst();

    // float  freq  = m_sorter->elems[index]
    //            * (m_ctx->winHeight / static_cast<float>(m_sorter->elems.size()));

    // float framerate = Core::Platform::Display::getFramerate();
    // float frameTime = (framerate > 0.01f) ? (1.0f / framerate) : 0.0f;
    // float sortTime  = 1.0f / Sort::BaseSort::s_speed;
    // float seconds   = std::max(frameTime, sortTime);

    // if (m_sorter->getFlags().isSorting || m_sorter->getFlags().isShuffling
    //     || m_sorter->getFlags().isChecking)
    // {
    //     if (m_soundEngine->load(freq, seconds) == Utils::Signal::Error)
    //     {
    //         LOG_ERROR("Could not load audio buffer");
    //         if (m_soundEngine->alGetLastError() != AL_NO_ERROR)
    //         {
    //             LOG_ERROR(
    //                 "Error loading audio with code: "
    //                 << m_soundEngine->alErrorString(m_soundEngine->alGetLastError())
    //                 << "(" << m_soundEngine->alGetLastError() << ")"
    //             );
    //             return;
    //         }
    //     }

    // if (m_soundEngine->play() == Utils::Signal::Error)
    // {
    //     LOG_ERROR("Could not play audio buffer");
    //     if (m_soundEngine->alGetLastError() != AL_NO_ERROR)
    //     {
    //         LOG_ERROR(
    //             "Error playing audio with code: "
    //             << m_soundEngine->alErrorString(m_soundEngine->alGetLastError())
    //             << "(" << m_soundEngine->alGetLastError() << ")"
    //         );
    //         return;
    //     }
    // }
    // }

    // std::this_thread::sleep_for(
    //     std::chrono::milliseconds(static_cast<int>(0.04 * 1000))
    // );
    // }
    // }
    // );
    // }

    void App::createSortThread()
    {
        m_sortThread = std::make_optional<std::thread>(
            [this]()
            {
                auto sorter = getSorter();

                sorter->shuffle(m_UI.getUIState().isReversed);

                if (!sorter->getFlags().hasAborted)
                {
                    LOG_INFO("Running Algorithm");

                    sorter->timer.start();
                    sorter->realTimer.start();

                    sorter->getFlags().setFlags(Sort::FlagGroup::StartSorting);
                    sorter->sort();
                    sorter->getFlags().setFlags(Sort::FlagGroup::DoneSorting);

                    sorter->realTimer.pause();
                    sorter->timer.pause();
                }

                if (!sorter->getFlags().hasAborted) { sorter->check(); }
            }
        );
    }

    Utils::Signal App::init()
    {
        LOG_INFO("Initializing App");

        if (initSDL() == Utils::Signal::Error)
        {
            LOG_ERROR("SDL intialization failed with error: " << SDL_GetError());
            return Utils::Signal::SDLInitError;
        }

        if (initFont() == Utils::Signal::Error)
        {
            LOG_ERROR("Font not found");
            return Utils::Signal::FontLoadError;
        }

        if (initAudio() == Utils::Signal::Error)
        {
            LOG_ERROR("Audio could not be initialized");
            return Utils::Signal::AudioInitError;
        }

        if (initImGui() == Utils::Signal::Error)
        {
            LOG_ERROR("Could not initialize ImGui");
            return Utils::Signal::ImGuiInitError;
        }

        LOG_INFO("Creating SortView");
        m_sortView = std::make_unique<Renderer::SortView>(shared_from_this(), m_UI.getUIState());

        LOG_INFO("Creating Registry");
        m_sortRegistry = Core::SortRegistry(shared_from_this());
        m_sortRegistry.registerAllSorts();

        return Utils::Signal::Success;
    }

    Utils::Signal App::initSDL()
    {
        LOG_INFO("Initializing SDL");

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            LOG_ERROR("Could not initialize SDL");
            return Utils::Signal::Error;
        }
        LOG_INFO("SDL initialized successfully");

        m_ctx = Renderer::RenderContext::CreateContext(1920.0f, 1080.0f, 240);

        LOG_INFO("Setting render parameters");
        SDL_SetRenderDrawColor(m_ctx->renderer, 0x0, 0x0, 0x0, 0x0);
        SDL_SetRenderDrawBlendMode(m_ctx->renderer, SDL_BLENDMODE_BLEND);

        LOG_INFO("Clearing window");
        SDL_RenderClear(m_ctx->renderer);

        return Utils::Signal::Success;
    }

    Utils::Signal App::initFont()
    {
        LOG_INFO("Loading font");
        TTF_Init();

        if (m_ctx->createFont("res/font.ttf") == Utils::Signal::Error)
        {
            return Utils::Signal::Error;
        }

        return Utils::Signal::Success;
    }

    Utils::Signal App::initAudio()
    {
        LOG_INFO("Initializing audio subsystem");
        m_audioSystem = Audio::System::get();
        if (m_audioSystem->init() == Utils::Signal::Error)
        {
            LOG_ERROR("Sound could not be initialized");
            return Utils::Signal::Error;
        }

        // m_soundEngine->setWaveform(SoundEngine::Waveform::Sine);
        LOG_INFO("Audio subsystem initialized successfully");
        return Utils::Signal::Success;
    }

    Utils::Signal App::initImGui()
    {
        LOG_INFO("Setting up GUI");

        LOG_INFO("Setting up ImGui context");
        IMGUI_CHECKVERSION();
        ImGuiContext* imCtx = ImGui::CreateContext();
        ImGui::SetCurrentContext(imCtx);

        LOG_INFO("Setting ImGui IO flags");

        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

#if defined(__ANDROID__)
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
#endif

        LOG_INFO("Setting ImGui styling");
        setImGuiStyle();

        LOG_INFO("Setting up ImGui renderer");
        if (!ImGui_ImplSDL3_InitForSDLRenderer(m_ctx->window, m_ctx->renderer))
        {
            return Utils::Signal::Error;
        }

        LOG_INFO("Initializing ImGui SDL renderer");
        if (!ImGui_ImplSDLRenderer3_Init(m_ctx->renderer)) { return Utils::Signal::Error; }

        m_UI               = Renderer::UI(shared_from_this());

        m_imguiInitialized = true;

        return Utils::Signal::Success;
    }

    Utils::Signal App::handleSortRequests()
    {
        if (m_sorter->getFlags().hasQuit || m_sorter->getFlags().hasAborted)
        {
            Utils::terminateThread(m_sortThread);

            m_sorter->timer.end();
            m_sorter->realTimer.end();

            return (
                m_sorter->getFlags().hasQuit ? Utils::Signal::CloseApp : Utils::Signal::StopSort
            );
        }

        if (m_sorter->getFlags().shouldSort)
        {
            Utils::terminateThread(m_sortThread);

            m_sorter->getFlags().setFlags(Sort::FlagGroup::CreatingThread);

            createSortThread();
        }
        return Utils::Signal::Success;
    }

    void App::run()
    {
        LOG_INFO("Initializing sorter");
        if (auto* entry = m_sortRegistry.get("bubble"))
        {
            constexpr std::uint64_t defaultSize = 512;
            m_sorter                            = entry->factory();
            m_sorter->setLength(defaultSize);
        }

        LOG_INFO("Creating audio thread");

        // createAudioThread();

        LOG_INFO("Starting main loop");

        bool running = true;
        while (running)
        {
            auto start = std::chrono::high_resolution_clock::now();

            switch (handleSortRequests())
            {
                case Utils::Signal::StopSort: break;
                case Utils::Signal::CloseApp:
                    LOG_INFO("Exit signal recieved");
                    running = false;
                    break;
                [[likely]] case Utils::Signal::Success:
                    [[fallthrough]];
                default: break;
            }

            while (SDL_PollEvent(&m_event))
            {
                SDL_ConvertEventToRenderCoordinates(m_ctx->renderer, &m_event);

                ImGui_ImplSDL3_ProcessEvent(&m_event);
                switch (m_event.type)
                {
                    case SDL_EVENT_DISPLAY_ORIENTATION:
                        m_ctx->refreshOrientation();
                        m_UI.refreshOrientation();
                        break;
                    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                        if (m_event.window.windowID != SDL_GetWindowID(m_ctx->window)) { break; }
                    case SDL_EVENT_QUIT: [[fallthrough]];
                    case SDL_EVENT_TERMINATING:
                        m_sorter->getFlags().setFlags(Sort::FlagGroup::Quit);
                        break;
                }
            }

            {
                m_sortView->update(m_UI.getUIState());

                ImGui_ImplSDLRenderer3_NewFrame();
                ImGui_ImplSDL3_NewFrame();
                ImGui::NewFrame();

                m_UI.renderUI();

                ImGui::Render();
                ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_ctx->renderer);

                SDL_RenderPresent(m_ctx->renderer);
            }

            {
                std::chrono::duration<double, std::milli>
                       elapsed = std::chrono::high_resolution_clock::now() - start;

                double delay   = m_ctx->getFrameTime() - elapsed.count();

                if (delay > 1.5) { SDL_Delay(static_cast<Uint32>(delay - 1.0)); }

                while (true)
                {
                    auto now = std::chrono::high_resolution_clock::now();

                    if (std::chrono::duration<double, std::milli>(now - start).count()
                        >= m_ctx->getFrameTime())
                    {
                        break;
                    }
                }
            }
        }

        return;
    }

}  // namespace Core
