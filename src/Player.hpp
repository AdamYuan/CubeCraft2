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
	bool HitTest(glm::vec3 &pos, int axis, float velocity);

	World const *wld;

	bool jumping = false;

public:

	//all the move function will return false if the movement is blocked

	explicit Player(const World &wld);
	bool flying;

	void KeyControl(GLFWwindow *win, const MyGL::FrameRateManager &framerate);
	void MouseControl(GLFWwindow *win, int width, int height);

	void PhysicsUpdate(const MyGL::FrameRateManager &framerate);

	bool Move(const glm::vec3 &velocity);
	bool MoveAxis(int axis, float velocity);

	glm::ivec3 GetChunkPosition() const;
	glm::vec3 GetPosition() const;
	void SetPosition(const glm::vec3 &pos);

	glm::mat4 GetViewMatrix();
};


#endif
