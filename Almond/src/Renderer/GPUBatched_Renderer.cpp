#include <GL/glew.h>
#include <array>

#include "GPUBatched_Renderer.h"
#include "OldCamera.h"
#include "GLCall.h"
#include "Shader.h"
#include "OpenGLBuffer.h"
#include "OpenGLVertexArray.h"

struct Quad
{
	struct Vertex
	{
		glm::vec3 VertexPosition;
		glm::vec3 Position;
		glm::vec2 Scale;
		float rotation;
		unsigned int Color;
		glm::vec2 TexCoord;
		float TexID;
	};
	Vertex topRight;
	Vertex bottomRight;
	Vertex bottomLeft;
	Vertex topLeft;
};

struct RendererData
{
	static const int MAX_BATCH_COUNT = 10000;
	static const int MAX_VERTEX_COUNT = MAX_BATCH_COUNT * 4;
	static const int MAX_INDEX_COUNT = MAX_BATCH_COUNT * 6;

	Quad* m_QuadBuffer;
	Quad* m_QuadBufferPtr = nullptr;

	std::unique_ptr<OpenGLVertexArray> m_VertexArray;
	std::unique_ptr<OpenGLVertexBuffer> m_VertexBuffer;
	std::unique_ptr<OpenGLIndexBuffer> m_IndexBuffer;
	std::unique_ptr<Shader> m_Shader;

	GPUBatched_Renderer::RenderStats m_RenderStats;

	static const int s_MaxTextureSlots = 32;
	std::array<int, s_MaxTextureSlots> m_TextureSlots;
	unsigned int m_TextureSlotIndex = 1; //0 = white texture;
	glm::vec4 m_Vertices[4];
	int indexCount = 0;

	glm::mat4 projection;
	glm::mat4 view;
	float fov;
};

static RendererData s_Data;

GPUBatched_Renderer::GPUBatched_Renderer()
{
	std::string shaderPath("resources/shaders/Basic.glsl");
	s_Data.m_Shader = std::make_unique<Shader>();
	s_Data.m_Shader->init(shaderPath);
	s_Data.m_Shader->use();

	s_Data.m_QuadBuffer = new Quad[s_Data.MAX_BATCH_COUNT];

	s_Data.m_VertexArray = std::make_unique<OpenGLVertexArray>();
	s_Data.m_VertexArray->Bind();

	s_Data.m_VertexBuffer = std::make_unique<OpenGLVertexBuffer>(nullptr, s_Data.MAX_VERTEX_COUNT * sizeof(Quad::Vertex));

	BufferLayout layout;
	layout.AddAttribute({ "aPos", BufferAttribType::Float3, false });
	layout.AddAttribute({ "aTranslate", BufferAttribType::Float3, false });
	layout.AddAttribute({ "aScale", BufferAttribType::Float2, false });
	layout.AddAttribute({ "aRotate", BufferAttribType::Float, false });
	layout.AddAttribute({ "aColor", BufferAttribType::UnsignedByte, true });
	layout.AddAttribute({ "aTexCoords", BufferAttribType::Float2, false });
	layout.AddAttribute({ "aTexId", BufferAttribType::Float, false });
	s_Data.m_VertexBuffer->SetLayout(layout);


	//EBO
	//Populates INDICES array as that will remain the same
	uint32_t indices[s_Data.MAX_INDEX_COUNT];
	uint32_t offset = 0;
	for (int i = 0; i < s_Data.MAX_INDEX_COUNT; i += 6)
	{
		indices[i + 0] = 0 + offset;
		indices[i + 1] = 1 + offset;
		indices[i + 2] = 3 + offset;

		indices[i + 3] = 1 + offset;
		indices[i + 4] = 2 + offset;
		indices[i + 5] = 3 + offset;

		offset += 4;
	}

	s_Data.m_QuadBufferPtr = s_Data.m_QuadBuffer;

	s_Data.m_IndexBuffer = std::make_unique<OpenGLIndexBuffer>(indices, sizeof(indices));


	int32_t samplers[s_Data.s_MaxTextureSlots];
	for (uint32_t i = 0; i < s_Data.s_MaxTextureSlots; ++i)
	{
		samplers[i] = i;
	}

	s_Data.m_Vertices[0] = { 0.5f, 0.5f, 0.0f, 1.0f }; // top right
	s_Data.m_Vertices[1] = { 0.5f, -0.5f, 0.0f, 1.0f };  // bottom right
	s_Data.m_Vertices[2] = { -0.5f, -0.5f, 0.0f, 1.0f };  // bottom left
	s_Data.m_Vertices[3] = { -0.5f,  0.5f, 0.0f, 1.0f };   // top left 


	s_Data.m_Shader->setIntArray("uTextures", samplers, s_Data.s_MaxTextureSlots);
}


void GPUBatched_Renderer::Shutdown()
{
	/*delete[] quadBuffer;*/
}

