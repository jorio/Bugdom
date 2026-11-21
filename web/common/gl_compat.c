#ifdef __EMSCRIPTEN__
#define WEB_GL_IMPLEMENTATION
#include "gl_compat.h"
#include "webtexture.h"
#include "platform.h"
extern void WebEndFrame(void);
#include <emscripten.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

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
    if (SDL_strcmp(name, "glClientActiveTexture") == 0) return (SDL_FunctionPointer)WebGLClientActiveTexture;
    if (SDL_strcmp(name, "glActiveTexture") == 0) return (SDL_FunctionPointer)WebGLActiveTexture;
    return SDL_GL_GetProcAddress(name);
}

static GLfloat currentColor[4] = {1,1,1,1};
static int activeUnit, clientUnit;
static bool sphereS[2], sphereT[2], texCoordArray[2];
static const uint8_t* vertexPointer;
static const uint8_t* normalPointer;
static int vertexStride, normalStride;
static GLenum vertexType, normalType;
static GLint vertexSize;
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
            const jit = GLImmediate.TexEnvJIT;
            if (!jit.pangeaPatched) {
                const generate = jit.genAllPassLines;
                jit.genAllPassLines = (...args) => globalThis.PangeaGL.repairTexEnvShader(generate(...args));
                jit.pangeaPatched = true;
            }
        });
        unpackRowLength = 0;
        boundTexture = 0;
        if (opaqueTextures) SDL_memset(opaqueTextures, 0, textureCapacity * sizeof(bool));
    }
    return context;
}

static void DrawConvertedElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
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

void WebGLGetFloatv(GLenum pname, GLfloat* values)
{
    if (pname == GL_CURRENT_COLOR) SDL_memcpy(values, currentColor, sizeof(currentColor));
    else glGetFloatv(pname, values);
}
void WebGLActiveTexture(GLenum unit) { activeUnit = unit - GL_TEXTURE0; glActiveTexture(unit); }
void WebGLClientActiveTexture(GLenum unit) { clientUnit = unit - GL_TEXTURE0; glClientActiveTexture(unit); }
void WebGLVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    vertexPointer = pointer; vertexType = type; vertexSize = size;
    vertexStride = stride ? stride : size * (int)sizeof(float);
    glVertexPointer(size, type, stride, pointer);
}
void WebGLNormalPointer(GLenum type, GLsizei stride, const void* pointer)
{
    normalPointer = pointer; normalType = type;
    normalStride = stride ? stride : 3 * (int)sizeof(float);
    glNormalPointer(type, stride, pointer);
}
void WebGLEnableClientState(GLenum cap)
{
    if (cap == GL_TEXTURE_COORD_ARRAY && clientUnit < 2) texCoordArray[clientUnit] = true;
    glEnableClientState(cap);
}
void WebGLDisableClientState(GLenum cap)
{
    if (cap == GL_TEXTURE_COORD_ARRAY && clientUnit < 2) texCoordArray[clientUnit] = false;
    glDisableClientState(cap);
}
void WebGLTexGeni(GLenum coord, GLenum pname, GLint value)
{
    Require((coord == GL_S || coord == GL_T) && pname == GL_TEXTURE_GEN_MODE && value == GL_SPHERE_MAP,
        "Only sphere texture generation is supported by these games");
}

void WebGLDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    bool sphere = sphereS[0] || sphereT[0] || sphereS[1] || sphereT[1];
    if (!sphere || !count) { DrawConvertedElements(mode, count, type, indices); return; }
    Require(vertexType == GL_FLOAT && vertexSize == 3 && normalType == GL_FLOAT && vertexPointer && normalPointer,
        "Sphere mapping requires float positions and normals");
    uint32_t maxIndex = 0;
    for (int k = 0; k < count; k++)
    {
        uint32_t index = type == GL_UNSIGNED_INT ? ((const uint32_t*)indices)[k]
            : type == GL_UNSIGNED_SHORT ? ((const uint16_t*)indices)[k] : ((const uint8_t*)indices)[k];
        if (index > maxIndex) maxIndex = index;
    }
    Require(maxIndex <= UINT16_MAX, "Sphere mesh exceeds client-array range");
    static GLfloat uv[2 * 65536];
    GLfloat m[16]; glGetFloatv(GL_MODELVIEW_MATRIX, m);
    // Inverse transpose of the model-view's upper 3x3, including nonuniform scale.
    float a=m[0], b=m[4], c=m[8], d=m[1], e=m[5], f=m[9], g=m[2], h=m[6], i=m[10];
    float co[] = {e*i-f*h, f*g-d*i, d*h-e*g, c*h-b*i, a*i-c*g, b*g-a*h, b*f-c*e, c*d-a*f, a*e-b*d};
    float det = a*co[0] + b*co[1] + c*co[2];
    float sign = det < 0 ? -1 : 1;
    for (uint32_t k = 0; k <= maxIndex; k++)
    {
        const float* p = (const float*)(vertexPointer + k*vertexStride);
        const float* n = (const float*)(normalPointer + k*normalStride);
        float x=m[0]*p[0]+m[4]*p[1]+m[8]*p[2]+m[12];
        float y=m[1]*p[0]+m[5]*p[1]+m[9]*p[2]+m[13];
        float z=m[2]*p[0]+m[6]*p[1]+m[10]*p[2]+m[14];
        float nx=sign*(co[0]*n[0]+co[1]*n[1]+co[2]*n[2]);
        float ny=sign*(co[3]*n[0]+co[4]*n[1]+co[5]*n[2]);
        float nz=sign*(co[6]*n[0]+co[7]*n[1]+co[8]*n[2]);
        float len=fmaxf(1e-20f,sqrtf(x*x+y*y+z*z)); x/=len; y/=len; z/=len;
        len=fmaxf(1e-20f,sqrtf(nx*nx+ny*ny+nz*nz)); nx/=len; ny/=len; nz/=len;
        float dot=2*(x*nx+y*ny+z*nz);
        x-=dot*nx; y-=dot*ny; z=1+z-dot*nz;
        len=fmaxf(1e-20f,2*sqrtf(x*x+y*y+z*z));
        uv[2*k]=x/len+.5f; uv[2*k+1]=y/len+.5f;
    }
    for (int unit=0; unit<2; unit++) if (sphereS[unit] || sphereT[unit])
    {
        glClientActiveTexture(GL_TEXTURE0+unit);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, uv);
    }
    DrawConvertedElements(mode, count, type, indices);
    for (int unit=0; unit<2; unit++) if ((sphereS[unit] || sphereT[unit]) && !texCoordArray[unit])
    {
        glClientActiveTexture(GL_TEXTURE0+unit);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glClientActiveTexture(GL_TEXTURE0+clientUnit);
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
    SDL_memcpy(currentColor, color, sizeof(currentColor));
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    glColor4f(r,g,b,a);
}
void WebGLColor4fv(const GLfloat* c) { WebGLColor4f(c[0],c[1],c[2],c[3]); }
void WebGLColor3f(GLfloat r, GLfloat g, GLfloat b) { WebGLColor4f(r,g,b,1); }
void WebGLColor3fv(const GLfloat* c) { WebGLColor4f(c[0],c[1],c[2],1); }
void WebGLEnable(GLenum cap)
{
    if (cap == GL_TEXTURE_GEN_S && activeUnit < 2) sphereS[activeUnit] = true;
    else if (cap == GL_TEXTURE_GEN_T && activeUnit < 2) sphereT[activeUnit] = true;
    else if (cap != GL_COLOR_MATERIAL) glEnable(cap);
}
void WebGLDisable(GLenum cap)
{
    if (cap == GL_TEXTURE_GEN_S && activeUnit < 2) sphereS[activeUnit] = false;
    else if (cap == GL_TEXTURE_GEN_T && activeUnit < 2) sphereT[activeUnit] = false;
    else if (cap != GL_COLOR_MATERIAL) glDisable(cap);
}
void WebGLHint(GLenum target, GLenum mode) { if (target != GL_FOG_HINT) glHint(target, mode); }
void WebGLPixelStorei(GLenum pname, GLint value)
{
    if (pname == GL_UNPACK_ROW_LENGTH) unpackRowLength = value;
    else glPixelStorei(pname, value);
}
void WebGLGetIntegerv(GLenum pname, GLint* value)
{
    if (pname == GL_UNPACK_ROW_LENGTH) *value = unpackRowLength;
    else if (pname == GL_BLEND_SRC) glGetIntegerv(GL_BLEND_SRC_RGB, value);
    else if (pname == GL_BLEND_DST) glGetIntegerv(GL_BLEND_DST_RGB, value);
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
