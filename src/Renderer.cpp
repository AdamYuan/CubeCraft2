//
// Created by adamyuan on 1/10/18.
//
#include "Renderer.hpp"
#include "Resource.hpp"
#include "World.hpp"
#include <MyGL/Frustum.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Renderer::RenderWorld(const World &wld, const glm::mat4 &vpMatrix,
						   const glm::vec3 &position)
{
	static MyGL::Frustum frustum = {};
	static constexpr float range = CHUNK_SIZE*(CHUNK_LOADING_RANGE - 1);

	glm::vec3 p_center = (glm::vec3)(World::BlockPosToChunkPos(glm::floor(position)));

	frustum.CalculatePlanes(vpMatrix);

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
	Resource::ChunkShader->PassMat4(Resource::ChunkShader_matrix, vpMatrix);

	Resource::ChunkShader->PassFloat(Resource::ChunkShader_viewDistance, range);
	Resource::ChunkShader->PassFloat(Resource::ChunkShader_dayTime, wld.GetDayTime());
	Resource::ChunkShader->PassFloat(Resource::ChunkShader_dayLight, wld.GetDayLight());
	Resource::ChunkShader->PassVec3(Resource::ChunkShader_camera, position);

	for(const glm::ivec3 &pos : wld.RenderSet)
	{
		glm::vec3 center((glm::vec3)(pos * CHUNK_SIZE) + glm::vec3(CHUNK_SIZE/2));
		ChunkPtr chk = wld.GetChunk(pos);
		if (chk && frustum.CubeInFrustum(center, CHUNK_SIZE/2) &&
			glm::distance((glm::vec3)pos, p_center) < (float)CHUNK_LOADING_RANGE + 1)
			chk->VertexBuffer->Render(GL_TRIANGLES);
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void Renderer::RenderCrosshair(const glm::mat4 &vpMatrix)
{
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);
	Resource::LineShader->Use();
	Resource::LineShader->PassMat4(Resource::LineShader_matrix, vpMatrix);
	Resource::CrosshairObject->Render(GL_TRIANGLES);
	glDisable(GL_COLOR_LOGIC_OP);
}

void Renderer::RenderSelectionBox(const glm::mat4 &vpMatrix, const glm::ivec3 &position)
{
	glEnable(GL_DEPTH_TEST);
	Resource::LineShader->Use();
	Resource::LineShader->PassMat4(Resource::LineShader_matrix, vpMatrix * glm::translate(glm::mat4(1.0f), glm::vec3(position)));
	Resource::BoxObject->Render(GL_LINES);
	glDisable(GL_DEPTH_TEST);
}

void Renderer::RenderSky(const glm::mat3 &view, const glm::mat4 &projection, const glm::mat4 &sunModelMatrix, float dayTime)
{
	glm::mat4 vpMatrix = projection * glm::mat4(view);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_CCW);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	//sky
	glCullFace(GL_FRONT);
	glActiveTexture(GL_TEXTURE0);
	Resource::SkyTexture->Bind();

	Resource::SkyShader->Use();
	Resource::SkyShader->PassMat4(Resource::SkyShader_matrix, vpMatrix);
	Resource::SkyShader->PassInt(Resource::SkyShader_sampler, 0);
	Resource::SkyShader->PassFloat(Resource::SkyShader_dayTime, dayTime);

	Resource::SkyObject->Render(GL_TRIANGLES);

	//moon and sun
	glCullFace(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Resource::SunShader->Use();
	Resource::SunShader->PassMat4(Resource::SunShader_matrix, vpMatrix * sunModelMatrix);
	Resource::SunShader->PassInt(Resource::SunShader_sampler, 0);

	glActiveTexture(GL_TEXTURE0);
	if(dayTime >= 0.2 && dayTime <= 0.8)
	{
		Resource::SunTexture->Bind();
		Resource::SunObject->Render(GL_TRIANGLES);
	}

	if(dayTime <= 0.3 || dayTime >= 0.7)
	{
		Resource::MoonTexture->Bind();
		Resource::MoonObject->Render(GL_TRIANGLES);
	}

	glDisable(GL_BLEND);

	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

