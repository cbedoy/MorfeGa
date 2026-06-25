#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <android/imagedecoder.h>

#include "AndroidOut.h"
#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"
#include "ChunkManager.h"
#include "Player.h"

#define PRINT_GL_STRING(s) {aout << #s": "<< glGetString(s) << std::endl;}
#define PRINT_GL_STRING_AS_LIST(s) { \
std::istringstream extensionStream((const char *) glGetString(s));\
std::vector<std::string> extensionList(\
        std::istream_iterator<std::string>{extensionStream},\
        std::istream_iterator<std::string>());\
aout << #s":\n";\
for (auto& ext: extensionList) { aout << ext << "\n"; }\
aout << std::endl;\
}

#define SKY_COLOR 135 / 255.f, 206 / 255.f, 235 / 255.f, 1

static const char *vertex3D = R"glsl(#version 300 es
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

uniform mat4 uViewProj;
uniform mat4 uModel;

out vec2 fragUV;
out vec3 fragNormal;
out vec3 fragWorldPos;
out vec4 fragColor;

void main() {
    vec4 worldPos = uModel * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = mat3(uModel) * inNormal;
    fragUV = inUV;
    fragColor = inColor;
    gl_Position = uViewProj * worldPos;
}
)glsl";

static const char *fragment3D = R"glsl(#version 300 es
precision highp float;

in vec2 fragUV;
in vec3 fragNormal;
in vec3 fragWorldPos;
in vec4 fragColor;

uniform vec3 uLightDir;

out vec4 outColor;

void main() {
    if (fragColor.a < 0.1) discard;
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, uLightDir), 0.0);
    float lighting = 0.4 + diff * 0.6;
    outColor = vec4(fragColor.rgb * lighting, fragColor.a);
}
)glsl";

static const char *vertexSelector = R"glsl(#version 300 es
layout(location = 0) in vec3 inPosition;

uniform mat4 uViewProj;
uniform mat4 uModel;

void main() {
    gl_Position = uViewProj * uModel * vec4(inPosition, 1.0);
}
)glsl";

static const char *fragmentSelector = R"glsl(#version 300 es
precision highp float;

out vec4 outColor;

void main() {
    outColor = vec4(1.0, 1.0, 1.0, 0.6);
}
)glsl";

Renderer::Renderer(android_app *pApp)
    : app_(pApp)
    , display_(EGL_NO_DISPLAY)
    , surface_(EGL_NO_SURFACE)
    , context_(EGL_NO_CONTEXT)
    , width_(0)
    , height_(0)
    , joystickKnobX_(0)
    , joystickKnobY_(0) {

    lightDir_[0] = 0.5f; lightDir_[1] = -0.8f; lightDir_[2] = 0.3f;
    float len = std::sqrt(lightDir_[0]*lightDir_[0] + lightDir_[1]*lightDir_[1] + lightDir_[2]*lightDir_[2]);
    lightDir_[0] /= len; lightDir_[1] /= len; lightDir_[2] /= len;

    initRenderer();
}

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::initRenderer() {
    constexpr EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;
    width_ = -1;
    height_ = -1;

    PRINT_GL_STRING(GL_VENDOR);
    PRINT_GL_STRING(GL_RENDERER);
    PRINT_GL_STRING(GL_VERSION);

    std::vector<std::pair<std::string, GLuint>> attribLocations = {
        {"inPosition", 0},
        {"inNormal", 1},
        {"inUV", 2},
        {"inColor", 3}
    };
    std::vector<std::string> uniformNames = {"uViewProj", "uModel", "uLightDir"};

    shader_.reset(Shader::loadShader(vertex3D, fragment3D, attribLocations, uniformNames));
    assert(shader_);
    shader_->activate();

    selectorShader_.reset(Shader::loadShader(vertexSelector, fragmentSelector,
        {{"inPosition", 0}}, {"uViewProj", "uModel"}));

    glClearColor(SKY_COLOR);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    camera_ = std::make_unique<Camera>();
    player_ = std::make_unique<Player>(*camera_);
    chunkManager_ = std::make_unique<ChunkManager>();

    camera_->setPosition(player_->getEyePosition());
}

void Renderer::updateRenderArea() {
    EGLint width, height;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);
    }
}

void Renderer::updatePlayer(float deltaTime) {
    player_->update(deltaTime, *chunkManager_);
    if (camera_->isFirstPerson()) {
        camera_->setPosition(player_->getEyePosition());
    } else {
        camera_->setPosition(player_->getPosition());
    }
}

