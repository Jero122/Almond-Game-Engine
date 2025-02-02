#pragma once
#include <GL/glew.h>
class GLRenderCommand
{
public:
	static void Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	static void ClearColor(float r, float g, float b, float a)
	{
		glClearColor(r, g , b , a);
	}

	static void UnbindFrameBuffer()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	static void BindTexture(int texture, int slot)
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	static void BindTextureUnit(unsigned int unit, unsigned int texture)
	{
		glBindTextureUnit(unit, texture);
	}

	static void BindTextures(unsigned int firstUnit, int count, const unsigned int* textures)
	{
		glBindTextures(firstUnit, count, textures);
	}

	static void DrawElementsTriangle(int count, const void* indices)
	{
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
	}
};
