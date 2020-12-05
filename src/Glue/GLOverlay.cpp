#include "GLOverlay.h"
#include <cmath>
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

#define CHECK_GL_ERROR() CheckGLError("GLOverlay", __LINE__)

static const char* kVertexShaderSource = R"(#version 150 core
out vec2 texCoord;
in vec2 texCoordIn;
in vec2 vertexPos;
void main() {
    gl_Position = vec4(vertexPos.xy, 0.0, 1.0);
    texCoord = texCoordIn;
}
)";

static const char* kFragmentShaderSource = R"(#version 150 core
in vec2 texCoord;
out vec4 color;
uniform sampler2D image;
void main() {
    color = texture(image, texCoord);
}
)";

enum
{
	kAttribIdVertexPos,
	kAttribIdTexCoord,
};

GLOverlay::GLOverlay(
	int width,
	int height,
	unsigned char* pixels)
	: gl()
{
	this->textureWidth = width;
	this->textureHeight = height;
	this->textureData = pixels;
	this->needClear = false;

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
	gl.BindAttribLocation(program, kAttribIdTexCoord, "texCoord");
	gl.LinkProgram(program);
	CHECK_GL_ERROR();

	gl.DetachShader(program, vertexShader);
	gl.DetachShader(program, fragmentShader);
	gl.DeleteShader(vertexShader);
	gl.DeleteShader(fragmentShader);
	CHECK_GL_ERROR();

	gl.GenVertexArrays(1, &vao);
	gl.GenBuffers(1, &vbo);
	CHECK_GL_ERROR();

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureWidth, textureHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, textureData);
	CHECK_GL_ERROR();
}

GLOverlay::~GLOverlay()
{
	glDeleteTextures(1, &texture);
	gl.DeleteProgram(program);
	gl.DeleteBuffers(1, &vbo);
	gl.DeleteVertexArrays(1, &vao);
}

void GLOverlay::UpdateQuad(
	const int windowWidth,
	const int windowHeight,
	int fit)
{
	float screenLeft   = 0.0f;
	float screenRight  = (float)windowWidth;
	float screenTop    = 0.0f;
	float screenBottom = (float)windowHeight;
	needClear = false;

	// Adjust screen coordinates if we want to pillarbox/letterbox the image.
	if (fit & (OVERLAY_LETTERBOX | OVERLAY_PILLARBOX))
	{
		const float targetAspectRatio = (float) windowWidth / windowHeight;
		const float sourceAspectRatio = (float) textureWidth / textureHeight;

		if (fabs(sourceAspectRatio - targetAspectRatio) < 0.1)
		{
			// source and window have nearly the same aspect ratio -- fit (no-op)
		}
		else if ((fit & OVERLAY_LETTERBOX) && sourceAspectRatio > targetAspectRatio)
		{
			// source is wider than window -- letterbox
			needClear = true;
			float letterboxedHeight = windowWidth / sourceAspectRatio;
			screenTop = (windowHeight - letterboxedHeight) / 2;
			screenBottom = screenTop + letterboxedHeight;
		}
		else if ((fit & OVERLAY_PILLARBOX) && sourceAspectRatio < targetAspectRatio)
		{
			// source is narrower than window -- pillarbox
			needClear = true;
			float pillarboxedWidth = sourceAspectRatio * windowWidth / targetAspectRatio;
			screenLeft = (windowWidth / 2.0f) - (pillarboxedWidth / 2.0f);
			screenRight = screenLeft + pillarboxedWidth;
		}
	}

	// Compute normalized device coordinates for the quad vertices.
	float ndcLeft   = 2.0f * screenLeft  / windowWidth - 1.0f;
	float ndcRight  = 2.0f * screenRight / windowWidth - 1.0f;
	float ndcTop    = 1.0f - 2.0f * screenTop    / windowHeight;
	float ndcBottom = 1.0f - 2.0f * screenBottom / windowHeight;

	// Compute texture coordinates.
	float uLeft     = 0.0f;
	float uRight    = 1.0f;
	float vTop      = 0.0f;
	float vBottom   = 1.0f;

	vertexBufferData = {
		// First triangle
		ndcLeft,  ndcTop,         uLeft,  vTop,
		ndcRight, ndcTop,         uRight, vTop,
		ndcRight, ndcBottom,      uRight, vBottom,
		// Second triangle
		ndcLeft,  ndcTop,         uLeft,  vTop,
		ndcRight, ndcBottom,      uRight, vBottom,
		ndcLeft,  ndcBottom,      uLeft,  vBottom,
	};
}

void GLOverlay::UpdateTexture(int x, int y, int w, int h)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, textureWidth);
	// Don't use GL_UNPACK_SKIP_ROWS/GL_UNPACK_SKIP_PIXELS because it interferes with Quesa
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, textureData + (y * textureWidth + x) * 4);
}

void GLOverlay::Render(
	int windowWidth,
	int windowHeight,
	bool linearFiltering
	)
{
	gl.UseProgram(program);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	glViewport(0, 0, windowWidth, windowHeight);

	glBindTexture(GL_TEXTURE_2D, texture);
	GLint textureFilter = linearFiltering ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureFilter);

	gl.BindVertexArray(vao);
	gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
	gl.EnableVertexAttribArray(kAttribIdVertexPos);
	gl.EnableVertexAttribArray(kAttribIdTexCoord);
	gl.VertexAttribPointer(kAttribIdVertexPos, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	gl.VertexAttribPointer(kAttribIdTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*) (sizeof(float) * 2));

	gl.BufferData(GL_ARRAY_BUFFER, vertexBufferData.size() * sizeof(float), vertexBufferData.data(), GL_STATIC_DRAW);
	gl.BindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
