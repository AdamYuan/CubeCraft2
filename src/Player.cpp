//
// Created by adamyuan on 12/27/17.
//

#include "Player.hpp"
#include "Settings.hpp"
#include <glm/gtx/string_cast.hpp>

#define MOUSE_SENSITIVITY 0.17f

Player::Player() : flying(true)
{

}

void Player::Control(GLFWwindow *win, int width, int height, const MyGL::FrameRateManager &framerate)
{
	float dist = framerate.GetMovementDistance(3.0f);

	if(glfwGetKey(win, GLFW_KEY_W))
		Cam.MoveForward(dist, 0);
	if(glfwGetKey(win, GLFW_KEY_S))
		Cam.MoveForward(dist, 180);
	if(glfwGetKey(win, GLFW_KEY_A))
		Cam.MoveForward(dist, 90);
	if(glfwGetKey(win, GLFW_KEY_D))
		Cam.MoveForward(dist, -90);

	if(flying)
	{
		if(glfwGetKey(win, GLFW_KEY_SPACE))
			Cam.MoveUp(dist);
		if(glfwGetKey(win, GLFW_KEY_LEFT_SHIFT))
			Cam.MoveUp(-dist);
	}

	double x, y;
	glfwGetCursorPos(win, &x, &y);
	Cam.ProcessMouseMovement((float) (width / 2 - x), (float) (height / 2 - y),
								MOUSE_SENSITIVITY);
	glfwSetCursorPos(win, width / 2, height / 2);
}

glm::vec3 Player::GetPosition() const
{
	return Cam.Position;
}

void Player::SetPosition(const glm::vec3 &pos)
{
	Cam.Position = pos;
}

glm::mat4 Player::GetViewMatrix()
{
	return Cam.GetViewMatrix();
}

glm::ivec3 Player::GetChunkPosition() const
{
	return glm::ivec3(Cam.Position.x / CHUNK_SIZE - (Cam.Position.x < 0),
					  Cam.Position.y / CHUNK_SIZE - (Cam.Position.y < 0),
					  Cam.Position.z / CHUNK_SIZE - (Cam.Position.z < 0));
}

