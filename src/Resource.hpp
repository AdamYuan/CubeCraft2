#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <MyGL/Shader.hpp>
#include <MyGL/Texture.hpp>
#include <MyGL/VertexObject.hpp>

#include <string>

namespace Resource
{
	constexpr int
			ATTR_POSITION = 0, ATTR_TEXCOORD = 1,
			ATTR_CHUNK_FACE = 2, ATTR_CHUNK_LIGHTING = 3;

	extern const char *UNIF_SAMPLER, *UNIF_MATRIX;

	extern MyGL::ShaderPtr ChunkShader, LineShader;
	extern MyGL::TexturePtr ChunkTexture;
	extern MyGL::VertexObjectPtr CrosshairObject, BoxObject;

	extern void InitResources();
}

#endif // RESOURCE_HPP
