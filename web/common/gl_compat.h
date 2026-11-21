#pragma once
// Thin, shared adapter from these games' client-array OpenGL to WebGL 1.
#ifdef __EMSCRIPTEN__
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#ifdef __cplusplus
extern "C" {
#endif
SDL_GLContext WebCreateGLContext(SDL_Window* window);
SDL_Window* WebCreateWindow(const char* title, int width, int height, SDL_WindowFlags flags);
bool WebSwapWindow(SDL_Window* window);
SDL_MouseButtonFlags WebGetMouseState(float* x, float* y);
bool WebRelativeMouseMode(SDL_Window* window, bool enabled);
bool WebFullscreen(SDL_Window* window, bool enabled);
SDL_FunctionPointer WebGLGetProcAddress(const char* name);
void WebGLGetFloatv(GLenum pname, GLfloat* values);
void WebGLActiveTexture(GLenum unit);
void WebGLClientActiveTexture(GLenum unit);
void WebGLTexGeni(GLenum coord, GLenum pname, GLint value);
void WebGLVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
void WebGLNormalPointer(GLenum type, GLsizei stride, const void* pointer);
void WebGLEnableClientState(GLenum cap);
void WebGLDisableClientState(GLenum cap);
void WebGLDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
void WebGLColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void WebGLColor4fv(const GLfloat* color);
void WebGLColor3f(GLfloat r, GLfloat g, GLfloat b);
void WebGLColor3fv(const GLfloat* color);
void WebGLMaterialfv(GLenum face, GLenum pname, const GLfloat* values);
void WebGLEnable(GLenum cap);
void WebGLDisable(GLenum cap);
void WebGLHint(GLenum target, GLenum mode);
void WebGLPixelStorei(GLenum pname, GLint value);
void WebGLGetIntegerv(GLenum pname, GLint* value);
void WebGLBindTexture(GLenum target, GLuint texture);
void WebGLTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
void WebGLTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
#ifdef __cplusplus
}
#endif
#ifndef WEB_GL_IMPLEMENTATION
#define SDL_GL_CreateContext WebCreateGLContext
#define SDL_CreateWindow WebCreateWindow
#define SDL_GL_SwapWindow WebSwapWindow
#define SDL_GetMouseState WebGetMouseState
#define SDL_SetWindowRelativeMouseMode WebRelativeMouseMode
#define SDL_SetWindowFullscreen WebFullscreen
#define glDrawElements WebGLDrawElements
#define glGetFloatv WebGLGetFloatv
#define glActiveTexture WebGLActiveTexture
#define glClientActiveTexture WebGLClientActiveTexture
#define glTexGeni WebGLTexGeni
#define glVertexPointer WebGLVertexPointer
#define glNormalPointer WebGLNormalPointer
#define glEnableClientState WebGLEnableClientState
#define glDisableClientState WebGLDisableClientState
#define glColor4f WebGLColor4f
#define glColor4fv WebGLColor4fv
#define glColor3f WebGLColor3f
#define glColor3fv WebGLColor3fv
#define glMaterialfv WebGLMaterialfv
#define glColorMaterial(face, mode) ((void)0)
#define glLightModeli(pname, value) glLightModelf(pname, (GLfloat)(value))
#define SDL_GL_GetProcAddress WebGLGetProcAddress
#define glEnable WebGLEnable
#define glDisable WebGLDisable
#define glHint WebGLHint
#define glPixelStorei WebGLPixelStorei
#define glGetIntegerv WebGLGetIntegerv
#define glBindTexture WebGLBindTexture
#define glTexImage2D WebGLTexImage2D
#define glTexSubImage2D WebGLTexSubImage2D
#endif
#endif
