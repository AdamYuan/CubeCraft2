#pragma once
#include <iostream>
#include <glm/glm.hpp>

namespace MyGL
{
	class Camera
	{
	public:
		float Yaw = 0.0f, Pitch = 0.0f;
		glm::vec3 Position;
		glm::mat4 GetViewMatrix() const;
		glm::vec3 GetLookDirection() const;
		void ProcessMouseMovement(float xOffset, float yOffset, float sensitivity);
		void MoveForward(const float &dist, const float &dir);
		void MoveUp(const float &dist);
	};
}
