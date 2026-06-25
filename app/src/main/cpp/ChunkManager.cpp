#include "ChunkManager.h"
#include <algorithm>
#include <cmath>
#include <cstring>

ChunkManager::ChunkManager()
    : noise_(std::make_unique<PerlinNoise>(12345)) {}

BlockType ChunkManager::getBlock(int worldX, int worldY, int worldZ) const {
    if (worldY < 0 || worldY >= kChunkHeight) return BlockType::AIR;
    int cx = worldX >= 0 ? worldX / kChunkWidth : (worldX - kChunkWidth + 1) / kChunkWidth;
    int cz = worldZ >= 0 ? worldZ / kChunkDepth : (worldZ - kChunkDepth + 1) / kChunkDepth;
    auto it = chunks_.find(ChunkCoord{cx, cz});
    if (it == chunks_.end()) return BlockType::AIR;
    return it->second->getBlockWorld(worldX, worldY, worldZ);
}

void ChunkManager::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    if (worldY < 0 || worldY >= kChunkHeight) return;
    int cx = worldX >= 0 ? worldX / kChunkWidth : (worldX - kChunkWidth + 1) / kChunkWidth;
    int cz = worldZ >= 0 ? worldZ / kChunkDepth : (worldZ - kChunkDepth + 1) / kChunkDepth;
    Chunk *chunk = getOrCreateChunk(cx, cz);
    if (chunk) {
        chunk->setBlockWorld(worldX, worldY, worldZ, type);
        // Mark neighbor chunks dirty if block is on the edge
        int lx = worldX - cx * kChunkWidth;
        int lz = worldZ - cz * kChunkDepth;
        if (lx == 0) {
            Chunk *neighbor = getChunk(cx - 1, cz);
            if (neighbor) neighbor->setDirty();
        }
        if (lx == kChunkWidth - 1) {
            Chunk *neighbor = getChunk(cx + 1, cz);
            if (neighbor) neighbor->setDirty();
        }
        if (lz == 0) {
            Chunk *neighbor = getChunk(cx, cz - 1);
            if (neighbor) neighbor->setDirty();
        }
        if (lz == kChunkDepth - 1) {
            Chunk *neighbor = getChunk(cx, cz + 1);
            if (neighbor) neighbor->setDirty();
        }
    }
}

Chunk *ChunkManager::getChunk(int chunkX, int chunkZ) const {
    auto it = chunks_.find(ChunkCoord{chunkX, chunkZ});
    return it != chunks_.end() ? it->second.get() : nullptr;
}

Chunk *ChunkManager::getOrCreateChunk(int chunkX, int chunkZ) {
    auto it = chunks_.find(ChunkCoord{chunkX, chunkZ});
    if (it != chunks_.end()) return it->second.get();
    auto chunk = std::make_unique<Chunk>(chunkX, chunkZ);
    generateChunk(*chunk);
    Chunk *ptr = chunk.get();
    chunks_[ChunkCoord{chunkX, chunkZ}] = std::move(chunk);
    return ptr;
}

void ChunkManager::generateChunk(Chunk &chunk) {
    int cx = chunk.getChunkX();
    int cz = chunk.getChunkZ();

    for (int z = 0; z < kChunkDepth; z++) {
        for (int x = 0; x < kChunkWidth; x++) {
            int wx = cx * kChunkWidth + x;
            int wz = cz * kChunkDepth + z;

            float n = noise_->fbm2D(wx * 0.03f, wz * 0.03f, 4, 2.0f, 0.5f);
            int height = (int)((n + 1.0f) * 0.5f * 20.0f) + 8;

            height = std::clamp(height, 1, kChunkHeight - 1);

            for (int y = 0; y <= height && y < kChunkHeight; y++) {
                BlockType type;
                if (y == height) {
                    type = BlockType::GRASS;
                } else if (y > height - 4) {
                    type = BlockType::DIRT;
                } else {
                    type = BlockType::STONE;
                }
                chunk.setBlock(x, y, z, type);
            }
        }
    }
}

void ChunkManager::ensureChunksAround(const Vector3 &playerPos) {
    int pcx = (int)std::floor(playerPos.x / kChunkWidth);
    int pcz = (int)std::floor(playerPos.z / kChunkDepth);

    for (int dz = -renderDistance_; dz <= renderDistance_; dz++) {
        for (int dx = -renderDistance_; dx <= renderDistance_; dx++) {
            int cx = pcx + dx;
            int cz = pcz + dz;
            if (std::abs(dx) + std::abs(dz) > renderDistance_ * renderDistance_ / 2) continue;
            getOrCreateChunk(cx, cz);
        }
    }
}

void ChunkManager::update() {
    for (auto &[coord, chunk] : chunks_) {
        if (chunk->isDirty()) {
            chunk->buildMesh();
        }
    }
}

void ChunkManager::render(const Shader &shader, const Matrix4x4 &vpMatrix, const Camera &camera) {
    Vector3 camPos = camera.getPosition();
    ensureChunksAround(camPos);

    for (auto &[coord, chunk] : chunks_) {
        if (chunk->isEmpty()) continue;

        // Basic distance culling
        float dx = (coord.x * kChunkWidth + kChunkWidth / 2) - camPos.x;
        float dz = (coord.z * kChunkDepth + kChunkDepth / 2) - camPos.z;
        float distSq = dx * dx + dz * dz;
        float maxDist = renderDistance_ * kChunkWidth;
        if (distSq > maxDist * maxDist) continue;

        chunk->render(shader, vpMatrix, camPos.data());
    }
}

int ChunkManager::getHeightAt(int worldX, int worldZ) const {
    float n = noise_->fbm2D(worldX * 0.03f, worldZ * 0.03f, 4, 2.0f, 0.5f);
    return (int)((n + 1.0f) * 0.5f * 20.0f) + 8;
}

RaycastHit ChunkManager::raycast(const Vector3 &origin, const Vector3 &direction, float maxDist) const {
    RaycastHit hit;
    hit.hit = false;

    Vector3 dir = direction.normalized();
    float t = 0.0f;
    constexpr float step = 0.1f;

    while (t < maxDist) {
        Vector3 pos = origin + dir * t;
        int bx = (int)std::floor(pos.x);
        int by = (int)std::floor(pos.y);
        int bz = (int)std::floor(pos.z);

        BlockType block = getBlock(bx, by, bz);
        if (block != BlockType::AIR) {
            hit.hit = true;
            hit.x = bx;
            hit.y = by;
            hit.z = bz;
            hit.type = block;

            // Determine which face was entered
            Vector3 entryPoint = pos;
            float cx = entryPoint.x - bx - 0.5f;
            float cy = entryPoint.y - by - 0.5f;
            float cz = entryPoint.z - bz - 0.5f;

            float ax = std::abs(cx);
            float ay = std::abs(cy);
            float az = std::abs(cz);

            if (ax >= ay && ax >= az) {
                hit.faceX = cx > 0 ? -1 : 1;
            } else if (ay >= az) {
                hit.faceY = cy > 0 ? -1 : 1;
            } else {
                hit.faceZ = cz > 0 ? -1 : 1;
            }
            return hit;
        }

        t += step;
    }

    return hit;
}
