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

	Resource::ChunkShader->Use();

	//texture
	glActiveTexture(GL_TEXTURE0);
	Resource::ChunkTexture->Bind();
	glActiveTexture(GL_TEXTURE1);
	Resource::SkyTexture->Bind();

	Resource::ChunkShader->PassInt(Resource::UNIF_SAMPLER, 0);
	Resource::ChunkShader->PassInt("skySampler", 1);
	Resource::ChunkShader->PassMat4(Resource::UNIF_MATRIX, vpMatrix);

	Resource::ChunkShader->PassFloat("viewDistance", range);
	Resource::ChunkShader->PassFloat("Time", wld.GetTime());
	Resource::ChunkShader->PassFloat("dayLight", wld.GetDayLight());
	Resource::ChunkShader->PassVec3("camera", position);

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
	Resource::LineShader->PassMat4(Resource::UNIF_MATRIX, vpMatrix);
	Resource::CrosshairObject->Render(GL_TRIANGLES);
	glDisable(GL_COLOR_LOGIC_OP);
}

void Renderer::RenderSelectionBox(const glm::mat4 &vpMatrix, const glm::ivec3 &position)
{
	glEnable(GL_DEPTH_TEST);
	Resource::LineShader->Use();
	Resource::LineShader->PassMat4(Resource::UNIF_MATRIX, vpMatrix * glm::translate(glm::mat4(1.0f), glm::vec3(position)));
	Resource::BoxObject->Render(GL_LINES);
	glDisable(GL_DEPTH_TEST);
}

void Renderer::RenderSky(const glm::mat3 &view, const glm::mat4 &projection, float Time)
{
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glActiveTexture(GL_TEXTURE0);
	Resource::SkyTexture->Bind();

	Resource::SkyShader->Use();
	Resource::SkyShader->PassMat4(Resource::UNIF_MATRIX, projection * glm::mat4(view));
	Resource::SkyShader->PassInt(Resource::UNIF_SAMPLER, 0);
	Resource::SkyShader->PassFloat("Time", Time);

	Resource::SkyObject->Render(GL_TRIANGLES);

	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
}
