//
// Created by adamyuan on 1/10/18.
//
#include "Renderer.hpp"
#include "Resource.hpp"
#include "World.hpp"
#include "ChunkAlgorithm.hpp"
#include <MyGL/Frustum.hpp>

void Renderer::RenderWorld(const World &wld, const glm::mat4 &vp_matrix, const glm::vec3 &position,
						   const glm::ivec3 &selection)
{
	static MyGL::Frustum frustum = {};
	const float range = CHUNK_SIZE * (Setting::ChunkLoadRange - 2);

	glm::vec3 p_center = (glm::vec3)(World::BlockPosToChunkPos(glm::floor(position)));

	frustum.CalculatePlanes(vp_matrix);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_CCW);
	glCullFace(GL_BACK);

	Resource::ChunkShader->Use();

	//texture
	glActiveTexture(GL_TEXTURE0);
	Resource::ChunkTexture->Bind();
	glActiveTexture(GL_TEXTURE1);
	Resource::SkyTexture->Bind();

	Resource::ChunkShader->PassInt(Resource::ChunkShader_sampler, 0);
	Resource::ChunkShader->PassInt(Resource::ChunkShader_skySampler, 1);
	Resource::ChunkShader->PassMat4(Resource::ChunkShader_matrix, vp_matrix);

	Resource::ChunkShader->PassFloat(Resource::ChunkShader_viewDistance, range);
	Resource::ChunkShader->PassFloat(Resource::ChunkShader_dayTime, wld.GetDayTime());
	Resource::ChunkShader->PassFloat(Resource::ChunkShader_dayLight, wld.GetDayLight());
	Resource::ChunkShader->PassVec3(Resource::ChunkShader_camera, position);
	Resource::ChunkShader->PassVec3(Resource::ChunkShader_selection, selection);

	for(const glm::ivec3 &pos : wld.GetOpaqueRenderSet())
	{
		ChunkPtr chk = wld.GetChunk(pos);
		if(!chk)
			continue;
		glm::vec3 center((glm::vec3)(pos * CHUNK_SIZE) + glm::vec3(CHUNK_SIZE/2));
		if (frustum.CubeInFrustum(center, CHUNK_SIZE/2) &&
			glm::distance((glm::vec3)pos, p_center) < (float)Setting::ChunkLoadRange + 1)
			chk->vertex_buffers_[0]->Render(GL_TRIANGLES);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(const glm::ivec3 &pos : wld.GetTransparentRenderVector())
	{
		ChunkPtr chk = wld.GetChunk(pos);
		if(!chk)
			continue;
		glm::vec3 center((glm::vec3)(pos * CHUNK_SIZE) + glm::vec3(CHUNK_SIZE/2));
		if (frustum.CubeInFrustum(center, CHUNK_SIZE/2) &&
			glm::distance((glm::vec3)pos, p_center) < (float)Setting::ChunkLoadRange + 1)
			chk->vertex_buffers_[1]->Render(GL_TRIANGLES);
	}
	glDisable(GL_BLEND);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void Renderer::RenderCrosshair(const glm::mat4 &vp_matrix)
{
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);
	Resource::LineShader->Use();
	Resource::LineShader->PassMat4(Resource::LineShader_matrix, vp_matrix);
	Resource::CrosshairObject->Render(GL_TRIANGLES);
	glDisable(GL_COLOR_LOGIC_OP);
}

void Renderer::RenderSky(const glm::mat3 &view, const glm::mat4 &projection, const glm::mat4 &sun_model_matrix, float day_time)
{
	glm::mat4 vp_matrix = projection * glm::mat4(view);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_CCW);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	//sky
	glCullFace(GL_FRONT);
	glActiveTexture(GL_TEXTURE0);
	Resource::SkyTexture->Bind();

	Resource::SkyShader->Use();
	Resource::SkyShader->PassMat4(Resource::SkyShader_matrix, vp_matrix);
	Resource::SkyShader->PassInt(Resource::SkyShader_sampler, 0);
	Resource::SkyShader->PassFloat(Resource::SkyShader_dayTime, day_time);

	Resource::SkyObject->Render(GL_TRIANGLES);

	//moon and sun
	glCullFace(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Resource::SunShader->Use();
	Resource::SunShader->PassMat4(Resource::SunShader_matrix, vp_matrix * sun_model_matrix);
	Resource::SunShader->PassInt(Resource::SunShader_sampler, 0);

	glActiveTexture(GL_TEXTURE0);
	if(day_time >= 0.2 && day_time <= 0.8)
	{
		Resource::SunTexture->Bind();
		Resource::SunObject->Render(GL_TRIANGLES);
	}

	if(day_time <= 0.3 || day_time >= 0.7)
	{
		Resource::MoonTexture->Bind();
		Resource::MoonObject->Render(GL_TRIANGLES);
	}

	glDisable(GL_BLEND);

	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void Renderer::RenderMenuBg()
{
	glActiveTexture(GL_TEXTURE0);
	Resource::SkyTexture->Bind();

	Resource::BgShader->Use();
	Resource::BgShader->PassInt(Resource::BgShader_sampler, 0);
	Resource::BgShader->PassMat4(Resource::BgShader_matrix, glm::mat4(1.0f));

	Resource::BgObject->Render(GL_TRIANGLES);
}

