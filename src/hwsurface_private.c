#include "hwsurface.h"
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#define MAX_SAMPLERS 8

static struct {
    GLuint FBO, VAO, VBO, RBO, Texture;
    int sizeOfSamplers;
    int Width, Height;
} RenderBuffer;

static bool InitRenderBuffer(int w, int h) {
    RenderBuffer.Width = w;
    RenderBuffer.Height = h;
    glGenFramebuffers(1, &RenderBuffer.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer.FBO);
    glGenTextures(1, &RenderBuffer.Texture);
    glBindTexture(GL_TEXTURE_2D, RenderBuffer.Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, RenderBuffer.Texture, 0);
    glGenRenderbuffers(1, &RenderBuffer.RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RenderBuffer.RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RenderBuffer.RBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    static float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &RenderBuffer.VAO);
    glGenBuffers(1, &RenderBuffer.VBO);
    glBindVertexArray(RenderBuffer.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, RenderBuffer.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    RenderBuffer.sizeOfSamplers = 0;
    return true;
}

static void DestroyRenderBuffer(void) {
    glDeleteBuffers(1, &RenderBuffer.VBO);
    glDeleteVertexArrays(1, &RenderBuffer.VAO);
    glDeleteRenderbuffers(1, &RenderBuffer.RBO);
    glDeleteTextures(1, &RenderBuffer.Texture);
    glDeleteFramebuffers(1, &RenderBuffer.FBO);
    memset(&RenderBuffer, 0, sizeof(RenderBuffer));
}

unsigned int NewTexture(Surface *surface) {
    unsigned int result = 0;
    if (!surface || !surface->buf)
        return result;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (void*)surface->buf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return result;
}

void DestroyTexture(unsigned int texture) {
    glDeleteTextures(1, &texture);
}

static bool CheckShaderLog(GLuint r, void(*check_fn)(GLuint, GLenum, GLint*), GLenum check_type) {
  GLint success = false;
  check_fn(r, check_type, &success);
  if (success)
    return true;
  
  GLint length = 0;
  check_fn(r, GL_INFO_LOG_LENGTH, &length);
  if (length) {
    char log[1024];
    glGetShaderInfoLog(r, 1024, NULL, log);
    fprintf(stderr, "[ERROR] Shader compilation failed:\n%s\n", log);
  }
  return false;
}

static GLuint CompileShader(GLuint t, const char *c) {
    GLuint r = glCreateShader(t);
    glShaderSource(r, 1, &c, NULL);
    glCompileShader(r);
    return !CheckShaderLog(r, glGetShaderiv, GL_COMPILE_STATUS) ? 0 : r;
}

int NewShader(const char *vertexSrc, const char *fragmentSrc) {
    int result = glCreateProgram();
    int vertex = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    if (!vertex)
        return 0;
    glAttachShader(result, vertex);
    int fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (!fragment)
        return 0;
    glAttachShader(result, fragment);
    glLinkProgram(result);
    if (!CheckShaderLog(result, glGetProgramiv, GL_LINK_STATUS))
        return 0;
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return result;    
}

static const char *LoadFileToMemory(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;
    
    fseek(f, 0, SEEK_END);
    size_t length = ftell(f);
    fseek(f, 0, SEEK_SET);
    const char *result = malloc(length);
    fread((void*)result, 1, length, f);
    fclose(f);
    return result;
}

int NewShaderFromFiles(const char *vertexPath, const char *fragmentPath) {
    int result = 0;
    const char *vertexSrc = LoadFileToMemory(vertexPath);
    if (!vertexSrc)
        return result;
    const char *fragmentSrc = LoadFileToMemory(fragmentPath);
    if (!fragmentSrc) {
        free((void*)vertexSrc);
        return result;
    }
    result = NewShader(vertexSrc, fragmentSrc);
    if (vertexSrc)
        free((void*)vertexSrc);
    if (fragmentSrc)
        free((void*)fragmentSrc);
    return result;
}

int GetShaderLocation(int shader, const char *uniformName) {
    return glGetUniformLocation(shader, uniformName);
}

int GetShaderLocationAttrib(int shader, const char *attribName) {
    return glGetAttribLocation(shader, attribName);
}

void SetShaderValueEx(int locIndex, const void *value, UniformType uniformType, int count) {
    switch (uniformType) {
#define X(A, B, C) \
        case UNIFORM_##A: \
            glUniform##B(locIndex, count, (C*)value); break;
        UNIFORMS
#undef X
        default:
            printf("ERROR: Failed to set uniform value, unknown type");
    }
}

void SetShaderValue(int locIndex, const void *value, int uniformType) {
    SetShaderValueEx(locIndex, value, uniformType, 1);
}

void SetShaderValueMatrix2x2(int locIndex, float mat[4]) {
    glUniformMatrix2fv(locIndex, 1, false, mat);
}

void SetShaderValueMatrix3x3(int locIndex, float mat[9]) {
    glUniformMatrix3fv(locIndex, 1, false, mat);
}

void SetShaderValueMatrix4x4(int locIndex, float mat[16]) {
    glUniformMatrix4fv(locIndex, 1, false, mat);
}

void SetShaderValueTexture(int locIndex, int texture) {
    if (RenderBuffer.sizeOfSamplers == MAX_SAMPLERS) {
        printf("ERROR: Max samplers reached (%d)\n", MAX_SAMPLERS);
        return;
    }
    glUniform1i(locIndex, RenderBuffer.sizeOfSamplers);
    glActiveTexture(RenderBuffer.sizeOfSamplers++);
    glBindTexture(GL_TEXTURE_2D, texture);
}

void DestroyShader(int shader) {
    glDeleteProgram(shader);
}

void BeginGLRenderer(int shader) {
    glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer.FBO);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader);
}

void EndGLRenderer(Surface *out) {
    if (!out || !out->buf) {
        glUseProgram(0);
        return;
    }
    
    glBindVertexArray(RenderBuffer.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glUseProgram(0);
    
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, RenderBuffer.Width, RenderBuffer.Height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, out->buf);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    RenderBuffer.sizeOfSamplers = 0;
}