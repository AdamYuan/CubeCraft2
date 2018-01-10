//
// Created by adamyuan on 1/10/18.
//

#include "Renderer.hpp"
#include "Resource.hpp"
#include <MyGL/Frustum.hpp>

#include "World.hpp"
void Renderer::RenderWorld(const World &wld, const glm::mat4 &projection, const glm::mat4 &view,
						   const glm::vec3 &position)
{
	static MyGL::Frustum frustum = {};
	static constexpr float range = CHUNK_SIZE*(CHUNK_LOADING_RANGE - 1);

	glm::vec3 p_center = (glm::vec3)(World::BlockPosToChunkPos(glm::floor(position)));

	glm::mat4 matrix = projection * view;
	frustum.CalculatePlanes(matrix);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	Resource::ChunkShader->Use();

	//texture
	glActiveTexture(GL_TEXTURE0);
	Resource::ChunkTexture->Bind();

	Resource::ChunkShader->PassInt(Resource::UNIF_SAMPLER, 0);
	Resource::ChunkShader->PassMat4(Resource::UNIF_MATRIX, matrix);

	Resource::ChunkShader->PassFloat("viewDistance", range);
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

void Renderer::RenderCrosshair(const glm::mat4 &matrix)
{
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);
	Resource::LineShader->Use();
	Resource::LineShader->PassMat4(Resource::UNIF_MATRIX, matrix);
	Resource::CrosshairObject->Render(GL_TRIANGLES);
	glDisable(GL_COLOR_LOGIC_OP);
}