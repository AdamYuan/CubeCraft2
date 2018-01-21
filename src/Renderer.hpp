//
// Created by adamyuan on 1/10/18.
//

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glm/glm.hpp>

class World;

namespace Renderer
{
	extern void RenderWorld(const World &wld, const glm::mat4 &vpMatrix, const glm::vec3 &position,
								const glm::ivec3 &selection);
	extern void RenderCrosshair(const glm::mat4 &vpMatrix);
	extern void
	RenderSky(const glm::mat3 &view, const glm::mat4 &projection, const glm::mat4 &sunModelMatrix, float dayTime);

	extern void RenderMenuBg();
};


#endif //RENDERER_HPP
