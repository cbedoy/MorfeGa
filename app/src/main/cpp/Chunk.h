#ifndef MORFEGA_CHUNK_H
#define MORFEGA_CHUNK_H

#include <cstdint>
#include <memory>
#include <vector>
#include "BlockTypes.h"
#include "Model.h"

class Shader;
class Matrix4x4;

constexpr int kChunkWidth = 16;
constexpr int kChunkHeight = 64;
constexpr int kChunkDepth = 16;
constexpr int kChunkSize = kChunkWidth * kChunkHeight * kChunkDepth;

class Chunk {
public:
    Chunk(int chunkX, int chunkZ);
    ~Chunk() = default;

    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);
    void setBlockWorld(int worldX, int worldY, int worldZ, BlockType type);
    BlockType getBlockWorld(int worldX, int worldY, int worldZ) const;

    int getChunkX() const { return chunkX_; }
    int getChunkZ() const { return chunkZ_; }

    bool isDirty() const { return dirty_; }
    void setDirty() { dirty_ = true; }
    void buildMesh();
    void render(const Shader &shader, const Matrix4x4 &vpMatrix, const float *camPos) const;

    bool isEmpty() const;

private:
    int chunkX_, chunkZ_;
    uint8_t blocks_[kChunkSize];
    bool dirty_;

    std::unique_ptr<Model> mesh_;
    std::unique_ptr<Model> transparentMesh_;
};

#endif
