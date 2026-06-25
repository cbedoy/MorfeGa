#include "PerlinNoise.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>

PerlinNoise::PerlinNoise(uint32_t seed) : perm_(512) {
    std::vector<int> p(256);
    std::iota(p.begin(), p.end(), 0);
    std::mt19937 rng(seed);
    std::shuffle(p.begin(), p.end(), rng);
    for (int i = 0; i < 256; i++) {
        perm_[i] = perm_[i + 256] = p[i];
    }
}

float PerlinNoise::fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoise::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float z) const {
    int h = hash & 7;
    float u = h < 4 ? x : z;
    float v = h < 4 ? z : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float PerlinNoise::grad(int hash, float x, float y, float z) const {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float PerlinNoise::noise2D(float x, float z) const {
    int X = (int)std::floor(x) & 255;
    int Z = (int)std::floor(z) & 255;
    x -= std::floor(x);
    z -= std::floor(z);
    float u = fade(x);
    float v = fade(z);

    int aa = perm_[perm_[X] + Z];
    int ab = perm_[perm_[X] + Z + 1];
    int ba = perm_[perm_[X + 1] + Z];
    int bb = perm_[perm_[X + 1] + Z + 1];

    return lerp(
        lerp(grad(aa, x, z), grad(ba, x - 1, z), u),
        lerp(grad(ab, x, z - 1), grad(bb, x - 1, z - 1), u),
        v
    );
}

float PerlinNoise::noise3D(float x, float y, float z) const {
    int X = (int)std::floor(x) & 255;
    int Y = (int)std::floor(y) & 255;
    int Z = (int)std::floor(z) & 255;
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);

    int aaa = perm_[perm_[perm_[X] + Y] + Z];
    int aba = perm_[perm_[perm_[X] + Y + 1] + Z];
    int aab = perm_[perm_[perm_[X] + Y] + Z + 1];
    int abb = perm_[perm_[perm_[X] + Y + 1] + Z + 1];
    int baa = perm_[perm_[perm_[X + 1] + Y] + Z];
    int bba = perm_[perm_[perm_[X + 1] + Y + 1] + Z];
    int bab = perm_[perm_[perm_[X + 1] + Y] + Z + 1];
    int bbb = perm_[perm_[perm_[X + 1] + Y + 1] + Z + 1];

    float x1 = lerp(grad(aaa, x, y, z), grad(baa, x - 1, y, z), u);
    float x2 = lerp(grad(aba, x, y - 1, z), grad(bba, x - 1, y - 1, z), u);
    float x3 = lerp(grad(aab, x, y, z - 1), grad(bab, x - 1, y, z - 1), u);
    float x4 = lerp(grad(abb, x, y - 1, z - 1), grad(bbb, x - 1, y - 1, z - 1), u);

    return lerp(lerp(x1, x2, v), lerp(x3, x4, v), w);
}

float PerlinNoise::fbm2D(float x, float z, int octaves, float lacunarity, float gain) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise2D(x * frequency, z * frequency);
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }

    return value / maxValue;
}
