#ifndef hwsurface_h
#define hwsurface_h
#include "surface.h"

#define GLSL(VERSION,CODE) "#version " #VERSION "\n" #CODE

unsigned int NewTexture(Surface *surface);
void DestroyTexture(unsigned int texture);

int NewShader(const char *vertexSrc, const char *fragmentSrc);
int NewShaderFromFiles(const char *vertexPath, const char *fragmentPath);
int GetShaderLocation(int shader, const char *uniformName);
int GetShaderLocationAttrib(int shader, const char *attribName);
#define UNIFORMS \
    X(FLOAT, 1fv, float) \
    X(VEC2, 2fv, float) \
    X(VEC3, 3fv, float) \
    X(VEC4, 4fv, float) \
    X(INT, 1iv, int) \
    X(IVEC2, 2iv, int) \
    X(IVEC3, 3iv, int) \
    X(IVEC4, 4iv, int) \
    X(SAMPLER2D, 1iv, int)

typedef enum {
#define X(A, B, C) UNIFORM_##A,
UNIFORMS
#undef X
} UniformType;
void SetShaderValueEx(int locIndex, const void *value, UniformType uniformType, int count);
void SetShaderValue(int locIndex, const void *value, int uniformType);
void SetShaderValueMatrix2x2(int locIndex, float mat[4]);
void SetShaderValueMatrix3x3(int locIndex, float mat[9]);
void SetShaderValueMatrix4x4(int locIndex, float mat[16]);
void SetShaderValueTexture(int locIndex, int texture);
void DestroyShader(int shader);

bool InitGLRenderer(int w, int h);
void BeginGLRenderer(int shader);
void EndGLRenderer(Surface *out);
void DestroyGLRenderer(void);

#endif // hwsurface_h