//
// Created by adamyuan on 12/27/17.
//

//DO NOT CHANGE THIS ORDER
#include "Settings.hpp"
#include "World.hpp"

#include "Player.hpp"
#include <glm/gtx/string_cast.hpp>

#define MOUSE_SENSITIVITY 0.17f

Player::Player() : flying(true), BoundingBox({-0.25, -1.5, -0.25}, {0.25, 0.25, 0.25})
{

}

void Player::MouseControl(GLFWwindow *win, int width, int height)
{
	double x, y;
	glfwGetCursorPos(win, &x, &y);
	Cam.ProcessMouseMovement((float) (width / 2 - x), (float) (height / 2 - y),
							 MOUSE_SENSITIVITY);
	glfwSetCursorPos(win, width / 2, height / 2);
}

void Player::KeyControl(const World &wld, GLFWwindow *win, const MyGL::FrameRateManager &framerate)
{
	float dist = framerate.GetMovementDistance(1.0f);

	glm::vec3 oldPos = Cam.Position;

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
	glm::vec3 velocity = Cam.Position - oldPos;
	Cam.Position = oldPos;

	//collision test with separate axis
	for(int axis = 0; axis < 3; ++axis)
	{
		bool flag = true;
		float axisVelocity = velocity[axis];
		while(axisVelocity > 1.0f)
		{
			if(!HitTest(wld, Cam.Position, axis, 1.0f))
			{
				flag = false;
				break;
			}
			axisVelocity -= 1.0f;
		}
		if(flag)
			HitTest(wld, Cam.Position, axis, axisVelocity);
	}
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
	return World::BlockPosToChunkPos(glm::floor(Cam.Position));
}

bool Player::HitTest(const World &wld, glm::vec3 &pos, int axis, float velocity)
{
	pos[axis] += velocity;

	AABB now(BoundingBox.Min + pos, BoundingBox.Max + pos);

	glm::ivec3 Min = glm::floor(now.Min);
	glm::ivec3 Max = glm::floor(now.Max);

	int u, v;
	if(axis == 0)
		u = 1, v = 2;
	else if(axis == 1)
		u = 0, v = 2;
	else
		u = 0, v = 1;

	glm::ivec3 iter;
	iter[axis] = velocity > 0 ? Max[axis] : Min[axis];

	for(iter[u] = Min[u]; iter[u] <= Max[u]; ++iter[u])
		for(iter[v] = Min[v]; iter[v] <= Max[v]; ++iter[v])
		{
			AABB box = BlockMethods::GetBlockAABB(iter);
			if (now.Intersect(box) &&
				BlockMethods::HaveHitbox(wld.GetBlock(iter)))
			{
				pos[axis] -= velocity;
				return false;
			}
		}

	return true;
}


