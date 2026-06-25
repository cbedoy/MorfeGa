#ifndef MORFEGA_CHUNKMANAGER_H
#define MORFEGA_CHUNKMANAGER_H

#include <memory>
#include <unordered_map>
#include "Chunk.h"
#include "PerlinNoise.h"
#include "Shader.h"
#include "Camera.h"
#include "Utility.h"

struct ChunkCoord {
    int x, z;
    bool operator==(const ChunkCoord &o) const { return x == o.x && z == o.z; }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord &c) const {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.z) << 16);
    }
};

struct RaycastHit {
    bool hit = false;
    int x = 0, y = 0, z = 0;
    int faceX = 0, faceY = 0, faceZ = 0;
    BlockType type = BlockType::AIR;
};

class ChunkManager {
public:
    ChunkManager();
    ~ChunkManager() = default;

    BlockType getBlock(int worldX, int worldY, int worldZ) const;
    void setBlock(int worldX, int worldY, int worldZ, BlockType type);

    void update();
    void render(const Shader &shader, const Matrix4x4 &vpMatrix, const Camera &camera);

    RaycastHit raycast(const Vector3 &origin, const Vector3 &direction, float maxDist) const;

    int getHeightAt(int worldX, int worldZ) const;

private:
    Chunk *getOrCreateChunk(int chunkX, int chunkZ);
    Chunk *getChunk(int chunkX, int chunkZ) const;
    void generateChunk(Chunk &chunk);
    void ensureChunksAround(const Vector3 &playerPos);

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks_;
    std::unique_ptr<PerlinNoise> noise_;
    int renderDistance_ = 6;
};

#endif
