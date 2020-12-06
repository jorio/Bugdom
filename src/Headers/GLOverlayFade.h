#pragma once

#include "GLFunctions.h"
#include <array>

class GLOverlayFade
{
	static const int kFloatsPerVertex	= 2;
	static const int kFloatsPerTriangle	= 3 * kFloatsPerVertex;
	static const int kFloatsPerQuad		= 2 * kFloatsPerTriangle;

	GLFunctions gl;
	GLuint vao;
	GLuint vbo;
	GLuint program;
	GLint fadeColorUniformLoc;

	static constexpr float vertexBufferData[kFloatsPerQuad] =
	{
		-1.0f,		 1.0f,
		 1.0f,		 1.0f,
		 1.0f,		-1.0f,
		-1.0f,		 1.0f,
		 1.0f,		-1.0f,
		-1.0f,		-1.0f,
	};

public:
	GLOverlayFade();

	~GLOverlayFade();

	void DrawFade(float red, float green, float blue, float alpha);
};
