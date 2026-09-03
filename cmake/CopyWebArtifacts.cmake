# Post-link helper: copy the Emscripten artifacts (EmberClash.js + .wasm)
# into the source-tree web/ directory so the deploy bundle is self-contained.
#
# Inputs (set via -D on the cmake command line):
#   EMIT_JS    absolute path to the .js (build-web/bin/EmberClash.js)
#   EMIT_WASM  absolute path to the .wasm (build-web/bin/EmberClash.wasm)
#   WEB_DIR    absolute path to the deploy directory (web/)
#
# Behaviour: copy each artifact (overwrite in place) so re-running the build
# is idempotent. We deliberately do NOT generate a .data / .mem sidecar in
# this milestone; the M8 build does not emit one (no -sINITIAL_MEMORY=big
# etc., and no embedded file system).

if(NOT EMIT_JS OR NOT EMIT_WASM OR NOT WEB_DIR)
    message(FATAL_ERROR "CopyWebArtifacts.cmake: EMIT_JS / EMIT_WASM / WEB_DIR are required")
endif()

if(NOT EXISTS "${EMIT_JS}")
    message(FATAL_ERROR "CopyWebArtifacts.cmake: ${EMIT_JS} does not exist; did the link succeed?")
endif()
if(NOT EXISTS "${EMIT_WASM}")
    message(FATAL_ERROR "CopyWebArtifacts.cmake: ${EMIT_WASM} does not exist; did the link succeed?")
endif()

# Always start from a clean slate: remove the prior .js / .wasm so a stale
# build cannot be served by accident after a source change. The deploy
# directory's other files (index.html, README, etc.) are left alone.
file(REMOVE "${WEB_DIR}/EmberClash.js" "${WEB_DIR}/EmberClash.wasm")

configure_file("${EMIT_JS}"   "${WEB_DIR}/EmberClash.js"   COPYONLY)
configure_file("${EMIT_WASM}" "${WEB_DIR}/EmberClash.wasm" COPYONLY)

message(STATUS "Copied ${EMIT_JS}   -> ${WEB_DIR}/EmberClash.js")
message(STATUS "Copied ${EMIT_WASM} -> ${WEB_DIR}/EmberClash.wasm")
