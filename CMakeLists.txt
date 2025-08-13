cmake_minimum_required(VERSION 3.24)
project(cpp-engine VERSION 0.1.0 LANGUAGES C CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ALL_WARNINGS "Enable all warnings and warnings as errors" OFF)
add_compile_options(-w)

if (WIN32)
    add_compile_definitions(NOMINMAX)
endif ()

# Global dependency flags
set(SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(JPH_USE_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ozz_build_samples OFF CACHE BOOL "" FORCE)
set(ozz_build_tests OFF CACHE BOOL "" FORCE)
set(ozz_build_howtos OFF CACHE BOOL "" FORCE)
set(ozz_build_gltf OFF CACHE BOOL "" FORCE)
set(ozz_build_postfix OFF CACHE BOOL "" FORCE)
set(OPENAL_ROOT "C:/Users/Gabe/Documents/openal-soft-1.24.3-bin")  # Change this if needed
option(USE_ASSIMP_FROM_SOURCE "Build and use Assimp 6.0.2 from source" ON)


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Use MDd in Debug, MD in Release
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/MDd)
    else ()
        add_compile_options(/MD)
    endif ()
endif ()


# Fetch external libraries
include(FetchContent)


if (USE_ASSIMP_FROM_SOURCE)
    include(FetchContent)
    FetchContent_Declare(
            assimp_src
            GIT_REPOSITORY https://github.com/assimp/assimp.git
            GIT_TAG v6.0.2
    )
    FetchContent_GetProperties(assimp_src)
    if (NOT assimp_src_POPULATED)
        FetchContent_Populate(assimp_src)
        set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT ON CACHE BOOL "" FORCE)
        set(PKG_CONFIG_USE_CMAKE OFF CACHE BOOL "" FORCE)
        add_subdirectory(${assimp_src_SOURCE_DIR} ${assimp_src_BINARY_DIR})
    endif ()
endif ()

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
)

FetchContent_Declare(
        jolt
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG master
        SOURCE_SUBDIR Build
)

FetchContent_Declare(
        ozz
        GIT_REPOSITORY https://github.com/guillaumeblanc/ozz-animation.git
        GIT_TAG master
)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
)

FetchContent_Declare(
        effekseer
        GIT_REPOSITORY https://github.com/Effekseer/Effekseer.git
        GIT_TAG master
)

FetchContent_Declare(
        lua
        URL https://www.lua.org/ftp/lua-5.4.6.tar.gz
)

FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG c1f95a773c6f8f4fde8ca3efe872e7286afe4444
)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3 # or the latest stable version you prefer
)

FetchContent_MakeAvailable(spdlog jolt ozz imgui effekseer lua sol2 nlohmann_json)


if (WIN32)
    FetchContent_Declare(
            sndfile
            GIT_REPOSITORY https://github.com/libsndfile/libsndfile.git
            GIT_TAG 1.2.2
    )

    set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "" FORCE)  # Avoid building FLAC/MP3/VORBIS, optional
    set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)        # No command-line tools
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(sndfile)
endif ()


# ----------------- IMGUI -------------------
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
        ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(imgui PUBLIC IMGUI_HAS_DOCK)

# ----------------- LUA -------------------
file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/src/*.c)
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)

# ----------------- Source & Includes -------------------
file(GLOB_RECURSE SOURCES "lib/src/*.cpp" "lib/src/*.c" "src/*.cpp" "src/*.inl")

add_executable(${PROJECT_NAME} ${SOURCES}
        src/rendering/ui/ModelPreview.h
        src/rendering/ui/ModelPreview.cpp
        src/components/impl/TerrainRendererComponent.cpp
        src/components/impl/TerrainRendererComponent.h
        src/assets/impl/TerrainLoader.cpp
        src/assets/impl/TerrainLoader.h
        src/terrain/TerrainTile.cpp
        src/terrain/TerrainTile.h
        src/assets/AssetMeta.h
)


target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/src/JoltPhysics
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/include/JoltPhysics
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${lua_SOURCE_DIR}/src
)

# Effekseer
target_include_directories(${PROJECT_NAME} PRIVATE
        ${effekseer_SOURCE_DIR}/Dev/Cpp
        ${effekseer_SOURCE_DIR}/Dev/Cpp/Effekseer
        ${effekseer_SOURCE_DIR}/Dev/Cpp/EffekseerRendererGL
        ${effekseer_SOURCE_DIR}/Dev/Cpp/EffekseerRendererCommon
)

# ----------------- Platform-specific Dependencies -------------------

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# OpenAL
if (WIN32)
    set(OPENAL_LIBRARY "${OPENAL_ROOT}/libs/Win64/OpenAL32.lib")
    set(OPENAL_INCLUDE_DIR "${OPENAL_ROOT}/include")

    if (EXISTS "${OPENAL_LIBRARY}" AND EXISTS "${OPENAL_INCLUDE_DIR}/AL/al.h")
        message(STATUS "Using manually specified OpenAL from ${OPENAL_ROOT}")
    else ()
        message(FATAL_ERROR "OpenAL not found! Please set OPENAL_ROOT to the path where OpenAL Soft is installed.")
    endif ()

    include_directories(${OPENAL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE "${OPENAL_LIBRARY}")
else ()
    find_package(OpenAL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENAL_LIBRARY})
endif ()


if (WIN32)
    # Use built-in or bundled .lib
    set(ASSIMP_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/assimp.lib")
    if (EXISTS "${ASSIMP_LIB}")
        message(STATUS "Using bundled Assimp on Windows: ${ASSIMP_LIB}")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${ASSIMP_LIB}")
    else ()
        find_package(ASSIMP REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
    endif ()
else ()
    if (USE_ASSIMP_FROM_SOURCE)
        # Use the built-in CMake target from our fetch/build
        target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
    else ()
        find_package(ASSIMP 6.0.2 REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
    endif ()
endif ()

if (WIN32)
    set(GLFW_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw3.lib")
    set(GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/include")  # Optional if needed
    if (EXISTS "${GLFW_LIB}")
        message(STATUS "Using manually specified GLFW at ${GLFW_LIB}")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${GLFW_LIB}")
        target_link_libraries(imgui PRIVATE "${GLFW_LIB}")
        target_include_directories(${PROJECT_NAME} PRIVATE "${GLFW_INCLUDE_DIR}")
    else ()
        message(FATAL_ERROR "GLFW library not found at ${GLFW_LIB}")
    endif ()
else ()
    find_package(glfw3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
    target_link_libraries(imgui PRIVATE glfw)
endif ()


include_directories(${OPENAL_INCLUDE_DIR})

# SndFile
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE sndfile)
else ()
    # Linux/macOS
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SNDFILE REQUIRED sndfile)
    include_directories(${SNDFILE_INCLUDE_DIRS})
    link_directories(${SNDFILE_LIBRARY_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SNDFILE_LIBRARIES})
endif ()

# ----------------- Link Everything Else -------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
        lua
        sol2::sol2
        imgui
        spdlog::spdlog
        Jolt
        ozz_animation ozz_animation_offline ozz_geometry
        Effekseer
        EffekseerRendererGL
        nlohmann_json::nlohmann_json
)

# ----------------- Debug Info -------------------
get_target_property(LINK_LIBS ${PROJECT_NAME} LINK_LIBRARIES)
message(STATUS "${PROJECT_NAME} links: ${LINK_LIBS}")
