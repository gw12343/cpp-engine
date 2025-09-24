# ----------------- Build Options -----------------
cmake_minimum_required(VERSION 3.24)
project(cpp-engine VERSION 0.1.0 LANGUAGES C CXX)


option(GAME_BUILD "Compile engine as game build (stripped for game)" OFF)

if (GAME_BUILD)
    message(STATUS "Configuring GAME_BUILD")
    add_compile_definitions(GAME_BUILD)
    set(TARGET_NAME "${PROJECT_NAME}_game")
else ()
    set(TARGET_NAME "${PROJECT_NAME}")
endif ()

# ----------------- Detect Emscripten -----------------
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS "Compiling for Web (Emscripten)")
    set(BUILD_FOR_WEB ON)
else ()
    set(BUILD_FOR_WEB OFF)
endif ()

# ----------------- Web Flags -----------------
if (BUILD_FOR_WEB)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
        -sALLOW_MEMORY_GROWTH=1 \
        -sINITIAL_MEMORY=64MB \
        -sMAXIMUM_MEMORY=1024MB \
        -sSTACK_SIZE=5MB \
        -g4 \
        -sUSE_WEBGL2=1 \
        -sFULL_ES3=1 \
        -sSAFE_HEAP=1 \
        -sUSE_PTHREADS=0 \
        -sASSERTIONS=2 \
        -sSTACK_OVERFLOW_CHECK=1")

    # preload folders only if they exist
    if (EXISTS "${CMAKE_SOURCE_DIR}/resources")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/resources@/resources")
    endif ()

    if (EXISTS "${CMAKE_SOURCE_DIR}/scenes")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/scenes@/scenes")
    endif ()

    if (EXISTS "${CMAKE_SOURCE_DIR}/scripts")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/scripts@/scripts")
    endif ()
endif ()

# ----------------- Compiler settings -----------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ALL_WARNINGS "Enable all warnings and warnings as errors" OFF)
add_compile_options(-w)

if (WIN32)
    add_compile_definitions(NOMINMAX)
endif ()

option(USE_ASSIMP_FROM_SOURCE "Build and use Assimp 6.0.2 from source" ON)

# ----------------- Global dependency flags -----------------
set(SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(JPH_USE_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ozz_build_samples OFF CACHE BOOL "" FORCE)
set(ozz_build_tests OFF CACHE BOOL "" FORCE)
set(ozz_build_tools OFF CACHE BOOL "" FORCE)
set(ozz_build_howtos OFF CACHE BOOL "" FORCE)
set(ozz_build_gltf OFF CACHE BOOL "" FORCE)
set(ozz_build_postfix OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)
set(BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

# ----------------- Fetch external libraries -----------------
include(FetchContent)

# Assimp (works with Emscripten if we enable its zlib)
if (USE_ASSIMP_FROM_SOURCE)
    FetchContent_Declare(
            assimp_src
            GIT_REPOSITORY https://github.com/assimp/assimp.git
            GIT_TAG v6.0.2
    )

    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    if (BUILD_FOR_WEB)
        set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    else ()
        set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
    endif ()


    FetchContent_MakeAvailable(assimp_src)
endif ()

# Core dependencies (safe everywhere)
FetchContent_Declare(cereal GIT_REPOSITORY https://github.com/USCiLab/cereal.git GIT_TAG v1.3.2)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.12.0)
FetchContent_Declare(tracy GIT_REPOSITORY https://github.com/wolfpld/tracy.git GIT_TAG v0.12.2 GIT_SHALLOW TRUE)
FetchContent_Declare(jolt GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git GIT_TAG master SOURCE_SUBDIR Build)
FetchContent_Declare(ozz GIT_REPOSITORY https://github.com/guillaumeblanc/ozz-animation.git GIT_TAG master)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_Declare(effekseer GIT_REPOSITORY https://github.com/Effekseer/Effekseer.git GIT_TAG master)
FetchContent_Declare(lua URL https://www.lua.org/ftp/lua-5.4.6.tar.gz)
FetchContent_Declare(sol2 GIT_REPOSITORY https://github.com/ThePhD/sol2.git GIT_TAG c1f95a7)
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)
FetchContent_Declare(efsw GIT_REPOSITORY https://github.com/SpartanJ/efsw.git GIT_TAG master)

if (NOT BUILD_FOR_WEB)
    FetchContent_Declare(
            nativefiledialog
            GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog.git
            GIT_TAG release_116
    )
endif ()

set(EFSW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(EFSW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(cereal spdlog jolt ozz imgui effekseer lua sol2 nlohmann_json efsw tracy)
if (NOT BUILD_FOR_WEB)
    FetchContent_MakeAvailable(nativefiledialog)


    add_library(nfd STATIC
            ${nativefiledialog_SOURCE_DIR}/src/nfd_common.c
    )

    if (WIN32)
        target_sources(nfd PRIVATE ${nativefiledialog_SOURCE_DIR}/src/nfd_win.cpp)
    else () # Linux
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
        include_directories(${GTK3_INCLUDE_DIRS})
        link_directories(${GTK3_LIBRARY_DIRS})
        add_definitions(${GTK3_CFLAGS_OTHER})

        target_sources(nfd PRIVATE ${nativefiledialog_SOURCE_DIR}/src/nfd_gtk.c)
        target_link_libraries(nfd PRIVATE ${GTK3_LIBRARIES})
    endif ()

    target_include_directories(nfd PUBLIC ${nativefiledialog_SOURCE_DIR}/src/include)
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
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(imgui PUBLIC IMGUI_HAS_DOCK)

# ----------------- LUA -------------------
file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/src/*.c)
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)

# ----------------- Sources -------------------
file(GLOB_RECURSE SOURCES
        "lib/src/*.cpp"
        "lib/src/*.c"
        "src/*.cpp"
        "src/*.inl"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${lua_SOURCE_DIR}/src
)

# ----------------- Platform-specific -------------------
if (BUILD_FOR_WEB)
    message(STATUS "Using Emscripten's built-in GLFW & WebGL")
    target_link_options(${PROJECT_NAME} PRIVATE "-sUSE_GLFW=3")
    target_link_options(${PROJECT_NAME} PRIVATE "-sUSE_WEBGL2=1" "-sFULL_ES3=1")
else ()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    find_package(glfw3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
    target_link_libraries(imgui PRIVATE glfw)
endif ()

# Skip desktop-only libs on web
if (NOT BUILD_FOR_WEB)
    target_link_libraries(${PROJECT_NAME} PRIVATE nfd EffekseerRendererGL)
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

# ----------------- Link Everything Else -------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
        lua
        sol2::sol2
        imgui
        cereal::cereal
        spdlog::spdlog
        Jolt
        ozz_animation ozz_animation_offline ozz_geometry
        Effekseer
        nlohmann_json::nlohmann_json
        efsw
        Tracy::TracyClient
)

# Effekseer includes
target_include_directories(${PROJECT_NAME} PRIVATE
        ${effekseer_SOURCE_DIR}/Dev/Cpp
        ${effekseer_SOURCE_DIR}/Dev/Cpp/Effekseer
        ${effekseer_SOURCE_DIR}/Dev/Cpp/EffekseerRendererCommon
)

if (NOT BUILD_FOR_WEB)
    target_include_directories(${PROJECT_NAME} PRIVATE
            ${effekseer_SOURCE_DIR}/Dev/Cpp/EffekseerRendererGL
    )
endif ()

# ----------------- HTML Output -------------------
if (DEFINED EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME}
            PROPERTIES SUFFIX ".html"
            LINK_FLAGS " --bind -s USE_GLFW=3 -s WASM=1 -g4"
    )
endif ()
