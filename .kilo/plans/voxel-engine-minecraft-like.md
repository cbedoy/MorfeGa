# MorfeGa Voxel Engine — Implementation Plan

## Overview

Convert the existing OpenGL ES 3.0 2D sample into a 3D voxel builder game (Minecraft-like) with first/third person camera, procedural terrain, player physics, and block building/destroying.

## Existing Project Status

- Android Native Activity with GameActivity
- OpenGL ES 3.0 with EGL setup
- Orthographic 2D renderer (single textured quad)
- File structure: `Renderer.h/.cpp`, `Shader.h/.cpp`, `Model.h`, `TextureAsset.h/.cpp`, `Utility.h/.cpp`, `AndroidOut.h/.cpp`, `main.cpp`
- CMake with game-activity dependency

## Phase 1 — 3D Camera & Perspective

### Files to create/modify:

1. **`Camera.h` / `Camera.cpp`** (new)
   - `Vector3` position, `Vector3` target, `Vector3` up
   - `lookAt()` — builds view matrix
   - `perspective()` — builds projection matrix (FOV 70°, near 0.1, far 200)
   - Pitch/yaw with clamping for first-person rotation
   - Mode toggle between first-person and third-person
   - Third-person: camera orbits behind player at configurable distance (default 5 units)

2. **`Renderer.h` / `Renderer.cpp`** (modified)
   - Replace orthographic with perspective projection
   - Add `Camera` member, update `updateRenderArea()` to use perspective
   - Vertex shader: `uProjection` becomes combined `uViewProjection` (view × projection) with model matrix per chunk
   - Fragment shader: add basic directional lighting (sun direction)
   - Shader uniform: `uViewProjection`, `uModel`, `uLightDir`, `uCameraPos`
   - `createModels()` removed (terrain handled by ChunkManager)

3. **`Shader.h` / `Shader.cpp`** (modified)
   - Add uniforms: `uModel` (mat4), `uLightDir` (vec3), `uCameraPos` (vec3)
   - Use `layout(location = ...)` instead of `glGetAttribLocation` for simplicity
   - `drawModel()` now takes model matrix

4. **Shaders** (inline in Renderer or separate)
   - Vertex: transforms by `uViewProjection × uModel`
   - Fragment: simple diffuse lighting `max(dot(normal, lightDir), 0.2) * textureColor`

## Phase 2 — Chunk System & Procedural Terrain

### Files to create:

1. **`BlockTypes.h`** (new)
   ```cpp
   enum class BlockType : uint8_t {
       AIR = 0,
       GRASS,
       DIRT,
       STONE,
       WOOD,
       SAND,
       WATER,
       LEAVES,
       BRICK,
       SNOW,
       COBBLESTONE,
       PLANKS,
       GRAVEL,
       COUNT  // 13 types
   };
   ```

2. **`PerlinNoise.h` / `PerlinNoise.cpp`** (new)
   - 2D/3D Perlin noise implementation (or Simplex)
   - `noise2D(x, z)` returns height value [-1, 1]
   - `noise3D(x, y, z)` for cave/cave-like features (optional)
   - Octave noise (fractal Brownian motion) with 4-6 octaves
   - Configurable seed

3. **`Chunk.h` / `Chunk.cpp`** (new)
   - Size: 16×64×16 (width × height × depth)
   - Stores `BlockType blocks[16][64][16]` as flat `uint8_t` array
   - `getBlock(x, y, z)`, `setBlock(x, y, z, type)`
   - `buildMesh()` — creates VBO/IBO data using greedy meshing
     - For each face direction (posX, negX, posY, negY, posZ, negZ):
       - Skip if neighbor block is same type (no interior faces)
       - Greedy: merge adjacent quads into larger quads
     - Output: interleaved vertex buffer (position 3 × float, normal 3 × float, UV 2 × float) + index buffer (uint16_t)
     - UV coordinates based on block type (atlas lookup)
   - `dirty_` flag: mesh rebuild only on modification
   - VAO/VBO/EBO for rendering

