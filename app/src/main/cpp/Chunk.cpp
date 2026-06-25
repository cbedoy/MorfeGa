#include "Chunk.h"
#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"
#include <cstring>

static std::shared_ptr<TextureAsset> g_whiteTexture;

static void ensureWhiteTexture() {
    if (g_whiteTexture) return;
    g_whiteTexture = TextureAsset::createWhiteTexture();
}

inline int idx(int x, int y, int z) {
    return (y * kChunkDepth + z) * kChunkWidth + x;
}

inline bool inBounds(int x, int y, int z) {
    return x >= 0 && x < kChunkWidth && y >= 0 && y < kChunkHeight && z >= 0 && z < kChunkDepth;
}

static const float kBlockColors[][3] = {
    {0, 0, 0},        // AIR
    {0.35f, 0.85f, 0.15f}, // GRASS (top) — saturated green
    {0.6f, 0.4f, 0.2f}, // DIRT — warm brown
    {0.6f, 0.6f, 0.6f}, // STONE — medium gray
    {0.5f, 0.25f, 0.1f}, // WOOD — dark brown
    {0.85f, 0.8f, 0.55f}, // SAND — warm beige
    {0.2f, 0.5f, 0.9f}, // WATER — deep blue
    {0.15f, 0.55f, 0.15f}, // LEAVES — forest green
    {0.7f, 0.25f, 0.15f}, // BRICK — terracotta red
    {0.95f, 0.95f, 0.95f}, // SNOW — pure white
    {0.5f, 0.45f, 0.4f}, // COBBLESTONE — dark gray
    {0.8f, 0.6f, 0.35f}, // PLANKS — oak wood
    {0.55f, 0.5f, 0.45f}, // GRAVEL — speckled gray
};

Chunk::Chunk(int chunkX, int chunkZ)
    : chunkX_(chunkX), chunkZ_(chunkZ), dirty_(true) {
    std::memset(blocks_, 0, sizeof(blocks_));
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (!inBounds(x, y, z)) return BlockType::AIR;
    return static_cast<BlockType>(blocks_[idx(x, y, z)]);
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (!inBounds(x, y, z)) return;
    blocks_[idx(x, y, z)] = static_cast<uint8_t>(type);
    dirty_ = true;
}

void Chunk::setBlockWorld(int worldX, int worldY, int worldZ, BlockType type) {
    int lx = worldX - chunkX_ * kChunkWidth;
    int lz = worldZ - chunkZ_ * kChunkDepth;
    setBlock(lx, worldY, lz, type);
}

BlockType Chunk::getBlockWorld(int worldX, int worldY, int worldZ) const {
    int lx = worldX - chunkX_ * kChunkWidth;
    int lz = worldZ - chunkZ_ * kChunkDepth;
    return getBlock(lx, worldY, lz);
}

bool Chunk::isEmpty() const {
    for (int i = 0; i < kChunkSize; i++) {
        if (blocks_[i] != 0) return false;
    }
    return true;
}

void Chunk::buildMesh() {
    ensureWhiteTexture();
    dirty_ = false;

    struct CubeFace {
        float verts[4][3];
        float normal[3];
    };

    static const CubeFace faces[6] = {
        {{{0,0,1},{1,0,1},{1,1,1},{0,1,1}}, {0,0,1}},   // +Z
        {{{1,0,0},{1,0,1},{1,1,1},{1,1,0}}, {1,0,0}},   // +X
        {{{0,1,0},{1,1,0},{1,1,1},{0,1,1}}, {0,1,0}},   // +Y
        {{{1,0,0},{0,0,0},{0,1,0},{1,1,0}}, {0,0,-1}},  // -Z
        {{{0,0,1},{0,0,0},{0,1,0},{0,1,1}}, {-1,0,0}},  // -X
        {{{0,0,1},{1,0,1},{1,0,0},{0,0,0}}, {0,-1,0}},  // -Y
    };

    std::vector<Vertex> solidVerts;
    std::vector<Index> solidIndices;
    std::vector<Vertex> transVerts;
    std::vector<Index> transIndices;

    BlockType neighbors[6];
    int worldXBase = chunkX_ * kChunkWidth;
    int worldZBase = chunkZ_ * kChunkDepth;

    for (int y = 0; y < kChunkHeight; y++) {
        for (int z = 0; z < kChunkDepth; z++) {
            for (int x = 0; x < kChunkWidth; x++) {
                BlockType type = getBlock(x, y, z);
                if (type == BlockType::AIR) continue;

                bool transparent = isTransparent(type);
                int typeIdx = static_cast<int>(type);
                const float *col = kBlockColors[typeIdx];

                neighbors[0] = getBlock(x, y, z + 1);
                neighbors[1] = getBlock(x + 1, y, z);
                neighbors[2] = getBlock(x, y + 1, z);
                neighbors[3] = getBlock(x, y, z - 1);
                neighbors[4] = getBlock(x - 1, y, z);
                neighbors[5] = getBlock(x, y - 1, z);

                for (int face = 0; face < 6; face++) {
                    BlockType n = neighbors[face];
                    bool nTransparent = isTransparent(n);

                    bool showFace = (n == BlockType::AIR) ||
                                    (transparent && n != BlockType::AIR && !nTransparent) ||
                                    (!transparent && nTransparent);

                    if (!showFace) continue;

                    auto &verts = transparent ? transVerts : solidVerts;
                    auto &indices = transparent ? transIndices : solidIndices;

                    const CubeFace &f = faces[face];
                    Index base = verts.size();

                    for (int v = 0; v < 4; v++) {
                        float alpha = transparent ? (type == BlockType::WATER ? 0.5f : 0.7f) : 1.0f;
                        verts.emplace_back(
                            worldXBase + x + f.verts[v][0],
                            y + f.verts[v][1],
                            worldZBase + z + f.verts[v][2],
                            f.normal[0], f.normal[1], f.normal[2],
                            (v == 1 || v == 2) ? 1.0f : 0.0f,
                            (v == 2 || v == 3) ? 1.0f : 0.0f,
                            col[0], col[1], col[2], alpha
                        );
                    }
                    indices.push_back(base);
                    indices.push_back(base + 1);
                    indices.push_back(base + 2);
                    indices.push_back(base);
                    indices.push_back(base + 2);
                    indices.push_back(base + 3);
                }
            }
        }
    }

    if (!solidVerts.empty()) {
        mesh_ = std::make_unique<Model>(std::move(solidVerts), std::move(solidIndices), g_whiteTexture);
    } else {
        mesh_.reset();
    }
    if (!transVerts.empty()) {
        transparentMesh_ = std::make_unique<Model>(std::move(transVerts), std::move(transIndices), g_whiteTexture);
    } else {
        transparentMesh_.reset();
    }
}

void Chunk::render(const Shader &shader, const Matrix4x4 &vpMatrix, const float *camPos) const {
    if (mesh_) {
        shader.drawModel(*mesh_);
    }
    if (transparentMesh_) {
        shader.drawModel(*transparentMesh_);
    }
}
