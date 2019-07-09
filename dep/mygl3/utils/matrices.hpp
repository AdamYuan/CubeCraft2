//
// Created by adamyuan on 4/15/18.
//

#ifndef MYGL2_MATRICES_HPP
#define MYGL2_MATRICES_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace mygl3
{
	class Matrices
	{
	private:
		glm::mat4 projection_, ortho_, ortho_center_;
	public:
		const glm::mat4 &GetProjection() const { return projection_; }
		const glm::mat4 &GetOrthoCenter() const { return ortho_center_; }
		const glm::mat4 &GetOrtho() const { return ortho_; }
		void UpdateProjection(int width, int height, float fovy) //degrees
		{
			projection_ = glm::perspective(glm::radians(fovy), (float)width / (float)height, 0.01f, 1000.0f);
		}
		void UpdateOrtho(int width, int height)
		{
			ortho_ = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
			ortho_center_ = glm::ortho((float)-width/2.0f, (float)width/2.0f, (float)height/2.0f, (float)-height/2.0f);
		}
	};
}

#endif //MYGL2_MATRICES_HPP
