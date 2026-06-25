#ifndef MORFEGA_SHADER_H
#define MORFEGA_SHADER_H

#include <string>
#include <GLES3/gl3.h>

class Model;

class Shader {
public:
    static Shader *loadShader(
            const std::string &vertexSource,
            const std::string &fragmentSource,
            const std::vector<std::pair<std::string, GLuint>> &attributeLocations,
            const std::vector<std::string> &uniformNames);

    inline ~Shader() {
        if (program_) {
            glDeleteProgram(program_);
            program_ = 0;
        }
    }

    void activate() const;
    void deactivate() const;
    void drawModel(const Model &model) const;
    void setViewProjectionMatrix(const float *vpMatrix) const;
    void setModelMatrix(const float *mMatrix) const;
    void setLightDir(const float *lightDir) const;
    void setCameraPos(const float *cameraPos) const;

    GLint getUniformLocation(const std::string &name) const;

private:
    static GLuint loadShader(GLenum shaderType, const std::string &shaderSource);

    constexpr Shader(GLuint program) : program_(program) {}

    GLuint program_;
};

#endif