void Renderer::render() {
    updateRenderArea();

    float aspect = float(width_) / float(height_);
    Matrix4x4 vp = camera_->getViewProjectionMatrix(aspect);
    camera_->update(0.016f);
    updatePlayer(0.016f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_->activate();
    shader_->setViewProjectionMatrix(vp.data());

    Vector3 lightDirNormalized(-lightDir_[0], -lightDir_[1], -lightDir_[2]);
    float ldLen = lightDirNormalized.length();
    if (ldLen > 0) lightDirNormalized = lightDirNormalized / ldLen;
    shader_->setLightDir(lightDirNormalized.data());

    chunkManager_->update();

    Matrix4x4 identity = Matrix4x4::identity();
    float identityData[16];
    std::memcpy(identityData, identity.data(), sizeof(identityData));
    shader_->setModelMatrix(identityData);

    chunkManager_->render(*shader_, vp, *camera_);

    renderSelector();
    renderCrosshair();
    renderJoystick();

    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::renderSelector() {
    if (!hit_.hit) return;

    float verts[] = {
        0,0,0, 1,0,0, 1,0,1, 0,0,1,
        0,1,0, 1,1,0, 1,1,1, 0,1,1,
        0,0,0, 1,0,0, 1,1,0, 0,1,0,
        0,0,1, 1,0,1, 1,1,1, 0,1,1,
        0,0,0, 0,0,1, 0,1,1, 0,1,0,
        1,0,0, 1,0,1, 1,1,1, 1,1,0,
    };
    unsigned short indices[] = {
        0,1,1,2,2,3,3,0,
        4,5,5,6,6,7,7,4,
        8,9,9,10,10,11,11,8,
        12,13,13,14,14,15,15,12,
        16,17,17,18,18,19,19,16,
        20,21,21,22,22,23,23,20
    };

    Matrix4x4 modelMat = Matrix4x4::translation(Vector3(hit_.x, hit_.y, hit_.z));
    float aspect = float(width_) / float(height_);
    Matrix4x4 vp = camera_->getViewProjectionMatrix(aspect);

    selectorShader_->activate();
    selectorShader_->setViewProjectionMatrix(vp.data());

    float modelData[16];
    std::memcpy(modelData, modelMat.data(), sizeof(modelData));
    selectorShader_->setModelMatrix(modelData);

    GLint posLoc = 0;
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, verts);
    glEnableVertexAttribArray(posLoc);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawElements(GL_LINES, 48, GL_UNSIGNED_SHORT, indices);

    glDisableVertexAttribArray(posLoc);
    glDisable(GL_BLEND);

    shader_->activate();
}

void Renderer::renderCrosshair() {
    float s = 10.0f / height_;
    float verts[] = { -s, 0, s, 0, 0, -s, 0, s };
    unsigned short idx[] = { 0, 1, 2, 3 };

    glDisable(GL_DEPTH_TEST);

    GLuint program;
    GLint vpLoc;
    if (crosshairProgram_ == 0) {
        const char *vSrc = R"(#version 300 es
layout(location = 0) in vec2 p;
void main() { gl_Position = vec4(p, 0.0, 1.0); })";
        const char *fSrc = R"(#version 300 es
precision mediump float;
out vec4 c;
void main() { c = vec4(1.0, 1.0, 1.0, 0.8); })";
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vSrc, nullptr);
        glCompileShader(vs);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fSrc, nullptr);
        glCompileShader(fs);
        program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        crosshairProgram_ = program;
    } else {
        program = crosshairProgram_;
    }

    glUseProgram(program);

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, nullptr);
    glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, (void*)(2 * sizeof(unsigned short)));

    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    glEnable(GL_DEPTH_TEST);
    shader_->activate();
}

void Renderer::handleInput() {
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) return;

    bool tapOnRight = false;

    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        float halfWidth = width_ / 2.0f;
        bool isLeft = x < halfWidth;

        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN: {
                int slot = isLeft ? 0 : 1;
                pointers_[slot].active = true;
                pointers_[slot].startX = x;
                pointers_[slot].startY = y;
                pointers_[slot].currentX = x;
                pointers_[slot].currentY = y;
                pointers_[slot].hasMoved = false;
                if (isLeft) {
                    joystickBaseX_ = x;
                    joystickBaseY_ = y;
                    joystickKnobX_ = 0;
                    joystickKnobY_ = 0;
                }
                break;
            }

            case AMOTION_EVENT_ACTION_MOVE: {
                for (int p = 0; p < motionEvent.pointerCount; p++) {
                    auto &ptr = motionEvent.pointers[p];
                    float px = GameActivityPointerAxes_getX(&ptr);
                    float py = GameActivityPointerAxes_getY(&ptr);

                    bool pIsLeft = px < halfWidth;
                    int slot = pIsLeft ? 0 : 1;
                    if (!pointers_[slot].active) continue;

                    float dx = px - pointers_[slot].startX;
                    float dy = py - pointers_[slot].startY;

                    if (slot == 0) {
                        // Joystick
                        float knobDx = px - joystickBaseX_;
                        float knobDy = py - joystickBaseY_;
                        float dist = std::sqrt(knobDx * knobDx + knobDy * knobDy);
                        if (dist > joystickRadius_) {
                            knobDx = knobDx / dist * joystickRadius_;
                            knobDy = knobDy / dist * joystickRadius_;
                        }
                        joystickKnobX_ = knobDx / joystickRadius_;
                        joystickKnobY_ = knobDy / joystickRadius_;
                        pointers_[slot].currentX = joystickBaseX_ + knobDx;
                        pointers_[slot].currentY = joystickBaseY_ + knobDy;
                        if (dist > 10.0f) pointers_[slot].hasMoved = true;
                    } else {
                        // Camera drag
                        if (std::abs(dx) > 2 || std::abs(dy) > 2) {
                            pointers_[slot].hasMoved = true;
                            camera_->rotate(dx, dy);
                            pointers_[slot].startX = px;
                            pointers_[slot].startY = py;
                            pointers_[slot].currentX = px;
                            pointers_[slot].currentY = py;
                        }
                    }
                }
                break;
            }

            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP: {
                int slot = isLeft ? 0 : 1;
                if (slot == 1 && pointers_[slot].active && !pointers_[slot].hasMoved) {
                    tapOnRight = true;
                }
                if (slot == 0) {
                    joystickKnobX_ = 0;
                    joystickKnobY_ = 0;
                }
                pointers_[slot].active = false;
                break;
            }

            default:
                break;
        }
    }
    android_app_clear_motion_events(inputBuffer);

    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
        auto &keyEvent = inputBuffer->keyEvents[i];
        if (keyEvent.action == AKEY_EVENT_ACTION_DOWN) {
            if (keyEvent.keyCode == AKEYCODE_SPACE) {
                player_->jump();
            }
        }
    }
    android_app_clear_key_events(inputBuffer);

    if (tapOnRight) {
        handleBreakBlock();
    }

    // Joystick → player movement
    Vector3 flatForward = camera_->getForward();
    flatForward.y = 0;
    if (flatForward.length() > 0) flatForward = flatForward.normalized();
    Vector3 flatRight = camera_->getRight();
    flatRight.y = 0;
    if (flatRight.length() > 0) flatRight = flatRight.normalized();
    Vector3 moveDir = flatForward * (-joystickKnobY_) + flatRight * joystickKnobX_;
    if (moveDir.length() > 0.1f) {
        moveDir = moveDir.normalized();
        player_->move(moveDir);
    } else {
        player_->move(Vector3(0, 0, 0));
    }
}

