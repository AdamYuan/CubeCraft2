//
// Created by adamyuan on 1/10/18.
//

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glm/glm.hpp>

class World;

namespace Renderer
{
	extern void RenderWorld(const World &wld, const glm::mat4 &vpMatrix, const glm::vec3 &position);
	extern void RenderCrosshair(const glm::mat4 &vpMatrix);
	extern void RenderSelectionBox(const glm::mat4 &vpMatrix, const glm::ivec3 &position);
};


#endif //RENDERER_HPP
