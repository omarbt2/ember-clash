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

    src/Gameplay/Fighter.cpp
    src/Gameplay/InputMap.cpp
    src/Gameplay/Match.cpp

    src/Graphics/Renderer.cpp
    src/Graphics/Window.cpp

    src/Input/InputManager.cpp

    src/Physics/Motion.cpp

    src/World/Arena.cpp
)

add_executable(EmberClash ${EMBER_SOURCES})

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

# -----------------------------------------------------------------------------
# Installation (basic layout for future distribution)
# -----------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS EmberClash
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)