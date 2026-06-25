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

uniform mat4 uViewProj;
uniform mat4 uModel;

out vec2 fragUV;
out vec3 fragNormal;
out vec3 fragWorldPos;

void main() {
    vec4 worldPos = uModel * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = mat3(uModel) * inNormal;
    fragUV = inUV;
    gl_Position = uViewProj * worldPos;
}
)glsl";

static const char *fragment3D = R"glsl(#version 300 es
precision highp float;

in vec2 fragUV;
in vec3 fragNormal;
in vec3 fragWorldPos;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uCameraPos;

out vec4 outColor;

void main() {
    vec4 texColor = texture(uTexture, fragUV);
    if (texColor.a < 0.1) discard;

    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, uLightDir), 0.15);
    float ambient = 0.5;
    float lighting = ambient + diff * 0.5;

    outColor = vec4(texColor.rgb * lighting, texColor.a);
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
    , isPointerDown_(false)
    , hasMoved_(false) {

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
        {"inUV", 2}
    };
    std::vector<std::string> uniformNames = {"uViewProj", "uModel", "uTexture", "uLightDir", "uCameraPos"};

    shader_.reset(Shader::loadShader(vertex3D, fragment3D, attribLocations, uniformNames));
    assert(shader_);
    shader_->activate();

    selectorShader_.reset(Shader::loadShader(vertexSelector, fragmentSelector,
        {{"inPosition", 0}}, {"uViewProj", "uModel"}));

    glClearColor(SKY_COLOR);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

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

    Vector3 camPos = camera_->getPosition();
    shader_->setCameraPos(camPos.data());

    chunkManager_->update();

    Matrix4x4 identity = Matrix4x4::identity();
    float identityData[16];
    std::memcpy(identityData, identity.data(), sizeof(identityData));
    shader_->setModelMatrix(identityData);

    chunkManager_->render(*shader_, vp, *camera_);

    renderSelector();
    renderCrosshair();

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

    bool hasTap = false;

    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                isPointerDown_ = true;
                pointerStartX_ = x;
                pointerStartY_ = y;
                hasMoved_ = false;
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                if (isPointerDown_) {
                    float dx = x - pointerStartX_;
                    float dy = y - pointerStartY_;
                    if (std::abs(dx) > 2 || std::abs(dy) > 2) {
                        hasMoved_ = true;
                        camera_->rotate(dx, dy);
                        pointerStartX_ = x;
                        pointerStartY_ = y;
                    }
                }
                break;

            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                if (isPointerDown_ && !hasMoved_) {
                    hasTap = true;
                }
                isPointerDown_ = false;
                break;

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

    if (hasTap) {
        handleBreakBlock();
    }

    Vector3 forward = camera_->getForward();
    Vector3 moveDir = forward * Vector3(1, 0, 1);
    if (isPointerDown_ && hasMoved_) {
        player_->move(moveDir.normalized() * Vector3(1, 0, 1));
    }
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
