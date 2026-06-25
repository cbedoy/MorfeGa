#include "Utility.h"
#include "AndroidOut.h"

#define CHECK_ERROR(e) case e: aout << "GL Error: "#e << std::endl; break;

bool Utility::checkAndLogGlError(bool alwaysLog) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        if (alwaysLog) aout << "No GL error" << std::endl;
        return true;
    } else {
        switch (error) {
            CHECK_ERROR(GL_INVALID_ENUM);
            CHECK_ERROR(GL_INVALID_VALUE);
            CHECK_ERROR(GL_INVALID_OPERATION);
            CHECK_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
            CHECK_ERROR(GL_OUT_OF_MEMORY);
            default: aout << "Unknown GL error: " << error << std::endl;
        }
        return false;
    }
}

float *Utility::buildOrthographicMatrix(float *outMatrix, float halfHeight, float aspect, float near, float far) {
    float halfWidth = halfHeight * aspect;
    outMatrix[0] = 1.f / halfWidth;  outMatrix[1] = 0.f;  outMatrix[2] = 0.f;  outMatrix[3] = 0.f;
    outMatrix[4] = 0.f;  outMatrix[5] = 1.f / halfHeight;  outMatrix[6] = 0.f;  outMatrix[7] = 0.f;
    outMatrix[8] = 0.f;  outMatrix[9] = 0.f;  outMatrix[10] = -2.f / (far - near);  outMatrix[11] = -(far + near) / (far - near);
    outMatrix[12] = 0.f;  outMatrix[13] = 0.f;  outMatrix[14] = 0.f;  outMatrix[15] = 1.f;
    return outMatrix;
}

float *Utility::buildIdentityMatrix(float *outMatrix) {
    outMatrix[0] = 1.f; outMatrix[1] = 0.f; outMatrix[2] = 0.f; outMatrix[3] = 0.f;
    outMatrix[4] = 0.f; outMatrix[5] = 1.f; outMatrix[6] = 0.f; outMatrix[7] = 0.f;
    outMatrix[8] = 0.f; outMatrix[9] = 0.f; outMatrix[10] = 1.f; outMatrix[11] = 0.f;
    outMatrix[12] = 0.f; outMatrix[13] = 0.f; outMatrix[14] = 0.f; outMatrix[15] = 1.f;
    return outMatrix;
}
