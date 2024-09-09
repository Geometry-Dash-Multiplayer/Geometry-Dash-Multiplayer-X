cmake_minimum_required(VERSION 3.21)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(GDMX VERSION 1.0.0)

set(
    GDMX_PROJECT_FILES
    "macros"
    "lobbies_popup"
    "lobby_cell"
    "lobby_info"
    "gdmx_manager"
    "player"
    "main"
    "inlevel"
)

foreach (PROJECT_FILE IN LISTS GDMX_PROJECT_FILES)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_FILE}.hpp")
        list(APPEND GDMX_HEADERS "src/${PROJECT_FILE}.hpp")
    endif()
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_FILE}.cpp")
        list(APPEND GDMX_SOURCES "src/${PROJECT_FILE}.cpp")
    endif()
endforeach()

add_library(
    GDMX SHARED 
    ${GDMX_SOURCES}
    ${GDMX_HEADERS}
)

source_group("Source Files\\Main" FILES ${GDMX_SOURCES})
source_group("Header Files\\Main" FILES ${GDMX_HEADERS})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
