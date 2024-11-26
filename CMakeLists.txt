cmake_minimum_required(VERSION 3.21)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
include(cmake/CPM.cmake)

project(GDMX VERSION 1.0.0)

set(
    GDMX_CORE_PROJECT_FILES
    "gdmx_manager"
)

set(
    GDMX_HOOKS_PROJECT_FILES
    "game_layer"
    "menu_layer"
    "player_object"
    "effect_manager"
)

set(
    GDMX_MODELS_PROJECT_FILES
    "player"
    "lobby"
    "sync"
)

set(
    GDMX_NET_PROJECT_FILES
    "constants"
    "listener"
    "request"
)

set(
    GDMX_UI_PROJECT_FILES
    "lobbies_popup"
    "lobby_cell"
    "create_lobby_popup"
    "options_menu"
)

set(
    GDMX_UTILS_PROJECT_FILES
    "macros"
    "spawn"
    "logging"
)

set(
    GDMX_SERIALIZATION_PROJECT_FILES
    "cocos2d"
)

foreach (DIR "core" "hooks" "models" "net" "ui" "utils" "serialization")
    string(TOUPPER ${DIR} DIR_UPPER)
    set(PROJECT_FILES_VAR "GDMX_${DIR_UPPER}_PROJECT_FILES")
    foreach (SRC IN LISTS ${PROJECT_FILES_VAR})
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${DIR}/${SRC}.hpp")
            list(APPEND "GDMX_${DIR_UPPER}_HEADERS" "src/${DIR}/${SRC}.hpp")
            list(APPEND GDMX_HEADERS "src/${DIR}/${SRC}.hpp")
        endif()
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${DIR}/${SRC}.cpp")
            list(APPEND "GDMX_${DIR_UPPER}_SOURCES" "src/${DIR}/${SRC}.cpp")
            list(APPEND GDMX_SOURCES "src/${DIR}/${SRC}.cpp")
        endif()
    endforeach()
endforeach()

add_library(
    GDMX SHARED
    ${GDMX_SOURCES}
    ${GDMX_HEADERS}
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

message(STATUS "Setting up boost")
set(BOOST_INCLUDE_LIBRARIES asio serialization unordered math program_options)
CPMAddPackage(
    NAME Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.7z
    URL_MD5 c33bf9137ff5ff1d3d0f28af212c9704
	DOWNLOAD_EXTRACT_TIMESTAMP ON
)

CPMAddPackage("gh:AngelDev06/eos-portable-archive#73bf18f")

set_target_properties(
    boost_serialization 
    boost_wserialization
PROPERTIES
    FOLDER "Dependencies"
)

foreach (boost_lib "thread" "serialization")
    target_compile_definitions(
        "boost_${boost_lib}"
    PRIVATE
        _HAS_ITERATOR_DEBUGGING=0
    )
endforeach()

target_link_libraries(
    GDMX
    Boost::asio
    Boost::serialization
    Boost::unordered
    eos::portable_archive
)

target_include_directories(
    GDMX
PRIVATE
    src
    src/core
)

target_compile_definitions(
    GDMX
PRIVATE
    $<$<BOOL:${WIN32}>:_WIN32_WINNT=0x0601>
)

target_compile_options(
    GDMX
PRIVATE
    $<$<BOOL:${MSVC}>:/W3>
)

target_precompile_headers(
    GDMX
PRIVATE
    <boost/asio.hpp>
    <boost/asio/experimental/awaitable_operators.hpp>
)

foreach (GROUP "Core" "Hooks" "Models" "Net" "UI" "Utils" "Serialization")
    string(TOUPPER ${GROUP} GROUP_UPPER)
    set(HEADERS_LIST_VAR "GDMX_${GROUP_UPPER}_HEADERS")
    set(SOURCES_LIST_VAR "GDMX_${GROUP_UPPER}_SOURCES")
    source_group("Header Files\\${GROUP}" FILES ${${HEADERS_LIST_VAR}})
    source_group("Source Files\\${GROUP}" FILES ${${SOURCES_LIST_VAR}})
endforeach()

set_target_properties(
    GeodeBindings
    GDMX_PACKAGE
    fmt
    boost_program_options
PROPERTIES
    FOLDER "Dependencies")

if (ANDROID)
    find_program(ADB_CMD adb)
    if (ADB_CMD)
        add_custom_target(
            GDMXDeploy ALL
            "${ADB_CMD}" push
                $<TARGET_FILE_DIR:GDMX>/angel.gdmx.geode
                /storage/emulated/0/Android/media/com.geode.launcher/game/geode/mods/ ||
            (echo Failed to push to device && exit 0)
            VERBATIM USES_TERMINAL
        )
        add_dependencies(GDMXDeploy GDMX)
        set_property(TARGET GDMXDeploy PROPERTY FOLDER "Dependencies")
    endif()
endif()
