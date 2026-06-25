#ifndef MORFEGA_MODEL_H
#define MORFEGA_MODEL_H

#include <vector>
#include "TextureAsset.h"
#include "Utility.h"

struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
    float r, g, b, a;

    Vertex() = default;
    Vertex(float px, float py, float pz, float nx, float ny, float nz, float u, float v)
        : px(px), py(py), pz(pz), nx(nx), ny(ny), nz(nz), u(u), v(v), r(1), g(1), b(1), a(1) {}
    Vertex(float px, float py, float pz, float nx, float ny, float nz, float u, float v, float r, float g, float b, float a = 1)
        : px(px), py(py), pz(pz), nx(nx), ny(ny), nz(nz), u(u), v(v), r(r), g(g), b(b), a(a) {}
};

using Index = uint16_t;

class Model {
public:
    Model(
            std::vector<Vertex> vertices,
            std::vector<Index> indices,
            std::shared_ptr<TextureAsset> spTexture)
            : vertices_(std::move(vertices))
            , indices_(std::move(indices))
            , spTexture_(std::move(spTexture)) {}

    const Vertex *getVertexData() const { return vertices_.data(); }
    size_t getVertexCount() const { return vertices_.size(); }
    size_t getIndexCount() const { return indices_.size(); }
    const Index *getIndexData() const { return indices_.data(); }
    const TextureAsset &getTexture() const { return *spTexture_; }

    // For chunk meshes without texture
    void setTexture(std::shared_ptr<TextureAsset> tex) { spTexture_ = std::move(tex); }

private:
    std::vector<Vertex> vertices_;
    std::vector<Index> indices_;
    std::shared_ptr<TextureAsset> spTexture_;
};

#endif
