#include "GLFunctions.h"
#include <SDL.h>
#include <stdexcept>

GLFunctions::GLFunctions()
{
	bool success = true;

	success &= nullptr != (AttachShader             = (PFNGLATTACHSHADERPROC)            SDL_GL_GetProcAddress("glAttachShader"           ));
	success &= nullptr != (BindAttribLocation       = (PFNGLBINDATTRIBLOCATIONPROC)      SDL_GL_GetProcAddress("glBindAttribLocation"     ));
	success &= nullptr != (BindBuffer               = (PFNGLBINDBUFFERPROC)              SDL_GL_GetProcAddress("glBindBuffer"             ));
	success &= nullptr != (BindVertexArray          = (PFNGLBINDVERTEXARRAYPROC)         SDL_GL_GetProcAddress("glBindVertexArray"        ));
	success &= nullptr != (BufferData               = (PFNGLBUFFERDATAPROC)              SDL_GL_GetProcAddress("glBufferData"             ));
	success &= nullptr != (CompileShader            = (PFNGLCOMPILESHADERPROC)           SDL_GL_GetProcAddress("glCompileShader"          ));
	success &= nullptr != (CreateProgram            = (PFNGLCREATEPROGRAMPROC)           SDL_GL_GetProcAddress("glCreateProgram"          ));
	success &= nullptr != (CreateShader             = (PFNGLCREATESHADERPROC)            SDL_GL_GetProcAddress("glCreateShader"           ));
	success &= nullptr != (DeleteBuffers            = (PFNGLDELETEBUFFERSPROC)           SDL_GL_GetProcAddress("glDeleteBuffers"          ));
	success &= nullptr != (DeleteProgram            = (PFNGLDELETEPROGRAMPROC)           SDL_GL_GetProcAddress("glDeleteProgram"          ));
	success &= nullptr != (DeleteShader             = (PFNGLDELETESHADERPROC)            SDL_GL_GetProcAddress("glDeleteShader"           ));
	success &= nullptr != (DeleteVertexArrays       = (PFNGLDELETEVERTEXARRAYSPROC)      SDL_GL_GetProcAddress("glDeleteVertexArrays"     ));
	success &= nullptr != (DetachShader             = (PFNGLDETACHSHADERPROC)            SDL_GL_GetProcAddress("glDetachShader"           ));
	success &= nullptr != (EnableVertexAttribArray  = (PFNGLENABLEVERTEXATTRIBARRAYPROC) SDL_GL_GetProcAddress("glEnableVertexAttribArray"));
	success &= nullptr != (GenBuffers               = (PFNGLGENBUFFERSPROC)              SDL_GL_GetProcAddress("glGenBuffers"             ));
	success &= nullptr != (GenVertexArrays          = (PFNGLGENVERTEXARRAYSPROC)         SDL_GL_GetProcAddress("glGenVertexArrays"        ));
	success &= nullptr != (GetShaderiv              = (PFNGLGETSHADERIVPROC)             SDL_GL_GetProcAddress("glGetShaderiv"            ));
	success &= nullptr != (GetUniformLocation       = (PFNGLGETUNIFORMLOCATIONPROC)      SDL_GL_GetProcAddress("glGetUniformLocation"     ));
	success &= nullptr != (LinkProgram              = (PFNGLLINKPROGRAMPROC)             SDL_GL_GetProcAddress("glLinkProgram"            ));
	success &= nullptr != (ShaderSource             = (PFNGLSHADERSOURCEPROC)            SDL_GL_GetProcAddress("glShaderSource"           ));
	success &= nullptr != (UniformMatrix4fv         = (PFNGLUNIFORMMATRIX4FVPROC)        SDL_GL_GetProcAddress("glUniformMatrix4fv"       ));
	success &= nullptr != (UseProgram               = (PFNGLUSEPROGRAMPROC)              SDL_GL_GetProcAddress("glUseProgram"             ));
	success &= nullptr != (VertexAttribPointer      = (PFNGLVERTEXATTRIBPOINTERPROC)     SDL_GL_GetProcAddress("glVertexAttribPointer"    ));

	if (!success)
	{
		throw std::runtime_error("Backdrop: SDL_GL_GetProcAddress failed.");
	}
}
