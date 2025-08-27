#pragma once

#ifndef _TESTING_
    #include "renderer/sort_view.h"
    #include "renderer/ui.h"
#endif

#include "audio/system.h"
#include "renderer/context.h"
#include "renderer/disp_type.h"

#ifndef _TESTING_
    #include <imgui.h>
    #include <SDL3/SDL.h>
    #include <SDL3_ttf/SDL_ttf.h>
#endif

#include <memory>
#include <optional>
#include <thread>

#ifndef _TESTING_

namespace Core
{
    class App : public std::enable_shared_from_this<App>
    {
    private:
        std::unique_ptr<Renderer::SortView> m_sortView;
        std::shared_ptr<Sort::BaseSort>     m_sorter;

        Audio::System*                      m_audioSystem {nullptr};
        Renderer::RenderContext*            m_ctx {nullptr};

        Renderer::UI                        m_UI {nullptr};
        Core::SortRegistry                  m_sortRegistry {nullptr};
        SDL_Event                           m_event {};

        bool                                m_imguiInitialized {false};

        std::optional<std::thread>          m_audioThread {};
        std::optional<std::thread>          m_sortThread {};

        void                                createAudioThread();
        void                                createSortThread();
        void                                setImGuiStyle() const noexcept;

        void                        handleAppEvents(void* t_userdata, SDL_Event* t_event) noexcept;

        [[nodiscard]] Utils::Signal initSDL();
        [[nodiscard]] Utils::Signal initImGui();
        [[nodiscard]] Utils::Signal initAudio();
        [[nodiscard]] Utils::Signal initFont();
        [[nodiscard]] Utils::Signal handleSortRequests();

    public:
        App() noexcept = default;
        ~App();

        Utils::Signal                                 init();
        void                                          run();

        inline Core::SortRegistry const&              getRegistry() const { return m_sortRegistry; }

        inline Renderer::RenderContext const*         getContext() const { return m_ctx; }

        inline std::shared_ptr<Sort::BaseSort> const& getSorter() const { return m_sorter; }

        inline void setSorter(std::shared_ptr<Sort::BaseSort> t_sorter) { m_sorter = t_sorter; }
    };
}  // namespace Core
#endif
