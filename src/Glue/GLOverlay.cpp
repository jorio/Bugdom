#include "GLOverlay.h"
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <fstream>

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
	if (pixels == nullptr)
	{
		ownTextureData = true;
		pixels = new unsigned char[width*height*4];
	}
	else
	{
		ownTextureData = false;
	}

	this->textureWidth = width;
	this->textureHeight = height;
	this->textureData = pixels;
	this->nQuads = 0;

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

GLOverlay::GLOverlay(GLOverlay&& other) noexcept
	: gl()	// TODO: not efficient
	, vao(other.vao)
	, vbo(other.vbo)
	, program(other.program)
	, texture(other.texture)
	, textureWidth(other.textureWidth)
	, textureHeight(other.textureHeight)
	, textureData(other.textureData)
	, ownTextureData(other.ownTextureData)
	, nQuads(other.nQuads)
	, vertexBufferData(other.vertexBufferData)
{
	other.ownTextureData = false;
	other.textureData = nullptr;
}

GLOverlay::~GLOverlay()
{
	glDeleteTextures(1, &texture);
	gl.DeleteProgram(program);
	gl.DeleteBuffers(1, &vbo);
	gl.DeleteVertexArrays(1, &vao);

	if (ownTextureData)
	{
		delete[] textureData;
		textureData = nullptr;
		ownTextureData = false;
	}
}

void GLOverlay::SubmitQuad(
		int srcX, int srcY, int srcW, int srcH,
		float dstX, float dstY, float dstW, float dstH)
{
	// Compute normalized device coordinates for the quad vertices.
	float ndcLeft   = 2.0f * (dstX       ) - 1.0f;
	float ndcRight  = 2.0f * (dstX + dstW) - 1.0f;
	float ndcTop    = 1.0f - 2.0f * (dstY       );
	float ndcBottom = 1.0f - 2.0f * (dstY + dstH);

	// Compute texture coordinates.
	float uLeft     = (float)(srcX       ) / textureWidth;
	float uRight    = (float)(srcX + srcW) / textureWidth;
	float vTop      = (float)(srcY       ) / textureHeight;
	float vBottom   = (float)(srcY + srcH) / textureHeight;

	float* v = &vertexBufferData[nQuads * kFloatsPerQuad];

//	X------------------ Y------------------ U------------------ U------------------
	*(v++) = ndcLeft;	*(v++) = ndcTop;	*(v++) = uLeft; 	*(v++) = vTop;			// First triangle
	*(v++) = ndcRight;	*(v++) = ndcTop;	*(v++) = uRight;	*(v++) = vTop;
	*(v++) = ndcRight;	*(v++) = ndcBottom;	*(v++) = uRight;	*(v++) = vBottom;
	*(v++) = ndcLeft;	*(v++) = ndcTop;	*(v++) = uLeft;		*(v++) = vTop;			// Second triangle
	*(v++) = ndcRight;	*(v++) = ndcBottom;	*(v++) = uRight;	*(v++) = vBottom;
	*(v++) = ndcLeft;	*(v++) = ndcBottom;	*(v++) = uLeft;		*(v++) = vBottom;

	nQuads++;
}

void GLOverlay::UpdateTexture(int x, int y, int w, int h)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, textureWidth);
	// Don't use GL_UNPACK_SKIP_ROWS/GL_UNPACK_SKIP_PIXELS because it interferes with Quesa
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, textureData + (y * textureWidth + x) * 4);
}

void GLOverlay::FlushQuads(bool linearFiltering)
{
	if (nQuads == 0)
		return;

	gl.UseProgram(program);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, texture);
	GLint textureFilter = linearFiltering ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureFilter);

	gl.BindVertexArray(vao);
	gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
	gl.EnableVertexAttribArray(kAttribIdVertexPos);
	gl.EnableVertexAttribArray(kAttribIdTexCoord);

	gl.VertexAttribPointer(
			kAttribIdVertexPos,							// index
			2,											// size (number of components)
			GL_FLOAT,									// type
			GL_FALSE,									// normalized
			kFloatsPerVertex * sizeof(float),			// stride
			0											// pointer offset
	);

	gl.VertexAttribPointer(
			kAttribIdTexCoord,							// index
			2,											// size (number of components)
			GL_FLOAT,									// type
			GL_FALSE,									// normalized
			kFloatsPerVertex * sizeof(float),			// stride
			(void*) (sizeof(float) * 2)					// pointer offset
	);

	gl.BufferData(
			GL_ARRAY_BUFFER,							// buffer
			nQuads * kFloatsPerQuad * sizeof(float),	// size (bytes)
			vertexBufferData.data(),					// data
			GL_STATIC_DRAW								// usage
	);

	gl.BindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 2 * nQuads * 3);

	nQuads = 0;
}

void GLOverlay::DumpTGA(const char* outFN, int x, int y, int w, int h)
{
	std::ofstream out(outFN, std::ios::out | std::ios::binary);
	uint16_t TGAhead[] = {0, 2, 0, 0, 0, 0, (uint16_t) w, (uint16_t) h, 0x0820};
	out.write(reinterpret_cast<char*>(&TGAhead), sizeof(TGAhead));

	char* scanline = (char*)textureData + (y*textureWidth*4) + (x*4);
	for (int i = 0; i < h; i++)
	{
		char* pixel = scanline;
		for (int j = 0; j < w; j++)
		{
			out.write(pixel+3, 1);
			out.write(pixel+2, 1);
			out.write(pixel+1, 1);
			out.write(pixel, 1);
			pixel += 4;
		}
		scanline += 4*textureWidth;
	}

	printf("GLOverlay: wrote %s\n", outFN);
}

GLOverlay GLOverlay::CapturePixels(int width, int height)
{
	int width4rem = width % 4;
	int width4ceil = width - width4rem + (width4rem == 0? 0: 4);

	GLOverlay capture(width4ceil, height, nullptr);

	glReadPixels(0, 0, width4ceil, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, capture.textureData);
	CHECK_GL_ERROR();

	capture.UpdateTexture(0, 0, width, height);

	//capture.DumpTGA("/tmp/Overlay_Capture.TGA", 0, 0, width, height);

	return capture;
}