4. **`ChunkManager.h` / `ChunkManager.cpp`** (new)
   - `std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>>`
   - Struct `ChunkCoord { int32_t x, z; }` with hash
   - `getChunk(worldX, worldZ)` — returns or generates
   - `getBlock(worldX, worldY, worldZ)`, `setBlock(worldX, worldY, worldZ, type)`
   - `generateChunk(chunkX, chunkZ)` — uses PerlinNoise:
     - Get terrain height from noise(x, z)
     - Grass on top, dirt below (3-4 layers), stone below that
     - Air above height
   - `update()` — iterates dirty chunks and rebuilds their meshes
   - `render()` — draws all chunks within render distance (~8 chunks radius = 128 blocks)
   - Frustum culling (optional, Phase 5)

## Phase 3 — Player & Physics

### Files to create:

1. **`Player.h` / `Player.cpp`** (new)
   - Position as `Vector3` (float)
   - Velocity as `Vector3`
   - Dimensions: 0.6 × 1.8 × 0.6 (width, height, depth) — AABB
   - Eye height: 1.62 (first-person camera offset)
   - Properties:
     - `isOnGround`, `isJumping`, `movementSpeed = 4.5f`, `jumpVelocity = 8.0f`
     - `reachDistance = 4.0f` (4 blocks)
     - `mode`: FLIGHT (creative-like) or WALK (survival-like) — default WALK
   - Methods:
     - `update(float deltaTime, ChunkManager&)` — gravity, movement, collision
     - `move(Vector3 direction)` — accumulate input for next update
     - `jump()` — if on ground, apply upward velocity
     - `getEyePosition()` — position + eye height
     - `getAABB()` — returns min/max for collision

2. **Collision system** (inside Player.cpp or separate `Collision.h`)
   - AABB vs voxel collision:
     - Sweep velocity along each axis independently
     - For each axis (X, Y, Z):
       - Move along axis by `velocity * deltaTime`
       - Check block occupancy at player's AABB corners
       - If collision, snap position and zero velocity on that axis
     - Block occupancy: check all blocks overlapping the AABB (iterate min..max in world coords)
     - Handle slopes/stairs? Not in v1 — simple full-block collision

3. **Input handling** (modified `main.cpp` and `Renderer::handleInput()`)
   - Touch → look (drag to rotate camera pitch/yaw)
   - Virtual joystick on left half of screen for movement
   - Jump button or swipe up
   - Tap on right half → build/destroy (Phase 4)
   - Camera mode toggle button (first ↔ third person)
   - Store input state in a struct: `InputState { float lookX, lookY; float moveX, moveZ; bool jump; bool build; bool destroy; bool toggleCamera; }`

## Phase 4 — Building & Destroying

### Files to create:

1. **`Raycast.h` / `Raycast.cpp`** (new)
   - `raycastVoxels(Vector3 origin, Vector3 direction, float maxDist, ChunkManager&)`
   - DDA (Digital Differential Analyzer) algorithm for voxel traversal
     - Start at `origin`, step along `direction` through voxel grid
     - At each step, check `chunkManager.getBlock(x, y, z)`
     - Returns: `RaycastHit { bool hit; Vector3 blockPos; Vector3 faceNormal; BlockType type; }`
     - `faceNormal` = which face was entered (needed for block placement)
   - Max iterations: `ceil(maxDist) * 3` (conservative upper bound)

2. **Block interaction** (in `main.cpp` or `Game` class):
   - On tap (AMOTION_EVENT_ACTION_DOWN or UP depending on gesture):
     - Perform raycast from camera center (crosshair)
     - If `hit`:
       - **Break mode (default)**: remove block at `hit.blockPos`
         - `chunkManager.setBlock(blockPos, BlockType::AIR)`
       - **Place mode**: place block adjacent to face
         - `newPos = hit.blockPos + hit.faceNormal`
         - Only if newPos is within bounds and is AIR
         - Use currently selected block type from inventory
       - Mark affected chunk as dirty for mesh rebuild
   - Mode toggle: long-press or button to switch between break/place

