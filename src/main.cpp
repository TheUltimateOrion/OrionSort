#include "core/app.h"
#include "core/logging/logging.h"
#include "renderer/context.h"

#ifndef __ANDROID__
    #define SDL_MAIN_HANDLED 1
#endif

#include <SDL3/SDL_main.h>

#include <memory>

#include <cstdlib>

#if defined(__ANDROID__)
int SDL_main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;
#else
int main()
{
    SDL_SetMainReady();
#endif

    std::shared_ptr<Core::App>
        app = std::make_shared<Core::App>();

    if (app->init() != Utils::Signal::Success) { return EXIT_FAILURE; }

    LOG_INFO("App initialized successfully");
    app->run();

    return EXIT_SUCCESS;
}
