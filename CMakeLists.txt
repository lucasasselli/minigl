cmake_minimum_required(VERSION 3.14)

project(minigl C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-Ofast")

option(MINIGL_PNG "PNG support" ON)
option(MINIGL_PNG_MINIZ "PNG using miniz" ON)
option(BUILD_TOOLS "Build tools" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_SHARED_LIBS "Build shared libraires" ON)

if (NOT BUILD_SHARED_LIBS)
    set(SPNG_SHARED OFF CACHE BOOL "")
    set(CGLM_SHARED OFF CACHE BOOL "")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)

if (MINIGL_PNG)
    # https://discourse.cmake.org/t/fetchcontent-a-directory-but-add-a-subdirectory/8603/15
    FetchContent_Declare(
        libspng
        GIT_REPOSITORY https://github.com/randy408/libspng.git
        GIT_TAG master
        SOURCE_SUBDIR this-directory-does-not-exist
    )
    FetchContent_MakeAvailable(libspng)
    add_library(spng
        ${libspng_SOURCE_DIR}/spng/spng.c)
    include_directories(${libspng_SOURCE_DIR}/spng)

    if (MINIGL_PNG_MINIZ)
        add_library(miniz
            ${PROJECT_SOURCE_DIR}/ext/miniz/miniz.c)
        include_directories(${PROJECT_SOURCE_DIR}/ext/miniz)
        target_compile_definitions(miniz PUBLIC -DMINIZ_NO_TIME)

        target_link_libraries(spng PUBLIC miniz)
        target_compile_definitions(spng PUBLIC -DSPNG_USE_MINIZ)
    endif()
endif()

# Fetch cglm library
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cglm)

# MiniGL library
file(GLOB MINIGL_FILES src/*.c)
add_library(minigl
    ${MINIGL_FILES})

if (MINIGL_PNG)
    add_definitions(-DMINIGL_PNG)
    target_include_directories(minigl PUBLIC ${libspng_SOURCE_DIR}/spng)
    target_link_libraries(minigl PUBLIC spng)
endif()

target_include_directories(minigl PUBLIC include)
target_link_libraries(minigl PUBLIC cglm)
target_link_libraries(minigl PUBLIC m)

# Examples
if (BUILD_EXAMPLES)
    # Fetch cgltf library
    FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(cgltf)

    add_executable(gltf
        ${PROJECT_SOURCE_DIR}/examples/gltf/gltf.c)
        target_include_directories(gltf PUBLIC ${cgltf_SOURCE_DIR})
    target_link_libraries(gltf PUBLIC minigl)
endif()

# Tools
if (BUILD_TOOLS)
    add_executable(png2tex 
        ${PROJECT_SOURCE_DIR}/tools/png2tex/main.c)
    target_link_libraries(png2tex PUBLIC minigl)
endif()
