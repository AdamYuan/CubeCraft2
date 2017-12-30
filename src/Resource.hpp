#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <MyGL/Shader.hpp>
#include <MyGL/Texture.hpp>
#include <string>

namespace Resource
{

	constexpr int
			ATTR_POSITION = 0, ATTR_TEXCOORD = 1,
			ATTR_CHUNK_FACE = 2, ATTR_CHUNK_LIGHTING = 3;

	const std::string UNIF_SAMPLER = "sampler";

	extern MyGL::ShaderPtr ChunkShader;
	extern MyGL::TexturePtr ChunkTexture;


	extern void InitResources();

}

#endif // RESOURCE_HPP
