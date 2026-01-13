#include "renderer/ui.h"

#include "core/app.h"
#include "core/logging/logging.h"
#include "core/platform/build_info.h"
#include "core/platform/display.h"
#include "renderer/context.h"
#include "renderer/state.h"
#include "sort/category.h"
#include "sort/flags.h"
#include "sort/sort.h"

#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <format>
#include <limits>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>

#include <cstddef>
#include <cstdint>

namespace Renderer
{
    UI::UI(std::shared_ptr<Core::App> t_app) noexcept : m_app {t_app} { }

    void UI::renderAboutWindow() noexcept
    {
        ImGui::SetNextWindowSize(ImVec2(550.0f, 450.0f));
        if (ImGui::BeginPopupModal("OrionSort##modal", nullptr, ImGuiWindowFlags_NoResize))
        {
            ImGui::Text("OrionSort");
            ImGui::SameLine();
            ImGui::TextDisabled("v%s", APP_VERSION);

            ImGui::Separator();

            // --- Build / Environment (two-column table) ---
            if (ImGui::BeginTable(
                    "about_kv", 2,
                    ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg
                        | ImGuiTableFlags_BordersInnerV
                ))
            {
                auto row = [](char const* k, char const* v)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(k);
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(v);
                };

                // Build & toolchain
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextDisabled("Build");
                ImGui::TableNextColumn();
                ImGui::Text("%s, %s %s", Core::Platform::BuildType(), __DATE__, __TIME__);

                row("OS", Core::Platform::OSName());
                row("Arch", (sizeof(void*) == 8) ? "x64 (64-bit)" : "x86 (32-bit)");
                {
                    static std::string const comp = Core::Platform::CompilerString();
                    row("Compiler", comp.c_str());
                }
                row("C++ Standard", Core::Platform::CppStandard());

                // ImGui/backend/runtime info
                row("Dear ImGui", ImGui::GetVersion());
                {
                    ImGuiIO const& io = ImGui::GetIO();
                    row("Backend (Platform)",
                        io.BackendPlatformName ? io.BackendPlatformName : "Unknown");
                    row("Backend (Renderer)",
                        io.BackendRendererName ? io.BackendRendererName : "Unknown");
                }

                {
                    ImGuiIO const& io = ImGui::GetIO();
                    char           buf[128];
                    std::snprintf(
                        buf, sizeof(buf), "%.0fx%.0f (scale %.2f)", io.DisplaySize.x,
                        io.DisplaySize.y, io.FontGlobalScale
                    );
                    row("Display", buf);
                }
                {
                    char        buf[128];
                    float const framerate = Core::Platform::Display::getFramerate();
                    float const ms        = (framerate > 0.01f) ? (1000.0f / framerate) : 0.0f;
                    std::snprintf(buf, sizeof(buf), "%.1f FPS (%.2f ms/frame)", framerate, ms);
                    row("Performance", buf);
                }

                ImGui::EndTable();
            }

            ImGui::Separator();
            if (ImGui::CollapsingHeader("Notes", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BulletText("This is a sorting visualizer built for clarity and speed.");
                ImGui::BulletText("Supports multiple algorithms and live step visualization.");
                ImGui::BulletText("Use the menu to change algorithms, size, and speed.");
            }

            if (ImGui::CollapsingHeader("Credits"))
            {
                ImGui::BulletText("Dear ImGui (MIT)");
                ImGui::BulletText("SDL (Zlib)");
                ImGui::BulletText("SDL_ttf (Zlib)");
                ImGui::BulletText("OpenAL-Soft (GPL v2)");
            }

            if (ImGui::CollapsingHeader("Keyboard Shortcuts"))
            {
                ImGui::BulletText("Work in Progress!!!");
            }

            // --- Action row ---
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Copy diagnostics"))
            {
                std::stringstream ss;
                ImGuiIO const&    io        = ImGui::GetIO();
                float             framerate = Core::Platform::Display::getFramerate();
                ss << "OrionSort v" << APP_VERSION
#ifdef GIT_COMMIT_HASH
                   << " (" << GIT_COMMIT_HASH << ")"
#endif
                   << "\nBuild: " << Core::Platform::BuildType() << ", " << __DATE__ << " "
                   << __TIME__ << "\nOS: " << Core::Platform::OSName()
                   << "\nArch: " << ((sizeof(void*) == 8) ? "x64" : "x86")
                   << "\nCompiler: " << Core::Platform::CompilerString()
                   << "\nC++: " << Core::Platform::CppStandard()
                   << "\nDear ImGui: " << ImGui::GetVersion() << "\nBackend(Platform): "
                   << (io.BackendPlatformName ? io.BackendPlatformName : "Unknown")
                   << "\nBackend(Renderer): "
                   << (io.BackendRendererName ? io.BackendRendererName : "Unknown")
                   << "\nDisplay: " << io.DisplaySize.x << "x" << io.DisplaySize.y << " (scale "
                   << io.FontGlobalScale << ")"
                   << "\nPerf: " << framerate << " FPS ("
                   << (framerate > 0.01f ? (1000.0f / framerate) : 0.0f) << " ms/frame)";
                ImGui::SetClipboardText(ss.str().c_str());
            }
            ImGui::SameLine();

            if (ImGui::Button("Website")) { SDL_OpenURL(GITHUB_REPO); }
            ImGui::SameLine();
            if (ImGui::Button("Report issue")) { SDL_OpenURL(GITHUB_REPO "/issues"); }
            ImGui::SameLine();
            if (ImGui::Button("Check updates")) { SDL_OpenURL(GITHUB_REPO "/releases"); }

            ImGui::SameLine();
            if (ImGui::Button("Close"))
            {
                m_uiState.isAboutPopupOpened = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Checkbox("Debug Mode", &m_uiState.isDebugMode);

            ImGui::EndPopup();
        }
    }

    void UI::renderText(
        std::string const& t_txt,
        float              t_x,
        float              t_y,
        SDL_Color          t_col
    ) const noexcept
    {
        if (auto appShared = m_app.lock())
        {
            RenderContext const* const ctx = appShared->getContext();

            SDL_Surface* textSurface = TTF_RenderText_Solid(ctx->font, t_txt.c_str(), 0, t_col);
            SDL_Texture* text        = SDL_CreateTextureFromSurface(ctx->renderer, textSurface);
            float        text_width  = static_cast<float>(textSurface->w);
            float        text_height = static_cast<float>(textSurface->h);

            SDL_DestroySurface(textSurface);
            SDL_FRect renderQuad {t_x, t_y, text_width, text_height};
            SDL_RenderTexture(ctx->renderer, text, nullptr, &renderQuad);
            SDL_DestroyTexture(text);
        }
    }

    void UI::renderInfo() const noexcept
    {
        if (auto appShared = m_app.lock())
        {
            auto&                          sorter   = appShared->getSorter();
            Core::SortRegistry const&      registry = appShared->getRegistry();
            RenderContext const* const     ctx      = appShared->getContext();

            Sort::Flags&                   flags    = sorter->getFlags();
            std::vector<std::string>       ids = registry.idsByCategory(m_uiState.sortCategory);
            Core::SortRegistryEntry const* currentEntry = registry.get(ids[m_uiState.sortIndex]);

            {
                Uint8 _r, _g, _b, _a;
                SDL_GetRenderDrawColor(ctx->renderer, &_r, &_g, &_b, &_a);

                SDL_FRect rect {0.0f, 0.0f, 300.0f, 160.0f};

                SDL_SetRenderDrawColor(ctx->renderer, 0x40, 0x40, 0x40, 0x80);
                SDL_RenderFillRect(ctx->renderer, &rect);

                SDL_SetRenderDrawColor(ctx->renderer, _r, _g, _b, _a);
            }

            renderText(
                "TIME: " + std::to_string(sorter->timer.getDuration()) + 's', 10.0f, 10.0f,
                {0xFF, 0xFF, 0xFF, 0x00}
            );
            renderText(
                "REAL TIME: " + std::to_string(sorter->realTimer.getDuration()) + 's', 10.0f, 30.0f,
                {0xFF, 0xFF, 0xFF, 0x00}
            );

            {
                std::string name {};
                if (m_uiState.sortIndex < ids.size()) { name = currentEntry->displayName; }
                renderText(std::string("SORT: ") + name, 10.0f, 50.0f, {0xFF, 0xFF, 0xFF, 0x00});
            }

            renderText(
                "SWAPS: " + std::to_string(sorter->elems.getSwaps()), 10.0f, 70.0f,
                {0xFF, 0xFF, 0xFF, 0}
            );
            renderText(
                "COMPARISONS: " + std::to_string(sorter->elems.getComparisons()), 10.0f, 90.0f,
                {0xFF, 0xFF, 0xFF, 0}
            );

            {
                std::string statusText {"IDLE"};

                SDL_Color   statusColor {0xFF, 0xFF, 0xFF, 0x00};

                if (flags.isSorting) { statusText = "SORTING..."; }

                if (flags.isShuffling) { statusText = "SHUFFLING..."; }

                if (flags.isChecking) { statusText = "CHECKING..."; }

                if (flags.isSorted)
                {
                    statusColor = {0x00, 0xFF, 0x00, 0x00};
                    statusText  = "SORTED!";
                }

                if (flags.hasAborted)
                {
                    statusColor = {0xFF, 0x00, 0x00, 0x00};
                    statusText  = "ABORTED!";
                }

                renderText(statusText, 10.0f, 120.0f, statusColor);
            }
        }
    }

    void UI::renderDebugMenu()
    {
#define IMGUI_DEBUG_FLAG(flags, boolVal)                                     \
    ImGui::TableNextRow();                                                   \
    ImGui::TableNextColumn();                                                \
    ImGui::Text(#boolVal);                                                   \
    ImGui::TableNextColumn();                                                \
    ImGui::TextUnformatted(std::format("{}", flags.boolVal.load()).c_str());

#define IMGUI_DEBUG_UISTATE(uiObj, data)                           \
    ImGui::TableNextRow();                                         \
    ImGui::TableNextColumn();                                      \
    ImGui::Text(#data);                                            \
    ImGui::TableNextColumn();                                      \
    ImGui::TextUnformatted(std::format("{}", uiObj.data).c_str());

#define IMGUI_DEBUG_SORT(sortPtr, data)                                 \
    ImGui::TableNextRow();                                              \
    ImGui::TableNextColumn();                                           \
    ImGui::Text(#data);                                                 \
    ImGui::TableNextColumn();                                           \
    ImGui::TextUnformatted(std::format("{}", (*sortPtr).data).c_str());

        ImGui::Text(
            "Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / Core::Platform::Display::getFramerate(),
            Core::Platform::Display::getFramerate()
        );

        ImGui::Spacing();
        ImGui::SeparatorText("Debug Variables");
        ImGui::Spacing();

        if (auto appShared = m_app.lock())
        {
            ImGui::TextUnformatted(std::format("{}", appShared->getSorter()->elems).c_str());

            Sort::Flags& flags = appShared->getSorter()->getFlags();
            if (ImGui::CollapsingHeader("Sorting Data"))
            {
                if (ImGui::BeginTable(
                        "debug_sort", 2,
                        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg
                            | ImGuiTableFlags_BordersInnerV
                    ))
                {
                    IMGUI_DEBUG_SORT(appShared->getSorter(), getFirst());
                    IMGUI_DEBUG_SORT(appShared->getSorter(), getSecond());
                    ImGui::EndTable();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Sorting Flags"))
            {
                if (ImGui::BeginTable(
                        "debug_flags", 2,
                        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg
                            | ImGuiTableFlags_BordersInnerV
                    ))
                {
                    IMGUI_DEBUG_FLAG(flags, hasAborted);
                    IMGUI_DEBUG_FLAG(flags, hasQuit);

                    IMGUI_DEBUG_FLAG(flags, isChecking);
                    IMGUI_DEBUG_FLAG(flags, isRunning);
                    IMGUI_DEBUG_FLAG(flags, isShuffling);
                    IMGUI_DEBUG_FLAG(flags, isSorted);
                    IMGUI_DEBUG_FLAG(flags, isSorting);

                    IMGUI_DEBUG_FLAG(flags, shouldSort);

                    ImGui::EndTable();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("UI State"))
            {
                if (ImGui::BeginTable(
                        "debug_uistate", 2,
                        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg
                            | ImGuiTableFlags_BordersInnerV
                    ))
                {
                    IMGUI_DEBUG_UISTATE(m_uiState, arrayLength);
                    IMGUI_DEBUG_UISTATE(m_uiState, isColored);
                    IMGUI_DEBUG_UISTATE(m_uiState, isReversed);
                    IMGUI_DEBUG_UISTATE(m_uiState, sortParameter);
                    IMGUI_DEBUG_UISTATE((int) m_uiState, sortCategory);
                    IMGUI_DEBUG_UISTATE((int) m_uiState, sortDisplayType);
                    IMGUI_DEBUG_UISTATE(m_uiState, sortIndex);

                    ImGui::EndTable();
                }
            }
        }
#undef IMGUI_DEBUG_FLAG
#undef IMGUI_DEBUG_UISTATE
    }

    void UI::renderSortChooser(
        Core::SortRegistry const&       t_registry,
        std::string const&              t_currentName,
        std::vector<std::string> const& t_ids
    )
    {
        if (ImGui::BeginCombo(
                "##combo1", m_sortCategories[m_uiState.sortCategory]
            ))  // The second parameter is the label previewed before opening the combo.
        {
            for (std::size_t n = 0; n < m_sortCategories.size(); ++n)
            {
                bool isSelected
                    = (m_sortCategories[m_uiState.sortCategory]
                       == m_sortCategories[n]);  // You can store your selection
                                                 // however you want, outside or
                                                 // inside your objects
                if (ImGui::Selectable(m_sortCategories[n], isSelected))
                {
                    m_uiState.sortCategory = static_cast<Sort::Category>(n);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();  // You may set the initial focus when
                                                   // opening the combo (scrolling + for
                                                   // keyboard navigation support)
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();

        if (ImGui::BeginCombo(
                "##combo2", t_currentName.c_str()
            ))  // The second parameter is the label previewed before opening the combo.
        {
            for (std::size_t n = 0; n < t_ids.size(); ++n)
            {
                Core::SortRegistryEntry const* selectedEntry = t_registry.get(t_ids[n]);
                bool                           isSelected    = (n == m_uiState.sortIndex);

                if (ImGui::Selectable(selectedEntry->displayName.c_str(), isSelected))
                {
                    m_uiState.sortIndex = n;
                }
                if (isSelected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }
    }

    void UI::renderSortDisplayConfigs()
    {
        if (ImGui::BeginCombo(
                "##combo3", m_sortDisplayTypes[m_uiState.sortDisplayType]
            ))  // The second parameter is the label previewed before opening the combo.
        {
            for (std::size_t n = 0; n < m_sortDisplayTypes.size(); ++n)
            {
                bool isSelected
                    = (m_sortDisplayTypes[m_uiState.sortDisplayType]
                       == m_sortDisplayTypes[n]);  // You can store your selection
                                                   // however you want, outside or
                                                   // inside your objects

                if (ImGui::Selectable(m_sortDisplayTypes[n], isSelected))
                {
                    m_uiState.sortDisplayType = static_cast<DisplayType>(n);
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();  // You may set the initial focus when
                                                   // opening the combo (scrolling + for
                                                   // keyboard navigation support)
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::Checkbox("Color", &m_uiState.isColored);
    }

    void UI::renderSortAlgorithmConfigs(Core::SortRegistryEntry const* const t_currentEntry)
    {
        ImGui::InputFloat("Set Speed", &Sort::BaseSort::s_speed, 0.001f);
        Sort::BaseSort::s_speed = std::clamp<float>(Sort::BaseSort::s_speed, 0.001f, 1000.f);

        ImGui::Spacing();

        int length = static_cast<int>(m_uiState.arrayLength);
        ImGui::InputInt("Set Array Length", &length, 2);
        m_uiState.arrayLength = std::clamp<size_t>(length, 2, 1024 * 10);

        if (t_currentEntry->isParameterized)
        {
            static std::uint8_t prevIndex = std::numeric_limits<std::uint8_t>::max();

            static std::pair<std::int64_t, std::int64_t> bounds;

            if (m_uiState.sortIndex != prevIndex)
            {
                auto tempSorter        = t_currentEntry->factory();
                auto parameterizedSort = std::dynamic_pointer_cast<Sort::IParameterized>(
                    tempSorter
                );
                bounds    = parameterizedSort->getParameterBounds();

                prevIndex = m_uiState.sortIndex;
            }

            m_uiState.sortParameter = std::clamp(
                m_uiState.sortParameter, bounds.first, bounds.second
            );

            ImGui::SliderScalar(
                "Set Buckets/Radix", ImGuiDataType_S64, &m_uiState.sortParameter, &bounds.first,
                &bounds.second, "%d"
            );
        }

        ImGui::Spacing();

        ImGui::Checkbox("Reverse instead of Shuffling", &m_uiState.isReversed);
    }

    void UI::renderActionButtons(
        Core::SortRegistryEntry const* const t_currentEntry,
        std::vector<std::string> const&      t_ids
    )
    {
        if (auto appShared = m_app.lock())
        {
            auto sorter = appShared->getSorter();

            if (!sorter->getFlags().isRunning)
            {
                if (ImGui::Button("Sort"))
                {
                    LOG_INFO("Starting sort");
                    if (m_uiState.sortIndex < t_ids.size())
                    {
                        if (t_currentEntry)
                        {
                            appShared->setSorter(t_currentEntry->factory());

                            auto newSorter = appShared->getSorter();

                            newSorter->setLength(m_uiState.arrayLength);

                            if (t_currentEntry->isParameterized)
                            {
                                dynamic_pointer_cast<Sort::IParameterized>(newSorter)->setParameter(
                                    m_uiState.sortParameter
                                );
                            }

                            newSorter->getFlags().setFlags(Sort::FlagGroup::SortButtonPressed);
                        }
                    }
                    else { LOG_ERROR("Unknown sort category/index"); }
                }
            }
            else
            {
                if (ImGui::Button("Stop"))
                {
                    sorter->getFlags().setFlags(Sort::FlagGroup::StopButtonPressed);

                    LOG_INFO("Stopping sort");
                }
            }

            ImGui::BeginDisabled(!sorter->getFlags().hasAborted);
            ImGui::SameLine();
            if (ImGui::Button("Reset")) { sorter->reset(); }
            ImGui::EndDisabled();
        }
    }

    void UI::renderUI()
    {
        if (auto appShared = m_app.lock())
        {
            auto&                     sorter   = appShared->getSorter();
            Core::SortRegistry const& registry = appShared->getRegistry();
            RenderContext const*      ctx      = appShared->getContext();

            std::vector<std::string>  ids      = registry.idsByCategory(m_uiState.sortCategory);

            if (m_uiState.sortIndex >= ids.size()) { m_uiState.sortIndex = 0; }
            Core::SortRegistryEntry const* const currentEntry = registry.get(
                ids[m_uiState.sortIndex]
            );
            std::string const currentName = currentEntry ? currentEntry->displayName : "";

            if (!m_uiState.isImGuiOpen) { sorter->getFlags().setFlags(Sort::FlagGroup::Quit); }

            renderInfo();

            renderAboutWindow();

            if (m_uiState.isAboutPopupOpened) { ImGui::OpenPopup("OrionSort##modal"); }

            // Main Window
            {
                constexpr float mainWindowWidth  = 500.0f;
                constexpr float mainWindowHeight = 400.0f;

                ImGui::SetNextWindowPos(
                    ImVec2(0.5f * ctx->winWidth, 0.5f * ctx->winHeight), ImGuiCond_FirstUseEver,
                    ImVec2(0.5f, 0.5f)
                );
                ImGui::SetNextWindowSize(
                    ImVec2(mainWindowWidth, mainWindowHeight), ImGuiCond_FirstUseEver
                );

                if (m_uiState.isScreenOrientationChanged)
                {
                    ImGui::SetNextWindowPos(
                        ImVec2(0.5f * ctx->winWidth, 0.5f * ctx->winHeight), ImGuiCond_None,
                        ImVec2(0.5f, 0.5f)
                    );
                    m_uiState.isScreenOrientationChanged = false;
                }

                ImGui::Begin("Configure", &m_uiState.isImGuiOpen, ImGuiWindowFlags_MenuBar);
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::MenuItem("About")) { m_uiState.isAboutPopupOpened = true; }
                    ImGui::EndMenuBar();
                }

                ImGui::SeparatorText("Sort Selection");
                ImGui::Spacing();

                renderSortChooser(registry, currentName, ids);

                ImGui::Spacing();
                ImGui::SeparatorText("Display Config");
                ImGui::Spacing();

                renderSortDisplayConfigs();

                ImGui::Spacing();
                ImGui::SeparatorText("Variables");
                ImGui::Spacing();

                renderSortAlgorithmConfigs(currentEntry);

                ImGui::Spacing();
                ImGui::SeparatorText("Actions");
                ImGui::Spacing();

                renderActionButtons(currentEntry, ids);

                ImGui::End();
            }

            // Debug Window
            if (m_uiState.isDebugMode)
            {
                ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_Once);
                ImGui::SetNextWindowPos(
                    ImVec2(ctx->winWidth / 2, ctx->winHeight / 2), ImGuiCond_Once
                );
                ImGui::Begin("Debug");

                renderDebugMenu();

                ImGui::End();
            }
        }
    }

    ANDROID_API void UI::refreshOrientation() { m_uiState.isScreenOrientationChanged = true; }
}  // namespace Renderer
