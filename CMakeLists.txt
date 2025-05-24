cmake_minimum_required(VERSION 3.24.0)
project(cpp-engine VERSION 0.1.0 LANGUAGES C CXX)


# Disable warnings as errors
option(ENABLE_ALL_WARNINGS "Enable all warnings and warnings as errors" OFF)
add_compile_options(-w)

# Or for specific dependencies
set(SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "Build spdlog with warnings" FORCE)


################################### ASSIMP #####################################################################
# Assimp is a library for importing and exporting 3D model formats. It supports a wide range of file formats,  #
# including OBJ, FBX, COLLADA, and many others. Assimp provides a unified interface for loading 3D models,	   #
# allowing developers to work with different file formats without having to write custom importers.			   #
################################################################################################################
include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)


include(FetchContent)                                           # include the module
FetchContent_Declare(
        jolt
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG master                                        # or pin to a release tag
        SOURCE_SUBDIR Build                                         # ⚡ crucial!
)
# Disable extras to speed up the build
set(JPH_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)                 # no sample apps
set(JPH_BUILD_TESTS OFF CACHE BOOL "" FORCE)                 # no unit tests
FetchContent_MakeAvailable(jolt)                               # pulls in Build/CMakeLists.txt


#################################### OZZ ANIMATION ##########################################
# Ozz Animation is a library for 3D animation and character animation.						#
# It provides a set of tools and utilities for working with skeletal animation, including	#
# animation blending, skinning, and animation retargeting.									#
#############################################################################################
include(FetchContent)
FetchContent_Declare(
        ozz
        GIT_REPOSITORY https://github.com/guillaumeblanc/ozz-animation.git
        GIT_TAG master
)
set(ozz_build_tools OFF CACHE BOOL "" FORCE)
set(ozz_build_samples OFF CACHE BOOL "" FORCE)
set(ozz_build_tests OFF CACHE BOOL "" FORCE)
set(ozz_build_howtos OFF CACHE BOOL "" FORCE)
set(ozz_build_gltf OFF CACHE BOOL "" FORCE)
set(ozz_build_postfix OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ozz)

#################################### IMGUI ####################################################
include(FetchContent)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking         # Use the docking branch
)

FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp

        # Backends
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PRIVATE glfw)

target_compile_definitions(imgui PUBLIC IMGUI_HAS_DOCK)

############################### EFFEKSEER ###################################
include(FetchContent)

# Fetch Effekseer
FetchContent_Declare(
        effekseer
        GIT_REPOSITORY https://github.com/Effekseer/Effekseer.git
        GIT_TAG master
)

# Optional: make it faster by shallow cloning
#set(FETCHCONTENT_QUIET ON)
#set(FETCHCONTENT_SOURCE_DIR_EFFEKSEER "${CMAKE_BINARY_DIR}/_deps/effekseer-src")
#(effekseer)
FetchContent_MakeAvailable(effekseer)
#target_link_libraries(effekseer PRIVATE GL)

############################################################################

# EnTT is a header-only library, so we don't need to link it
# It's already included in lib/include/entt

include_directories(${PROJECT_NAME} lib/include /usr/include lib/src/JoltPhysics/ src)
# Find all source files in src and lib/src directories
file(GLOB_RECURSE SOURCES
        "src/*.cpp"
        "lib/src/*.cpp"
        "lib/src/*.c"
)

find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})


# Add executable with all source files
add_executable(${PROJECT_NAME} ${SOURCES}
        src/rendering/particles/ParticleManager.h
        src/rendering/particles/ParticleManager.cpp
        src/terrain/TerrainLoader.h
        src/terrain/TerrainLoader.cpp
        src/terrain/TerrainManager.cpp
        src/terrain/TerrainManager.h
        lib/include/stb/stb_image_write.h)
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
# Link spdlog
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# Link JoltPhysics
target_link_libraries(${PROJECT_NAME} PRIVATE Jolt)

# Link GLFW
#target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# Link ASSIMP
target_link_libraries(${PROJECT_NAME} PRIVATE assimp)

# Link OpenAL
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENAL_LIBRARY})

#target_link_libraries(YourGame PRIVATE Effekseer::Effekseer Effekseer::EffekseerRendererGL)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)

include_directories(${SNDFILE_INCLUDE_DIRS})
link_directories(${SNDFILE_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SNDFILE_LIBRARIES})

# Link ozz-animation
target_link_libraries(${PROJECT_NAME} PUBLIC ozz_animation ozz_animation_offline ozz_geometry)


target_include_directories(${PROJECT_NAME} PRIVATE
        ${effekseer_SOURCE_DIR}/Dev/Cpp
        ${effekseer_SOURCE_DIR}/Dev/Cpp/Effekseer
        ${effekseer_SOURCE_DIR}/Dev/Cpp/EffekseerRendererGL
        ${effekseer_SOURCE_DIR}/Dev/Cpp/EffekseerRendererCommon
)

target_link_libraries(${PROJECT_NAME} PRIVATE Effekseer EffekseerRendererGL)

# Set C++ standard to C++17 (required for EnTT)
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)
# Print what we're linking
get_target_property(LINK_LIBS cpp-engine LINK_LIBRARIES)
message(STATUS " cpp-engine links: ${LINK_LIBS} ")