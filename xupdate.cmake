include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats/exec)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats/formats)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../Formats/images)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../XOptions)

include(${CMAKE_CURRENT_LIST_DIR}/../XGithub/xgithub.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../XOptions/xoptions.cmake)

# XUpdate downloads ZIP packages and therefore needs the complete ZIP core.
# Keep that dependency composed through xzip.cmake: it owns XArchive/XZip,
# their family classifiers, shared Formats/XDEX helpers and every decoder
# reachable from XDecompress. A hand-picked copy of that source list drifted
# out of sync and made this standalone consumer fail to compile.
include(${CMAKE_CURRENT_LIST_DIR}/../XArchive/xzip.cmake)

set(XUPDATE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xupdate.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xupdate.h
    ${XZIP_SOURCES}
    ${XGITHUB_SOURCES}
    ${XOPTIONS_SOURCES}
)
