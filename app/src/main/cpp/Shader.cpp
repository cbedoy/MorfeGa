#include "Shader.h"

#include <unordered_map>
#include "AndroidOut.h"
#include "Model.h"

static constexpr int kInvalidLocation = -1;

Shader *Shader::loadShader(
        const std::string &vertexSource,
        const std::string &fragmentSource,
        const std::vector<std::pair<std::string, GLuint>> &attributeLocations,
        const std::vector<std::string> &uniformNames) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) return nullptr;

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return nullptr;
    }

    GLuint program = glCreateProgram();
    if (!program) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return nullptr;
    }

    for (auto &[name, loc] : attributeLocations) {
        glBindAttribLocation(program, loc, name.c_str());
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength) {
            auto *log = new GLchar[logLength];
            glGetProgramInfoLog(program, logLength, nullptr, log);
            aout << "Failed to link program:\n" << log << std::endl;
            delete[] log;
        }
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return nullptr;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return new Shader(program);
}

GLuint Shader::loadShader(GLenum shaderType, const std::string &shaderSource) {
    GLuint shader = glCreateShader(shaderType);
    if (!shader) return 0;

    auto *rawStr = (GLchar *)shaderSource.c_str();
    GLint length = shaderSource.length();
    glShaderSource(shader, 1, &rawStr, &length);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength) {
            auto *infoLog = new GLchar[infoLength];
            glGetShaderInfoLog(shader, infoLength, nullptr, infoLog);
            aout << (shaderType == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
                 << " shader compile error:\n" << infoLog << std::endl;
            delete[] infoLog;
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void Shader::activate() const {
    glUseProgram(program_);
}

void Shader::deactivate() const {
    glUseProgram(0);
}

void Shader::drawModel(const Model &model) const {
    GLint position = glGetAttribLocation(program_, "inPosition");
    GLint normal = glGetAttribLocation(program_, "inNormal");
    GLint uv = glGetAttribLocation(program_, "inUV");

    if (position >= 0) {
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), model.getVertexData());
        glEnableVertexAttribArray(position);
    }
    if (normal >= 0) {
        glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              ((uint8_t *)model.getVertexData()) + 12);
        glEnableVertexAttribArray(normal);
    }
    if (uv >= 0) {
        glVertexAttribPointer(uv, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              ((uint8_t *)model.getVertexData()) + 24);
        glEnableVertexAttribArray(uv);
    }

    glActiveTexture(GL_TEXTURE0);
    GLint texUniform = glGetUniformLocation(program_, "uTexture");
    if (texUniform >= 0) glUniform1i(texUniform, 0);
    glBindTexture(GL_TEXTURE_2D, model.getTexture().getTextureID());

    glDrawElements(GL_TRIANGLES, model.getIndexCount(), GL_UNSIGNED_SHORT, model.getIndexData());

    if (uv >= 0) glDisableVertexAttribArray(uv);
    if (normal >= 0) glDisableVertexAttribArray(normal);
    if (position >= 0) glDisableVertexAttribArray(position);
}

void Shader::setViewProjectionMatrix(const float *vpMatrix) const {
    GLint loc = glGetUniformLocation(program_, "uViewProj");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, false, vpMatrix);
}

void Shader::setModelMatrix(const float *mMatrix) const {
    GLint loc = glGetUniformLocation(program_, "uModel");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, false, mMatrix);
}

void Shader::setLightDir(const float *lightDir) const {
    GLint loc = glGetUniformLocation(program_, "uLightDir");
    if (loc >= 0) glUniform3fv(loc, 1, lightDir);
}

void Shader::setCameraPos(const float *cameraPos) const {
    GLint loc = glGetUniformLocation(program_, "uCameraPos");
    if (loc >= 0) glUniform3fv(loc, 1, cameraPos);
}

GLint Shader::getUniformLocation(const std::string &name) const {
    return glGetUniformLocation(program_, name.c_str());
}
