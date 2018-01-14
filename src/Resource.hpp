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

	extern MyGL::ShaderPtr ChunkShader, LineShader, SkyShader, SunShader;
	extern MyGL::TexturePtr ChunkTexture, SkyTexture, SunTexture, MoonTexture;
	extern MyGL::VertexObjectPtr CrosshairObject, BoxObject, SkyObject, SunObject, MoonObject;

	extern void InitResources();
}

#endif // RESOURCE_HPP
