#ifndef MORFEGA_PERLINNOISE_H
#define MORFEGA_PERLINNOISE_H

#include <cstdint>
#include <vector>

class PerlinNoise {
public:
    PerlinNoise(uint32_t seed = 42);

    float noise2D(float x, float z) const;
    float noise3D(float x, float y, float z) const;
    float fbm2D(float x, float z, int octaves = 6, float lacunarity = 2.0f, float gain = 0.5f) const;

private:
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float z) const;
    float grad(int hash, float x, float y, float z) const;

    std::vector<int> perm_;
};

#endif
