#include "Core/App.h"
#include "Core/Logger.h"

#include <SDL3/SDL_main.h>

int SDL_main(int /*argc*/, char** /*argv*/) {
    ember::App app;
    const int rc = app.run();
    ember::Logger::info("Ember Clash exiting cleanly.");
    return rc;
}