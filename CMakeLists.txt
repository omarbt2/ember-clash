cmake_minimum_required(VERSION 3.20)

project(EmberClash
    VERSION 0.1.0
    DESCRIPTION "Ember Clash - a 2D 1v1 platform fighter"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Global compile settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(
        /W4           # High warning level
        /permissive-  # Strict standards conformance
        /Zc:__cplusplus # Report correct __cplusplus value
        /utf-8        # Source files are UTF-8
        /EHsc         # Standard C++ exception handling
    )
    # SDL3 may emit noisy warnings on /W4. Keep them visible but contained.
    if(MSVC_VERSION GREATER_EQUAL 1929)
        add_compile_options(/Zc:preprocessor)
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# -----------------------------------------------------------------------------
# Dependencies (SDL3 fetched at configure time)
# -----------------------------------------------------------------------------
include(FetchContent)

set(SDL3_TAG "release-3.2.10" CACHE STRING "SDL3 tag/branch to fetch")
set(SDL3_REPO "https://github.com/libsdl-org/SDL.git")

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY ${SDL3_REPO}
    GIT_TAG        ${SDL3_TAG}
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES_RECURSE TRUE
)

# Disable SDL3 examples/tests/static analysis; we only need the runtime lib.
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_TESTS    OFF CACHE BOOL "" FORCE)
set(SDL_STATIC_PIC OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SDL3)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------
set(EMBER_SOURCES
    # Core / gameplay / platform-agnostic sources. Both the desktop binary and
    # the WebAssembly binary include this set. The desktop-only entry point
    # (src/main.cpp) and the web-only entry point (src/main_emscripten.cpp)
    # are NOT in this list; the if(EMSCRIPTEN) block below picks exactly one.
    src/Core/App.cpp
    src/Core/Logger.cpp
    src/Core/Settings.cpp
    src/Core/Time.cpp

    src/Gameplay/Attack.cpp
    src/Gameplay/AiController.cpp
    src/Gameplay/Animation.cpp
    src/Gameplay/Fighter.cpp
    src/Gameplay/Sprite.cpp
    src/Gameplay/InputMap.cpp
    src/Gameplay/Match.cpp

    src/Audio/Audio.cpp

    src/Graphics/Renderer.cpp
    src/Graphics/Window.cpp

    src/Input/Gamepad.cpp
    src/Input/InputManager.cpp

    src/Physics/Motion.cpp

    src/UI/BitmapFont.cpp
    src/UI/Hud.cpp
    src/UI/Menu.cpp

    src/World/Arena.cpp
)

# Exactly one entry point per platform. Listing both in the same target
# breaks the Emscripten build (SDL_main.h is missing from the web toolchain)
# and would also confuse the linker on the desktop. We add the right one
# to EMBER_SOURCES per platform before add_executable().
set(EMBER_DESKTOP_ENTRY src/main.cpp)
set(EMBER_WEB_ENTRY     src/main_emscripten.cpp)

if(EMSCRIPTEN)
    set(EMBER_SOURCES ${EMBER_SOURCES} ${EMBER_WEB_ENTRY})
else()
    set(EMBER_SOURCES ${EMBER_SOURCES} ${EMBER_DESKTOP_ENTRY})
endif()

add_executable(EmberClash ${EMBER_SOURCES})

