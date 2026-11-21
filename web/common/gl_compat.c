#ifdef __EMSCRIPTEN__
#define WEB_GL_IMPLEMENTATION
#include "gl_compat.h"
#include "webtexture.h"
#include "platform.h"
extern void WebEndFrame(void);
#include <emscripten.h>
#include <stdlib.h>
#include <stdint.h>

SDL_Window* WebCreateWindow(const char* title, int width, int height, SDL_WindowFlags flags)
{
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    return SDL_CreateWindow(title, width, height, flags & ~SDL_WINDOW_HIGH_PIXEL_DENSITY);
}
bool WebSwapWindow(SDL_Window* window)
{
    bool result = SDL_GL_SwapWindow(window);
    WebEndFrame();
    WebPrepareFrame();
    return result;
}
SDL_MouseButtonFlags WebGetMouseState(float* x, float* y)
{
    SDL_MouseButtonFlags buttons = SDL_GetMouseState(x, y);
    float px, py;
    if (WebGetPointer(&px, &py)) { if (x) *x = px; if (y) *y = py; }
    if (WebGetClick()) buttons |= SDL_BUTTON_LMASK;
    return buttons;
}
bool WebRelativeMouseMode(SDL_Window* window, bool enabled) { (void)window; (void)enabled; return true; }
bool WebFullscreen(SDL_Window* window, bool enabled) { (void)window; (void)enabled; return true; }

SDL_FunctionPointer WebGLGetProcAddress(const char* name)
{
    // Emscripten's getProcAddress table omits this legacy entry point.
    if (SDL_strcmp(name, "glClientActiveTexture") == 0) return (SDL_FunctionPointer)glClientActiveTexture;
    if (SDL_strcmp(name, "glActiveTexture") == 0) return (SDL_FunctionPointer)glActiveTexture;
    return SDL_GL_GetProcAddress(name);
}

static GLint unpackRowLength;
static GLuint boundTexture;
static bool* opaqueTextures;
static size_t textureCapacity;

static void Require(bool valid, const char* reason)
{
    if (!valid) { SDL_LogError(SDL_LOG_CATEGORY_RENDER, "%s", reason); abort(); }
}

SDL_GLContext WebCreateGLContext(SDL_Window* window)
{
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context)
    {
        // SDL3 bypasses Browser.createContext's legacy GL initialization callback.
        EM_ASM({
            Browser.useWebGL = true;
            Module['GL_MAX_TEXTURE_IMAGE_UNITS'] = 2;
            GLImmediate.init();
        });
        unpackRowLength = 0;
        boundTexture = 0;
        if (opaqueTextures) SDL_memset(opaqueTextures, 0, textureCapacity * sizeof(bool));
    }
    return context;
}

void WebGLDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    if (type == GL_UNSIGNED_SHORT) { glDrawElements(mode, count, type, indices); return; }
    Require(count >= 0 && (type == GL_UNSIGNED_INT || type == GL_UNSIGNED_BYTE), "Unsupported client index format");
    static uint16_t* converted;
    static size_t capacity;
    if ((size_t)count > capacity)
    {
        converted = SDL_realloc(converted, (size_t)count * sizeof(*converted));
        Require(converted != NULL, "Cannot allocate WebGL index buffer");
        capacity = count;
    }
    for (int i = 0; i < count; i++)
    {
        uint32_t index = type == GL_UNSIGNED_INT ? ((const uint32_t*)indices)[i] : ((const uint8_t*)indices)[i];
        Require(index <= UINT16_MAX, "Mesh exceeds WebGL client-array index range");
        converted[i] = index;
    }
    glDrawElements(mode, count, GL_UNSIGNED_SHORT, converted);
}

void WebGLMaterialfv(GLenum face, GLenum pname, const GLfloat* values)
{
    if (pname == GL_AMBIENT_AND_DIFFUSE)
    {
        glMaterialfv(face, GL_AMBIENT, values);
        glMaterialfv(face, GL_DIFFUSE, values);
    }
    else glMaterialfv(face, pname, values);
}

void WebGLColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    // Legacy GL emulation lacks GL_COLOR_MATERIAL. These engines use it for
    // constant mesh colors; prelit vertex-colored meshes keep their color arrays.
    const GLfloat color[] = {r,g,b,a};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    glColor4f(r,g,b,a);
}
void WebGLColor4fv(const GLfloat* c) { WebGLColor4f(c[0],c[1],c[2],c[3]); }
void WebGLColor3f(GLfloat r, GLfloat g, GLfloat b) { WebGLColor4f(r,g,b,1); }
void WebGLColor3fv(const GLfloat* c) { WebGLColor4f(c[0],c[1],c[2],1); }
void WebGLEnable(GLenum cap) { if (cap != GL_COLOR_MATERIAL) glEnable(cap); }
void WebGLDisable(GLenum cap) { if (cap != GL_COLOR_MATERIAL) glDisable(cap); }
void WebGLHint(GLenum target, GLenum mode) { if (target != GL_FOG_HINT) glHint(target, mode); }
void WebGLPixelStorei(GLenum pname, GLint value)
{
    if (pname == GL_UNPACK_ROW_LENGTH) unpackRowLength = value;
    else glPixelStorei(pname, value);
}
void WebGLGetIntegerv(GLenum pname, GLint* value)
{
    if (pname == GL_UNPACK_ROW_LENGTH) *value = unpackRowLength;
    else glGetIntegerv(pname, value);
}
void WebGLBindTexture(GLenum target, GLuint texture)
{
    boundTexture = texture;
    glBindTexture(target, texture);
}

static uint8_t* Convert(int width, int height, GLenum format, GLenum type, const void* pixels, bool opaque)
{
    if (!pixels) return NULL;
    WebPixelFormat sourceFormat;
    if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV && format == GL_BGRA) sourceFormat = WEB_PIXEL_ARGB1555;
    else if (type == GL_UNSIGNED_INT_8_8_8_8 && format == GL_BGRA) sourceFormat = WEB_PIXEL_ABGR8888;
    else if (type == GL_UNSIGNED_INT_8_8_8_8_REV && format == GL_BGRA) sourceFormat = WEB_PIXEL_ARGB8888;
    else
    {
        Require(type == GL_UNSIGNED_BYTE, "Unsupported texture component type");
        switch (format)
        {
            case GL_RGB: sourceFormat = WEB_PIXEL_RGB8; break;
            case GL_RGBA: sourceFormat = WEB_PIXEL_RGBA8; break;
            case GL_BGR: sourceFormat = WEB_PIXEL_BGR8; break;
            case GL_BGRA: sourceFormat = WEB_PIXEL_BGRA8; break;
            default: Require(false, "Unsupported texture format"); return NULL;
        }
    }
    Require(width > 0 && height > 0 && (size_t)width <= SIZE_MAX / 4 / (size_t)height, "Invalid texture dimensions");
    uint8_t* converted = SDL_malloc((size_t)width * height * 4);
    Require(converted != NULL, "Cannot allocate texture conversion buffer");
    Require(WebConvertTextureRGBA(converted, pixels, width, height, unpackRowLength, sourceFormat, opaque), "Invalid texture stride");
    return converted;
}

void WebGLTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
    if (boundTexture >= textureCapacity)
    {
        size_t nextCapacity = (size_t)boundTexture + 256;
        opaqueTextures = SDL_realloc(opaqueTextures, nextCapacity * sizeof(bool));
        Require(opaqueTextures != NULL, "Cannot allocate texture metadata");
        SDL_memset(opaqueTextures + textureCapacity, 0, (nextCapacity - textureCapacity) * sizeof(bool));
        textureCapacity = nextCapacity;
    }
    bool opaque = internalFormat == GL_RGB || internalFormat == GL_RGB8 || internalFormat == GL_RGB5 || internalFormat == 3;
    opaqueTextures[boundTexture] = opaque;
    uint8_t* converted = Convert(width, height, format, type, pixels, opaque);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(target, level, GL_RGBA, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, converted);
    SDL_free(converted);
}
void WebGLTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
    bool opaque = boundTexture < textureCapacity && opaqueTextures[boundTexture];
    uint8_t* converted = Convert(width, height, format, type, pixels, opaque);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(target, level, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, converted);
    SDL_free(converted);
}
#endif
