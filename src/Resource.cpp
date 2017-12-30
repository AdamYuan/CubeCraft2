#include "Resource.hpp"
#include "Block.hpp"

#define TEXTURE_PATH(str) std::string("resources/texture/") + str
#define SHADER_PATH(str) std::string("resources/shader/") + str

MyGL::ShaderPtr Resource::ChunkShader;
MyGL::TexturePtr Resource::ChunkTexture;

void Resource::InitResources()
{
	ChunkShader = MyGL::NewShader();
	ChunkTexture = MyGL::NewTexture();

	ChunkTexture->Load2dArray(TEXTURE_PATH("blocks.png"), BLOCKS_TEXTURE_NUM);
	ChunkTexture->SetParameters(GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST, GL_REPEAT);
	ChunkTexture->BuildMipmap();

	ChunkShader->LoadFromFile(SHADER_PATH("block.frag"), GL_FRAGMENT_SHADER);
	ChunkShader->LoadFromFile(SHADER_PATH("block.vert"), GL_VERTEX_SHADER);
}
