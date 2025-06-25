cmake_minimum_required(VERSION 3.24)
project(cpp-engine VERSION 0.1.0 LANGUAGES C CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ALL_WARNINGS "Enable all warnings and warnings as errors" OFF)
add_compile_options(-w)

# Global dependency flags
set(SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ozz_build_samples OFF CACHE BOOL "" FORCE)
set(ozz_build_tests OFF CACHE BOOL "" FORCE)
set(ozz_build_howtos OFF CACHE BOOL "" FORCE)
set(ozz_build_gltf OFF CACHE BOOL "" FORCE)
set(ozz_build_postfix OFF CACHE BOOL "" FORCE)

# Fetch external libraries
include(FetchContent)

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

FetchContent_MakeAvailable(spdlog jolt ozz imgui effekseer lua sol2)

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
        ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(imgui PUBLIC IMGUI_HAS_DOCK)
target_link_libraries(imgui PRIVATE glfw)

# ----------------- LUA -------------------
file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/src/*.c)
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)

# ----------------- Source & Includes -------------------
file(GLOB_RECURSE LIB_SOURCES "lib/src/*.cpp" "lib/src/*.c" "src/*.cpp" "src/*.inl")

add_executable(${PROJECT_NAME} ${LIB_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        lib/include
        lib/src/JoltPhysics
        src
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
    find_path(OPENAL_INCLUDE_DIR AL/al.h)
    find_library(OPENAL_LIBRARY OpenAL32)
else ()
    find_package(OpenAL REQUIRED)
endif ()

if (OPENAL_INCLUDE_DIR AND OPENAL_LIBRARY)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENAL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENAL_LIBRARY})
else ()
    message(WARNING "OpenAL not found. Audio may not work!")
endif ()

# SndFile (Linux/macOS only; needs vcpkg or manual install on Windows)
if (NOT WIN32)
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
        assimp
        ozz_animation ozz_animation_offline ozz_geometry
        Effekseer
        EffekseerRendererGL
)

# ----------------- Debug Info -------------------
get_target_property(LINK_LIBS ${PROJECT_NAME} LINK_LIBRARIES)
message(STATUS "${PROJECT_NAME} links: ${LINK_LIBS}")