void GPUBatched_Renderer::BeginScene(OldCamera& camera)
{
	s_Data.projection = camera.GetProjectionMatrix();
	s_Data.view = camera.GetViewMatrix();

	
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	BeginBatch();
}

void GPUBatched_Renderer::BeginScene(EditorCamera& camera)
{
	s_Data.projection = camera.GetProjectionMatrix();
	s_Data.view = camera.GetViewMatrix();


	glClearColor(62.0f/255.0f, 62.0f / 255.0f, 58.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	BeginBatch();
}


void GPUBatched_Renderer::EndScene()
{
	EndBatch();
	Flush();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GPUBatched_Renderer::Submit(const glm::vec3 position, float rotation, glm::vec2 scale, glm::vec4 color, int textureID, glm::vec2* texCoords)
{
	s_Data.m_RenderStats.QuadCount++;

	if (s_Data.indexCount >= s_Data.MAX_INDEX_COUNT)
	{
		NextBatch();
	}

	float textureIndex = 0;

	//If textureID is already in batch
	for (unsigned int i = 0; i < s_Data.m_TextureSlotIndex; ++i)
	{
		if (s_Data.m_TextureSlots[i] == textureID)
		{
			//Set the textureIndex for this quad to the one already in the batch;
			textureIndex = i;
			break;
		}
	}

	//If textureID was not in batch
	if (textureIndex == 0 && textureID > 0)
	{
		//If all the textureSlots have been occupied, end batch
		if (s_Data.m_TextureSlotIndex >= s_Data.s_MaxTextureSlots)
		{
			NextBatch();
		}

		textureIndex = s_Data.m_TextureSlotIndex;
		s_Data.m_TextureSlots[s_Data.m_TextureSlotIndex] = textureID;
		s_Data.m_TextureSlotIndex++;
	}

	int r = color.r * 255;
	int g = color.g * 255;
	int b = color.b * 255;
	int a = color.a * 255;

	unsigned int c = a << 24 | b << 16 | g << 8 | r;
	float rot = -glm::radians(rotation);
	
	//top right
	s_Data.m_QuadBufferPtr->topRight = Quad::Vertex{s_Data.m_Vertices[0], position, scale,rot, c, texCoords[0], textureIndex};
	//bottom right
	s_Data.m_QuadBufferPtr->bottomRight = Quad::Vertex{ s_Data.m_Vertices[1], position, scale,rot, c, texCoords[1], textureIndex };
	//bottom left
	s_Data.m_QuadBufferPtr->bottomLeft = Quad::Vertex{ s_Data.m_Vertices[2], position, scale,rot, c, texCoords[2], textureIndex };;
	//top left
	s_Data.m_QuadBufferPtr->topLeft = Quad::Vertex{ s_Data.m_Vertices[3], position, scale,rot, c, texCoords[3], textureIndex };

	s_Data.m_QuadBufferPtr++;

	s_Data.indexCount += 6;

}

void GPUBatched_Renderer::BeginBatch()
{
	s_Data.m_QuadBufferPtr = s_Data.m_QuadBuffer;
	s_Data.indexCount = 0;
	s_Data.m_TextureSlotIndex = 1; //Set to 1 because 0 is reserved for colored quads
}

void GPUBatched_Renderer::EndBatch()
{
	s_Data.m_RenderStats.DrawCalls++;

	GLsizeiptr size = s_Data.m_QuadBufferPtr - s_Data.m_QuadBuffer;
	size = size * sizeof(Quad);

	s_Data.m_VertexBuffer->Bind();

	/*GLCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4 * MAX_BATCH_COUNT, vertexBuffer, GL_DYNAMIC_DRAW));*/
	GLCALL(glBufferSubData(GL_ARRAY_BUFFER, 0, size, s_Data.m_QuadBuffer));
}


void GPUBatched_Renderer::Flush()
{
	s_Data.m_Shader->use();
	s_Data.m_Shader->setMat4("uView", s_Data.view);
	s_Data.m_Shader->setMat4("uProjection", s_Data.projection);

	for (unsigned int i = 1; i < s_Data.m_TextureSlotIndex; ++i)
	{
		GLCALL(glActiveTexture(GL_TEXTURE0 + i));
		glBindTexture(GL_TEXTURE_2D, s_Data.m_TextureSlots[i]);
	}

	s_Data.m_VertexArray->Bind();
	s_Data.m_IndexBuffer->Bind();

	glDrawElements(GL_TRIANGLES, s_Data.indexCount, GL_UNSIGNED_INT, 0);
}


void GPUBatched_Renderer::NextBatch()
{
	EndBatch();
	Flush();
	BeginBatch();
}

void GPUBatched_Renderer::ResetStats()
{
	memset(&s_Data.m_RenderStats, 0, sizeof(RenderStats));
}

GPUBatched_Renderer::RenderStats GPUBatched_Renderer::GetStats()
{
	return s_Data.m_RenderStats;
}
