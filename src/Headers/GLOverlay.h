#pragma once

#include "GLFunctions.h"
#include <array>

class GLOverlay
{
	static const int kMaxQuads			= 4;
	static const int kFloatsPerVertex	= 4;
	static const int kFloatsPerTriangle	= 3 * kFloatsPerVertex;
	static const int kFloatsPerQuad		= 2 * kFloatsPerTriangle;

	GLFunctions gl;
	GLuint vao;
	GLuint vbo;
	GLuint program;
	GLuint texture;
	int textureWidth;
	int textureHeight;
	unsigned char* textureData;
	bool ownTextureData;
	int nQuads;
	std::array<GLfloat, kMaxQuads * kFloatsPerQuad> vertexBufferData;

public:
	GLOverlay(int width, int height, unsigned char* pixels);

	GLOverlay(GLOverlay&&) noexcept;

	~GLOverlay();

	void UpdateTexture(int damageX, int damageY, int damageWidth, int damageHeight);

	void SubmitQuad(
		int srcX,
		int srcY,
		int srcW,
		int srcH,
		float dstX,
		float dstY,
		float dstW,
		float dstH
	);

	void FlushQuads(bool linearFiltering);

	void DumpTGA(const char* outFN, int x, int y, int w, int h);

	static GLOverlay CapturePixels(int windowWidth, int windowHeight);
};
