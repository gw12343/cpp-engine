cmake_minimum_required(VERSION 3.24.0)
project(cpp-engine VERSION 0.1.0 LANGUAGES C CXX)


# Disable warnings as errors
option(ENABLE_ALL_WARNINGS "Enable all warnings and warnings as errors" OFF)
add_compile_options(-w)

# Or for specific dependencies
set(SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "Build spdlog with warnings" FORCE)


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
  GIT_TAG        master                                        # or pin to a release tag
  SOURCE_SUBDIR  Build                                         # ⚡ crucial!
)
# Disable extras to speed up the build
set(JPH_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)                 # no sample apps
set(JPH_BUILD_TESTS   OFF CACHE BOOL "" FORCE)                 # no unit tests
FetchContent_MakeAvailable(jolt)                               # pulls in Build/CMakeLists.txt



# EnTT is a header-only library, so we don't need to link it
# It's already included in lib/include/entt

include_directories(${PROJECT_NAME} lib/include lib/include/imgui /usr/include lib/src/JoltPhysics/ src)
# Find all source files in src and lib/src directories
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "lib/src/*.cpp"
    "lib/src/*.c"
)

find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})

# Add executable with all source files
add_executable(${PROJECT_NAME} ${SOURCES})


# Link spdlog
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# Link JoltPhysics
target_link_libraries(${PROJECT_NAME} PRIVATE Jolt)

# Link GLFW
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# Link ASSIMP
target_link_libraries(${PROJECT_NAME} PRIVATE assimp)

# Link OpenAL
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENAL_LIBRARY})

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)

include_directories(${SNDFILE_INCLUDE_DIRS})
link_directories(${SNDFILE_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SNDFILE_LIBRARIES})



# Add ozz-animation library
include(FetchContent)
FetchContent_Declare(
  ozz
  GIT_REPOSITORY https://github.com/guillaumeblanc/ozz-animation.git
  GIT_TAG master  # You might want to pin this to a specific version/tag later
)
# Disable building samples and tests to speed up the build
# set(ozz_build_samples OFF CACHE BOOL "" FORCE)
# set(ozz_build_tests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ozz)

# Link ozz-animation
target_link_libraries(${PROJECT_NAME} PRIVATE ozz_animation ozz_animation_offline ozz_geometry)


# Set C++ standard to C++17 (required for EnTT)
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
