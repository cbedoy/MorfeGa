# Agent Rules

## Commits
- Every code change must be committed before moving to the next task.
- Commit messages must be descriptive and in English.
- Do not leave uncommitted changes at the end of a work session.

## Code Style
- Follow existing project conventions.
- Do not add comments unless strictly necessary.
- Use C++17 for native code, Kotlin for Android glue.
- Use OpenGL ES 3.0 for graphics. No legacy OpenGL (GL_PROJECTION, glMatrixMode, glBegin/glEnd, etc.).
- Prefer `std::unique_ptr` over raw pointers for ownership.

## Workflow
1. Analyze the requirement.
2. Implement the changes.
3. Verify the code compiles (`./gradlew assembleDebug`).
4. Commit with a descriptive message.
5. Move to the next task.

## Project Conventions
- All block interaction happens through `ChunkManager` (not directly on chunks).
- Player physics: AABB collision resolving per-axis independently (X → Z → Y).
- Chunk meshes rebuild on `dirty_` flag, triggered by `setBlock()`.
- Camera controls: drag-to-look. Tap-to-interact (break block).
- Block coordinates are in world space (integer positions). Chunk-local coordinates derived from world.
- Perlin noise terrain: height = `(noise + 1) * 0.5 * 20 + 8`.
