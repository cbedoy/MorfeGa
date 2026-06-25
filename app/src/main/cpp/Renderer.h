#ifndef MORFEGA_RENDERER_H
#define MORFEGA_RENDERER_H

#include <EGL/egl.h>
#include <memory>
#include <vector>
#include "Model.h"
#include "Shader.h"
#include "Camera.h"
#include "ChunkManager.h"

struct android_app;
class Player;

class Renderer {
public:
    Renderer(android_app *pApp);
    virtual ~Renderer();

    void handleInput();
    void render();
    Camera &getCamera() { return *camera_; }
    Player &getPlayer() { return *player_; }

private:
    void initRenderer();
    void updateRenderArea();
    void updatePlayer(float deltaTime);

    void renderChunks();
    void renderSelector();
    void renderCrosshair();

    void handleBreakBlock();
    void handlePlaceBlock();

    android_app *app_;
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Player> player_;
    std::unique_ptr<ChunkManager> chunkManager_;

    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    std::unique_ptr<Shader> shader_;
    std::unique_ptr<Shader> selectorShader_;

    RaycastHit hit_;
    float lightDir_[3];
    GLuint crosshairProgram_ = 0;

    bool isPointerDown_;
    float pointerStartX_, pointerStartY_;
    bool hasMoved_;
};

#endif
