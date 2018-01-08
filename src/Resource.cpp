#include "Resource.hpp"
#include "Block.hpp"


#define TEXTURE_PATH(str) std::string("resources/texture/") + str
#define SHADER_PATH(str) std::string("resources/shader/") + str

namespace Resource
{
	MyGL::ShaderPtr ChunkShader, LineShader;
	MyGL::TexturePtr ChunkTexture;
	MyGL::VertexObjectPtr CrosshairObject;

	const char *UNIF_SAMPLER = "sampler", *UNIF_MATRIX = "matrix";

	void InitResources()
	{
		ChunkShader = MyGL::NewShader();
		ChunkShader->LoadFromFile(SHADER_PATH("block.frag"), GL_FRAGMENT_SHADER);
		ChunkShader->LoadFromFile(SHADER_PATH("block.vert"), GL_VERTEX_SHADER);

		LineShader = MyGL::NewShader();
		LineShader->LoadFromFile(SHADER_PATH("line.frag"), GL_FRAGMENT_SHADER);
		LineShader->LoadFromFile(SHADER_PATH("line.vert"), GL_VERTEX_SHADER);

		ChunkTexture = MyGL::NewTexture();
		ChunkTexture->Load2dArray(TEXTURE_PATH("blocks.png"), BLOCKS_TEXTURE_NUM);
		ChunkTexture->SetParameters(GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST, GL_REPEAT);
		ChunkTexture->BuildMipmap();


		float _size = 10.0, _width = 1.0f;
		static const float vertices[] = {-_width, -_size, -_width, _size, _width, -_size,
										 _width, -_size, -_width, _size, _width, _size,
										 -_size, _width, _size, -_width, -_size, -_width,
										 _size, _width, _size, -_width, -_size, _width,
										 -_width, -_width, -_width, _width, _width, -_width,
										 _width, -_width, -_width, _width, _width, _width};
		CrosshairObject = MyGL::NewVertexObject();
		CrosshairObject->SetDataArr(vertices, 36);
		CrosshairObject->SetAttributes(1, ATTR_POSITION, 2);
	}
}