3. **Selector highlight** (in Renderer)
   - Draw wireframe cube at currently targeted block position
   - Simple: draw lines using GL_LINES with a shader that ignores depth slightly (offset)
   - Or: overlay a semi-transparent solid cube

4. **Minimal inventory** (in Player or separate `Inventory.h`)
   - `std::vector<BlockType> slots` with at least 6-8 slots
   - `selectedSlot` index
   - Swipe up/down on screen edge or number keys to cycle
   - Render small HUD bar at bottom with block icons (Phase 5)

## Phase 5 — Textures & Polish

### Texture Atlas:

1. **Generate a texture atlas** (or placeholder)
   - 16×16 pixel per block face
   - Atlas grid: 16×16 tiles = 256×256 total (or 32×32 = 512×512 for clarity)
   - Each BlockType maps to a tile coordinate in the atlas
   - Load as PNG asset: `assets/textures/atlas.png`
   - UV logic per face:
     - `u = (tileX + subU) / ATLAS_COLS`
     - `v = (tileY + subV) / ATLAS_ROWS`

2. **UV mapping per block type** (in Chunk::buildMesh):
   - Determine face UV based on BlockType and face direction
   - For grass: top face is grass texture, sides are grass-side, bottom is dirt
   - For most others: all faces use same texture

3. **HUD elements** (basic, in Renderer or separate overlay):
   - **Crosshair**: small + in center of screen
   - **Hotbar**: row of block thumbnails at bottom, selected one highlighted
   - **Block name** (optional): display name of targeted block

4. **Touch controls refinements**:
   - Left side: virtual joystick (touch → drag, direction = displacement from center normalized)
   - Right side: look (touch → drag changes pitch/yaw)
   - Tap right: break/place block
   - Double-tap right: toggle break/place mode
   - Button to toggle camera perspective

## File Structure (final)

```
cpp/
├── CMakeLists.txt                 # Updated with new sources
├── main.cpp                       # Game loop with InputState & Player update
├── AndroidOut.h/.cpp              # (unchanged)
├── Renderer.h/.cpp                # 3D renderer with Camera, lights, chunk rendering
├── Shader.h/.cpp                  # 3D shader with model matrix & lighting
├── Camera.h/.cpp                  # First/third person camera
├── Model.h                        # (unchanged, keep as vertex container)
├── TextureAsset.h/.cpp            # Atlas texture loading (may refactor for atlas)
├── Utility.h/.cpp                 # Add matrix/vector math
├── BlockTypes.h                   # Block type enum
├── Chunk.h/.cpp                   # Voxel storage + mesh generation
├── ChunkManager.h/.cpp            # Chunk lifecycle + terrain generation
├── PerlinNoise.h/.cpp             # Noise generation
├── Player.h/.cpp                  # Player entity + physics
├── Raycast.h/.cpp                 # Voxel ray intersection
├── InputState.h                   # Input struct
└── Inventory.h                    # Block inventory
```

## Dependencies

- GLES 3.0 (already configured)
- game-activity (already configured)
- EGL (already configured)
- Android NDK image decoder (already used in TextureAsset)
- No external libraries — pure C++17

## Build System

- Update `CMakeLists.txt` to include all new `.cpp` files
- Keep `game-activity`, `EGL`, `GLESv3`, `jnigraphics`, `android`, `log` link libraries
- Keep `AndroidManifest.xml` unchanged

## Implementation Order

1. Utility math (Vector3 ops, matrix4x4) → Camera → Update Renderer to 3D perspective
2. BlockTypes → PerlinNoise → Chunk → ChunkManager → terrain rendering
3. Player → physics collision → input handling → movement
4. Raycast → block break/place → selector highlight → inventory
5. Texture atlas → HUD crosshair/hotbar → touch controls polish

## Commits

Each phase will be committed individually with a descriptive message before moving to the next.
