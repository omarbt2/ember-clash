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
    src/main.cpp

    src/Core/App.cpp
    src/Core/Logger.cpp
    src/Core/Time.cpp

    src/Gameplay/Attack.cpp
    src/Gameplay/AiController.cpp
    src/Gameplay/Fighter.cpp
    src/Gameplay/InputMap.cpp
    src/Gameplay/Match.cpp

    src/Graphics/Renderer.cpp
    src/Graphics/Window.cpp

    src/Input/InputManager.cpp

    src/Physics/Motion.cpp

    src/UI/BitmapFont.cpp
    src/UI/Hud.cpp

    src/World/Arena.cpp
)

add_executable(EmberClash ${EMBER_SOURCES})

# -----------------------------------------------------------------------------
# Platform-specific source and build configuration.
# -----------------------------------------------------------------------------
# Emscripten: swap the desktop entry point for the web entry point. The web
# entry calls App::step() from emscripten_set_main_loop(); see
# src/main_emscripten.cpp. No other gameplay source is platform-specific.
if(EMSCRIPTEN)
    target_sources(EmberClash PRIVATE src/main_emscripten.cpp)
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