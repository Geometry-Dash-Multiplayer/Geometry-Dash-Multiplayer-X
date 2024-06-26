cmake_minimum_required(VERSION 3.21)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(GDMX VERSION 1.0.0)

foreach (HEADER "macros")
    list(APPEND GDMX_HEADERS "headers/${HEADER}.h")
endforeach()

foreach (SRC "main")
    list(APPEND GDMX_SOURCES "src/${SRC}.cpp")
endforeach()

add_library(
    GDMX SHARED 
    ${GDMX_SOURCES}
    ${GDMX_HEADERS}
)

target_include_directories(
    GDMX PRIVATE
    "${PROJECT_SOURCE_DIR}/headers"
)

source_group("Source Files\\General" FILES ${GDMX_SOURCES})
source_group("Header Files\\General" FILES ${GDMX_HEADERS})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
