//
// Created by adamyuan on 12/27/17.
//

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <MyGL/Camera.hpp>
#include <MyGL/FrameRate.hpp>
#include <GLFW/glfw3.h>

#include "Util.hpp"

class World;

class Player
{
private:
	const AABB BoundingBox;
	MyGL::Camera Cam;
	bool HitTest(const World &wld, glm::vec3 &pos, int axis, float velocity);

public:

	Player();
	bool flying;

	void KeyControl(const World &wld, GLFWwindow *win, const MyGL::FrameRateManager &framerate);
	void MouseControl(GLFWwindow *win, int width, int height);

	glm::ivec3 GetChunkPosition() const;
	glm::vec3 GetPosition() const;
	void SetPosition(const glm::vec3 &pos);

	glm::mat4 GetViewMatrix();
};


#endif
