#pragma once

#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

class GLFunctions
{
public:
	PFNGLATTACHSHADERPROC             AttachShader;
	PFNGLBINDATTRIBLOCATIONPROC       BindAttribLocation;
	PFNGLBINDBUFFERPROC               BindBuffer;
	PFNGLBINDVERTEXARRAYPROC          BindVertexArray;
	PFNGLBUFFERDATAPROC               BufferData;
	PFNGLCOMPILESHADERPROC            CompileShader;
	PFNGLCREATEPROGRAMPROC            CreateProgram;
	PFNGLCREATESHADERPROC             CreateShader;
	PFNGLDELETEBUFFERSPROC            DeleteBuffers;
	PFNGLDELETEPROGRAMPROC            DeleteProgram;
	PFNGLDELETESHADERPROC             DeleteShader;
	PFNGLDELETEVERTEXARRAYSPROC       DeleteVertexArrays;
	PFNGLDETACHSHADERPROC             DetachShader;
	PFNGLENABLEVERTEXATTRIBARRAYPROC  EnableVertexAttribArray;
	PFNGLGENBUFFERSPROC               GenBuffers;
	PFNGLGENVERTEXARRAYSPROC          GenVertexArrays;
	PFNGLGETSHADERIVPROC              GetShaderiv;
	PFNGLGETUNIFORMLOCATIONPROC       GetUniformLocation;
	PFNGLLINKPROGRAMPROC              LinkProgram;
	PFNGLSHADERSOURCEPROC             ShaderSource;
	PFNGLUNIFORMMATRIX4FVPROC         UniformMatrix4fv;
	PFNGLUSEPROGRAMPROC               UseProgram;
	PFNGLVERTEXATTRIBPOINTERPROC      VertexAttribPointer;

	GLFunctions();
};
