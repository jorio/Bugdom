#pragma once

enum
{
	OVERLAY_FILL = 0,
	OVERLAY_PILLARBOX = 1,
	OVERLAY_LETTERBOX = 2,
	OVERLAY_CLEAR_BLACK = 4,
	OVERLAY_FIT = OVERLAY_PILLARBOX | OVERLAY_LETTERBOX,
};

#ifdef __cplusplus

#include "GLFunctions.h"
#include <array>

class GLOverlayQuad
{
	int srcX;
	int srcY;
	int srcW;
	int srcH;
	float dstX;
	float dstY;
	float dstW;
	float dstH;
};

class GLOverlay
{
	GLFunctions gl;
	GLuint vao;
	GLuint vbo;
	GLuint program;
	GLuint texture;
	int textureWidth;
	int textureHeight;
	unsigned char* textureData;
	bool needClear;
	std::array<GLfloat, 4 * 6> vertexBufferData;
	std::vector<GLOverlayQuad> quads;

public:
	GLOverlay(int width, int height, unsigned char* pixels);

	~GLOverlay();

	void UpdateTexture(int damageX, int damageY, int damageWidth, int damageHeight);

	void UpdateQuad(int windowWidth, int windowHeight, int fit);

	void Render(int windowWidth, int windowHeight, bool linearFiltering);
};

#endif
