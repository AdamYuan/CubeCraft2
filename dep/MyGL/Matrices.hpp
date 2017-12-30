#pragma once
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace MyGL
{
	class Matrices
	{
	public:
		glm::mat4 Projection3d, Matrix2d, Matrix2dCenter;
		void UpdateMatrices(int width, int height, float fovy = 45.0f);
	};
}
