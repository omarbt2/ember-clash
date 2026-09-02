# Copies SDL3.dll (or its Debug-suffixed variant) next to the game executable
# on Windows. Runs as part of EmberClash's POST_BUILD step.

if(NOT WIN32)
    return()
endif()

if(NOT SDL3_DLL)
    message(WARNING "CopySDL3DLL: SDL3_DLL not provided, skipping copy.")
    return()
endif()

if(NOT EXISTS "${SDL3_DLL}")
    message(WARNING "CopySDL3DLL: '${SDL3_DLL}' does not exist.")
    return()
endif()

file(COPY "${SDL3_DLL}"
     DESTINATION "${APP_DIR}"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)

message(STATUS "Copied SDL3 runtime: ${SDL3_DLL} -> ${APP_DIR}")