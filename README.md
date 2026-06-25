# MorfeGa

MorfeGa is a voxel-based 3D engine for Android, built with OpenGL ES 3.0 and C++ via Android NDK.

## Features

- 3D voxel world with procedural terrain generation (Perlin noise)
- Chunk-based system (16×64×16 blocks per chunk) with render distance culling
- Face culling and greedy mesh building for optimal performance
- 13 block types: AIR, GRASS, DIRT, STONE, WOOD, SAND, WATER, LEAVES, BRICK, SNOW, COBBLESTONE, PLANKS, GRAVEL
- First-person and third-person camera modes (toggle)
- Touch controls: drag to look around, tap to break blocks
- Player physics with AABB-voxel collision, gravity, and jumping
- Raycasting for block selection and interaction (4 block reach)
- Selector wireframe highlight on targeted block
- Crosshair HUD
- Basic directional lighting

## Requirements

- Android Studio
- JDK 17+
- Android SDK (API 35)
- Android NDK 28+
- CMake 3.22.1+

## Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/user/MorfeGa.git
   ```
2. Open the project in Android Studio.
3. Sync Gradle dependencies.
4. Run the app on an emulator (API 35) or physical device.

## Controls

- **Drag** anywhere on screen → look around (camera rotation)
- **Tap** → break the targeted block
- **Space** (keyboard/ADB) → jump

## Project Structure

```
├── app/
│   ├── src/main/
│   │   ├── cpp/              # C++ native code
│   │   │   ├── main.cpp             # Game loop entry point
│   │   │   ├── AndroidOut.h/.cpp    # Logcat output
│   │   │   ├── BlockTypes.h         # Block type enum
│   │   │   ├── Camera.h/.cpp        # 3D camera (first/third person)
│   │   │   ├── Chunk.h/.cpp         # Voxel chunk with mesh building
│   │   │   ├── ChunkManager.h/.cpp  # Chunk lifecycle + terrain generation
│   │   │   ├── Model.h              # Vertex/Index buffer container
│   │   │   ├── PerlinNoise.h/.cpp   # Noise generation
│   │   │   ├── Player.h/.cpp        # Player physics and movement
│   │   │   ├── Renderer.h/.cpp      # 3D renderer with lighting
│   │   │   ├── Shader.h/.cpp        # Shader program management
│   │   │   ├── TextureAsset.h/.cpp  # Texture loading
│   │   │   ├── Utility.h/.cpp       # Vector3/Matrix4x4 math
│   │   │   └── CMakeLists.txt       # CMake build config
│   │   ├── java/.../MainActivity.kt # Kotlin entry via GameActivity
│   │   └── AndroidManifest.xml
│   └── build.gradle.kts
├── .kilo/plans/              # Implementation plans
├── AGENTS.md                 # Agent development rules
├── README.md
├── build.gradle.kts
└── settings.gradle.kts
```

## Technical Stack

- **Language**: C++17 (native), Kotlin (Android glue)
- **Graphics**: OpenGL ES 3.0
- **Build**: CMake + Android NDK
- **Game Framework**: GameActivity (Android Games SDK)
- **Rendering**: EGL context, VAO/VBO/EBO, indexed triangle meshes

## License

All rights reserved.
