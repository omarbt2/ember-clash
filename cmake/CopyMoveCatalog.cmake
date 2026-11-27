# Copy the validated move catalog into the runtime asset layout.
# Inputs: MOVE_CATALOG (source JSON), DESTINATION_ROOT (app/web root).
if(NOT MOVE_CATALOG OR NOT DESTINATION_ROOT)
    message(FATAL_ERROR "CopyMoveCatalog.cmake: MOVE_CATALOG and DESTINATION_ROOT are required")
endif()
if(NOT EXISTS "${MOVE_CATALOG}")
    message(FATAL_ERROR "CopyMoveCatalog.cmake: source catalog does not exist: ${MOVE_CATALOG}")
endif()

file(MAKE_DIRECTORY "${DESTINATION_ROOT}/assets/data")
configure_file("${MOVE_CATALOG}" "${DESTINATION_ROOT}/assets/data/moves.json" COPYONLY)
message(STATUS "Copied move catalog -> ${DESTINATION_ROOT}/assets/data/moves.json")
