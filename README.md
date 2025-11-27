# cpp-engine

A modern C++ game engine with physics, audio, animation, and rendering capabilities.

## Overview

cpp-engine is a lightweight, modular game engine built in C++ that provides a comprehensive set of features for game development:

- Entity Component System (ECS) architecture
- 3D rendering with OpenGL
- Physics simulation using Jolt Physics
- Audio system with OpenAL and libsndfile
- Animation system using ozz-animation
- ImGui-based editor interface

## Dependencies

The engine relies on the following third-party libraries:

- **[EnTT](https://github.com/skypjack/entt)**: Modern entity-component-system (ECS) implementation
- **[Jolt Physics](https://github.com/jrouwe/JoltPhysics)**: High-performance physics engine
- **[GLFW](https://www.glfw.org/)**: Cross-platform window and input handling
- **[GLM](https://github.com/g-truc/glm)**: Mathematics library for graphics programming
- **[spdlog](https://github.com/gabime/spdlog)**: Fast C++ logging library
- **[Assimp](https://github.com/assimp/assimp)**: Open Asset Import Library for 3D model loading
- **[OpenAL](https://www.openal.org/)**: Cross-platform 3D audio API
- **[libsndfile](http://libsndfile.github.io/libsndfile/)**: Library for reading/writing audio files
- **[ozz-animation](https://github.com/guillaumeblanc/ozz-animation)**: Skeletal animation library
- **[Dear ImGui](https://github.com/ocornut/imgui)**: Immediate mode GUI library

## Building

### Prerequisites

- CMake 3.24 or higher
- C++17 compatible compiler
- Required libraries (most are fetched automatically by CMake)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/gw12343/cpp-engine.git
cd cpp-engine

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .
```

## Project Structure

- **src/core**: Core engine systems (Engine, Window, Input, etc.)
- **src/rendering**: Rendering system (Renderer, Shader, Texture, etc.)
- **src/physics**: Physics integration with Jolt Physics
- **src/sound**: Audio system using OpenAL
- **src/animation**: Animation system using ozz-animation
- **src/ui**: ImGui-based user interface
- **src/components**: Component definitions for the ECS
- **src/utils**: Utility functions and helpers

## Usage

```cpp
#include <core/Engine.h>
using namespace Engine;

int main() {
    GEngine engine(1600*2, 1200*2, "My Game");
    
    if (!engine.Initialize()) {
        return -1;
    }
    
    engine.Run();
    return 0;
}
```

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE.md](LICENSE.md) file for details.
