#include "GLOverlayFade.h"
#include <cstring>
#include <cstdio>
#include <stdexcept>

static void CheckGLError(const char* file, const int line)
{
	static char buf[256];
	auto error = glGetError();
	if (error != 0)
	{
		snprintf(buf, 256, "OpenGL Error %d in %s:%d", error, file, line);
		throw std::runtime_error(buf);
	}
}

#define CHECK_GL_ERROR() CheckGLError("GLOverlayFade", __LINE__)

static const char* kVertexShaderSource = R"(#version 150 core
in vec2 vertexPos;
void main() {
	gl_Position = vec4(vertexPos.xy, 0.0, 1.0);
}
)";

static const char* kFragmentShaderSource = R"(#version 150 core
out vec4 color;
uniform vec4 fadeColor;
void main() {
	color.rgb = fadeColor.rgb * fadeColor.a;
	color.a = fadeColor.a;
}
)";

enum
{
	kAttribIdVertexPos,
};

GLOverlayFade::GLOverlayFade()
	: gl()
{
	GLint status;

	GLuint vertexShader = gl.CreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = gl.CreateShader(GL_FRAGMENT_SHADER);

	const GLint vsLength = (GLint) strlen(kVertexShaderSource);
	gl.ShaderSource(vertexShader, 1, (const GLchar**) &kVertexShaderSource, &vsLength);
	gl.CompileShader(vertexShader);
	gl.GetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) throw std::runtime_error("vertex shader compilation failed");

	const GLint fsLength = (GLint) strlen(kFragmentShaderSource);
	gl.ShaderSource(fragmentShader, 1, (const GLchar**) &kFragmentShaderSource, &fsLength);
	gl.CompileShader(fragmentShader);
	gl.GetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) throw std::runtime_error("fragment shader compilation failed");

	program = gl.CreateProgram();
	gl.AttachShader(program, vertexShader);
	gl.AttachShader(program, fragmentShader);
	gl.BindAttribLocation(program, kAttribIdVertexPos, "vertexPos");
	gl.LinkProgram(program);
	CHECK_GL_ERROR();

	fadeColorUniformLoc = gl.GetUniformLocation(program, "fadeColor");
	CHECK_GL_ERROR();

	gl.DetachShader(program, vertexShader);
	gl.DetachShader(program, fragmentShader);
	gl.DeleteShader(vertexShader);
	gl.DeleteShader(fragmentShader);
	CHECK_GL_ERROR();

	gl.GenVertexArrays(1, &vao);
	gl.GenBuffers(1, &vbo);
	CHECK_GL_ERROR();
}

GLOverlayFade::~GLOverlayFade()
{
	gl.DeleteProgram(program);
	gl.DeleteBuffers(1, &vbo);
	gl.DeleteVertexArrays(1, &vao);
}

void GLOverlayFade::DrawFade(float red, float green, float blue, float alpha)
{
	if (alpha < 0.001f)
		return;

	gl.UseProgram(program);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	gl.BindVertexArray(vao);
	gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
	gl.EnableVertexAttribArray(kAttribIdVertexPos);

	gl.Uniform4f(fadeColorUniformLoc, red, green, blue, alpha);

	gl.VertexAttribPointer(
			kAttribIdVertexPos,							// index
			2,											// size (number of components)
			GL_FLOAT,									// type
			GL_FALSE,									// normalized
			kFloatsPerVertex * sizeof(float),			// stride
			0											// pointer offset
	);

	gl.BufferData(
			GL_ARRAY_BUFFER,							// buffer
			1 * kFloatsPerQuad * sizeof(float),			// size (bytes)
			vertexBufferData,							// data
			GL_STATIC_DRAW								// usage
	);

	gl.BindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 2*3);

	glDisable(GL_BLEND);
}