void Renderer::renderJoystick() {
    if (!pointers_[0].active) return;

    float px = joystickBaseX_;
    float py = joystickBaseY_;

    float nx = 2.0f * px / width_ - 1.0f;
    float ny = 1.0f - 2.0f * py / height_;
    float r = 2.0f * joystickRadius_ / height_;

    float knobNx = nx + 2.0f * joystickKnobX_ * joystickRadius_ / height_;
    float knobNy = ny - 2.0f * joystickKnobY_ * joystickRadius_ / height_;

    if (joystickProgram_ == 0) {
        const char *vSrc = R"(#version 300 es
layout(location = 0) in vec2 p;
void main() { gl_Position = vec4(p, 0.0, 1.0); })";
        const char *fSrc = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 c;
void main() { c = uColor; })";
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vSrc, nullptr);
        glCompileShader(vs);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fSrc, nullptr);
        glCompileShader(fs);
        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        joystickProgram_ = program;
    }

    glUseProgram(joystickProgram_);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Base ring (hexagon approximation)
    constexpr int kSegments = 20;
    float baseVerts[kSegments * 2 + 2];
    for (int i = 0; i <= kSegments; i++) {
        float angle = float(i) / float(kSegments) * 6.283185f;
        baseVerts[i * 2] = nx + r * std::cos(angle);
        baseVerts[i * 2 + 1] = ny + r * std::sin(angle);
    }

    GLint colorLoc = glGetUniformLocation(joystickProgram_, "uColor");
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Draw base ring
    glBufferData(GL_ARRAY_BUFFER, sizeof(baseVerts), baseVerts, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 0.2f);
    glDrawArrays(GL_LINE_STRIP, 0, kSegments + 1);

    // Draw knob
    float knobR = r * 0.35f;
    float knobVerts[kSegments * 2 + 2];
    for (int i = 0; i <= kSegments; i++) {
        float angle = float(i) / float(kSegments) * 6.283185f;
        knobVerts[i * 2] = knobNx + knobR * std::cos(angle);
        knobVerts[i * 2 + 1] = knobNy + knobR * std::sin(angle);
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(knobVerts), knobVerts, GL_STREAM_DRAW);
    glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 0.6f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, kSegments + 1);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    glEnable(GL_DEPTH_TEST);
    shader_->activate();
}

void Renderer::handleBreakBlock() {
    Vector3 origin = camera_->getPosition();
    Vector3 dir = camera_->getForward();

    hit_ = chunkManager_->raycast(origin, dir, 4.0f);
    if (hit_.hit) {
        chunkManager_->setBlock(hit_.x, hit_.y, hit_.z, BlockType::AIR);
    }
}

void Renderer::handlePlaceBlock() {
    Vector3 origin = camera_->getPosition();
    Vector3 dir = camera_->getForward();

    RaycastHit placeHit = chunkManager_->raycast(origin, dir, 4.0f);
    if (placeHit.hit) {
        int nx = placeHit.x + placeHit.faceX;
        int ny = placeHit.y + placeHit.faceY;
        int nz = placeHit.z + placeHit.faceZ;
        if (chunkManager_->getBlock(nx, ny, nz) == BlockType::AIR) {
            chunkManager_->setBlock(nx, ny, nz, BlockType::GRASS);
        }
    }
}
