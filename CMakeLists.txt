cmake_minimum_required(VERSION 3.14)

project(minigl C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

option(MINIGL_PNG "PNG support" ON)
option(MINIGL_TOOLS "Build tools" ON)

file(GLOB MINIGL_FILES src/*.c)

if (MINIGL_PNG)
    if (NOT BUILD_SHARED_LIBS)
        set(SPNG_SHARED OFF CACHE BOOL "")
        set(CGLM_SHARED OFF CACHE BOOL "")
    endif()

    add_library(spng
        ${PROJECT_SOURCE_DIR}/ext/libspng/spng/spng.c)

    add_subdirectory(ext/miniz)
    target_link_libraries(spng PUBLIC miniz)
    target_include_directories(spng PUBLIC ext/miniz)
    add_definitions(-DSPNG_USE_MINIZ)
endif()

add_library(minigl
    ${MINIGL_FILES})

if (MINIGL_PNG)
    add_definitions(-DMINIGL_PNG)
    target_include_directories(minigl PUBLIC ext/libspng/spng)
    target_link_libraries(minigl PUBLIC spng)
endif()

add_subdirectory(ext/cglm)

target_include_directories(minigl PUBLIC include)

target_link_libraries(minigl PUBLIC cglm)
target_link_libraries(minigl PUBLIC m)

if (MINIGL_TOOLS)
    add_executable(png2tex 
        ${PROJECT_SOURCE_DIR}/tools/png2tex/main.c)
    target_link_libraries(png2tex PUBLIC minigl)
endif()
