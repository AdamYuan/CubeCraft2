#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <mygl3/shader.hpp>
#include <mygl3/texture.hpp>
#include <mygl3/vertexobject.hpp>

#include <string>

namespace Resource
{
	constexpr int
			ATTR_POSITION = 0, ATTR_TEXCOORD = 1,
			ATTR_CHUNK_FACE = 2, ATTR_CHUNK_LIGHTING = 3;

	extern mygl3::Shader ChunkShader, LineShader, SkyShader, SunShader, BgShader;
	extern GLint ChunkShader_sampler, ChunkShader_skySampler, ChunkShader_camera, ChunkShader_viewDistance, ChunkShader_dayTime, ChunkShader_dayLight, ChunkShader_matrix, ChunkShader_selection;
	extern GLint LineShader_matrix;
	extern GLint SkyShader_matrix, SkyShader_sampler, SkyShader_dayTime;
	extern GLint SunShader_matrix, SunShader_sampler;
	extern GLint BgShader_matrix, BgShader_sampler;
	extern mygl3::Texture2D SkyTexture, SunTexture, MoonTexture;
	extern mygl3::Texture2DArray ChunkTexture;
	extern mygl3::VertexObject<false> CrosshairObject, SunObject, MoonObject, BgObject;
	extern mygl3::VertexObject<true> SkyObject;

	extern void InitResources();
}

#endif // RESOURCE_HPP