# -----------------------------------------------------------------------------
# Platform-specific source and build configuration.
# -----------------------------------------------------------------------------
# Emscripten: the entry point is src/main_emscripten.cpp (already selected
# in EMBER_SOURCES above). The web entry calls App::step() from
# emscripten_set_main_loop(); see src/main_emscripten.cpp. No other gameplay
# source is platform-specific.
if(EMSCRIPTEN)
    # The shared include path lives at the top so both branches can use it.
    # Without this, web builds see "Graphics/Renderer.h file not found" because
    # every gameplay .cpp includes via the "Module/Header.h" relative form.
    target_include_directories(EmberClash PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    # The Emscripten SDL3 port (pulled in by -sUSE_SDL=3 below) installs
    # headers as <SDL/SDL.h> - the SDL2 layout. Our code uses the SDL3
    # upstream layout <SDL3/SDL.h> (which is what FetchContent provides).
    # To keep ONE set of includes in the codebase, we link the SDL3
    # static library that the same FetchContent_MakeAvailable above
    # already built; this gives us the proper headers AND the proper
    # library. The Emscripten -sUSE_SDL=3 flag below is still required
    # for the JS <-> SDL event bridge glue (HTML5 keyboard, canvas,
    # audio, etc.), but we don't use its headers or its static lib.
    target_link_libraries(EmberClash PRIVATE SDL3::SDL3)
    # Replace the desktop .exe output with the Emscripten triple
    # (EmberClash.html + EmberClash.js + EmberClash.wasm). The empty
    # SUFFIX is what tells CMake to use the Emscripten output layout
    # (.html as the primary artifact, .js / .wasm as sidecars) instead
    # of the default .exe. We do not set RUNTIME_OUTPUT_NAME because
    # that would clobber all three artifacts.
    set_target_properties(EmberClash PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/bin
        SUFFIX                    ""
    )
    # Required Emscripten link flags for SDL3 + browser:
    #   -sUSE_SDL=3     Tells emcc to link against the SDL3 port; this
    #                   pulls in <emscripten.h> glue, the HTML5 shell, and
    #                   the JS <-> SDL event bridge.
    #   -sALLOW_MEMORY_GROWTH=1  Lets the WebAssembly heap grow on demand
    #                   instead of failing on a fixed-size bump allocator.
    #   -sEXIT_RUNTIME=1 Ensures SDL_Quit() and friends actually run on
    #                   page close (otherwise the browser just kills the
    #                   worker without unwinding the WASM heap).
    # We deliberately do NOT enable ASYNCIFY (-sASYNCIFY): the gameplay
    # loop is plain synchronous C++ that never blocks on a JS callback,
    # so the ASYNCIFY code-size cost would buy us nothing.
    target_link_options(EmberClash PRIVATE
        "-sUSE_SDL=3"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sEXIT_RUNTIME=1"
    )
    # The deploy bundle is the source-tree web/ directory: hand-maintained
    # web/index.html (the player-facing page) plus the emcc-built
    # EmberClash.js + EmberClash.wasm that emcc produced in
    # ${CMAKE_BINARY_DIR}/bin. Copy the artifacts next to the HTML so
    # `python -m http.server` (or any static host) can serve the page
    # without a separate "publish" step. The .html inside web/ IS the
    # production page; we are NOT using --shell-file (the M8 spec
    # deliberately ships the page as a static asset instead of an emcc
    # shell template, so future M10 release work can edit the page
    # without re-running the emcc shell-template pass).
    add_custom_command(
        TARGET EmberClash POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DEMIT_JS=$<TARGET_FILE:EmberClash>
            -DEMIT_WASM=$<TARGET_FILE:EmberClash>.wasm
            -DWEB_DIR=${CMAKE_SOURCE_DIR}/web
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyWebArtifacts.cmake
        COMMENT "Copying EmberClash.js + EmberClash.wasm to web/"
    )
    # Note: nothing in src/Core/Time.cpp needs an Emscripten-specific clock.
    # SDL_GetPerformanceCounter maps to emscripten_get_now() under the hood.
else()
    target_include_directories(EmberClash PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(EmberClash PRIVATE
        SDL3::SDL3
    )
    # Place runtime artifacts in a dedicated directory so we can ship the DLL
    # alongside the executable.
    set_target_properties(EmberClash PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # On Windows, copy SDL3.dll next to the executable after every successful
    # link of the main target (Debug and Release configurations both need it).
    if(WIN32)
        add_custom_command(
            TARGET EmberClash POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DSDL3_DLL=$<TARGET_FILE:SDL3::SDL3>
                -DAPP_DIR=$<TARGET_FILE_DIR:EmberClash>
                -P ${CMAKE_SOURCE_DIR}/cmake/CopySDL3DLL.cmake
            COMMENT "Copying SDL3.dll next to EmberClash.exe"
        )
    endif()
endif()

# -----------------------------------------------------------------------------
# Installation (basic layout for future distribution)
# -----------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS EmberClash
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)