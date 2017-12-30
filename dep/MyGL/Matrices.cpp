#include "Matrices.hpp"
namespace MyGL
{
	void Matrices::UpdateMatrices(int width, int height, float fovy)
	{
		Projection3d = glm::perspective(glm::radians(fovy), (float)width/(float)height, 0.01f, 1000.0f);
		Matrix2d = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
		Matrix2dCenter = glm::ortho((float)-width/2.0f, (float)width/2.0f, (float)height/2.0f, (float)-height/2.0f);
	}
}
