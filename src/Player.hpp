//
// Created by adamyuan on 12/27/17.
//

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <MyGL/Camera.hpp>
#include <MyGL/FrameRate.hpp>
#include <GLFW/glfw3.h>

class Player
{
private:
	MyGL::Camera Cam;
public:

	Player();
	bool flying;

	void Control(GLFWwindow *win, int width, int height, const MyGL::FrameRateManager &framerate);

	glm::ivec3 GetChunkPosition() const;
	glm::vec3 GetPosition() const;
	void SetPosition(const glm::vec3 &pos);

	glm::mat4 GetViewMatrix();
};


#endif
