#ifndef MORFEGA_BLOCKTYPES_H
#define MORFEGA_BLOCKTYPES_H

#include <cstdint>

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
    COUNT
};

inline bool isTransparent(BlockType type) {
    return type == BlockType::AIR || type == BlockType::WATER || type == BlockType::LEAVES;
}

inline bool isSolid(BlockType type) {
    return type != BlockType::AIR;
}

#endif
