#include "Camera.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace MyGL
{
	glm::mat4 Camera::GetViewMatrix()
	{
		glm::mat4 view;
		view = glm::rotate(view, glm::radians(-Pitch), glm::vec3(1.0f,0.0f,0.0f));
		view = glm::rotate(view, glm::radians(-Yaw), glm::vec3(0.0f,1.0f,0.0f));
		view = glm::translate(view, -Position);

		return view;
	}
	void Camera::MoveForward(const float &dist, const float &dir)
	{
		float rad = glm::radians(Yaw + dir);
		Position.x -= std::sin(rad) * dist;
		Position.z -= std::cos(rad) * dist;
	}
	void Camera::MoveUp(const float &dist)
	{
		Position.y += dist;
	}

	void Camera::ProcessMouseMovement(float xOffset, float yOffset, float sensitivity)
	{
		xOffset *= sensitivity;
		yOffset *= sensitivity;

		Yaw   += xOffset;
		Pitch += yOffset;

		if(Pitch > 90.0f)
			Pitch = 90;
		else if(Pitch < -90.0f)
			Pitch = -90;
		if(Yaw < 0.0f)
			Yaw += 360;
		else if(Yaw > 360.0f)
			Yaw -= 360;
	}
}
