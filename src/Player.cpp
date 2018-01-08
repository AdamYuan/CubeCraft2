//
// Created by adamyuan on 12/27/17.
//

//DO NOT CHANGE THIS ORDER
#include "Settings.hpp"
#include "World.hpp"

#include "Player.hpp"

#define MOUSE_SENSITIVITY 0.17f

Player::Player(const World &wld) : flying(false),
								   BoundingBox({-0.25, -1.25, -0.25}, {0.25, 0.25, 0.25}),
								   wld(&wld)
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

void Player::KeyControl(GLFWwindow *win, const MyGL::FrameRateManager &framerate)
{
	float dist = framerate.GetMovementDistance(WALK_SPEED);

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
		jumping = false;
		if(glfwGetKey(win, GLFW_KEY_SPACE))
			Cam.MoveUp(dist);
		if(glfwGetKey(win, GLFW_KEY_LEFT_SHIFT))
			Cam.MoveUp(-dist);
	}
	//jumping
	if(!flying && glfwGetKey(win, GLFW_KEY_SPACE))
	{
		glm::vec3 vec = oldPos;
		//check player is on a solid block
		if(!HitTest(vec, 1, -0.001f))
			jumping = true;
	}

	glm::vec3 velocity = Cam.Position - oldPos;
	if(!flying && velocity != glm::vec3(0.0f))
		velocity = glm::normalize(velocity) * dist;
	Cam.Position = oldPos;

	Move(velocity);
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

bool Player::HitTest(glm::vec3 &pos, int axis, float velocity)
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
	bool positive = velocity > 0;
	iter[axis] = positive ? Max[axis] : Min[axis];

	for(iter[u] = Min[u]; iter[u] <= Max[u]; ++iter[u])
		for(iter[v] = Min[v]; iter[v] <= Max[v]; ++iter[v])
		{
			AABB box = BlockMethods::GetBlockAABB(iter);
			if (now.Intersect(box) &&
				BlockMethods::HaveHitbox(wld->GetBlock(iter)))
			{
				//set the right position
				pos[axis] = (float)(iter[axis] + !positive) -
						(positive ? BoundingBox.Max[axis] : BoundingBox.Min[axis]);
				return false;
			}
		}

	return true;
}

bool Player::Move(const glm::vec3 &velocity)
{
	//move with separate axis
	bool returnV = true;

	for(int axis = 0; axis < 3; ++axis)
		returnV &= MoveAxis(axis, velocity[axis]);

	return returnV;
}

#define HIT_TEST_STEP 0.875f
#define MAX_MOVE_DIST 10.0f
bool Player::MoveAxis(int axis, float velocity)
{
	if(velocity == 0.0f)
		return true;

	velocity = glm::min(velocity, MAX_MOVE_DIST);

	short sign = 1;
	if(velocity < 0)
	{
		sign = -1;
		velocity = -velocity;
	}

	//prevent collision missing when moving fast
	while(velocity > HIT_TEST_STEP)
	{
		if(!HitTest(Cam.Position, axis, HIT_TEST_STEP * sign))
			return false;
		velocity -= HIT_TEST_STEP;
	}
	return HitTest(Cam.Position, axis, velocity * sign);
}

void Player::PhysicsUpdate(const MyGL::FrameRateManager &framerate)
{
	static double lastTime = glfwGetTime();
	if(flying)
	{
		lastTime = glfwGetTime();
		return;
	}

	if(jumping)
		if(!MoveAxis(1, framerate.GetMovementDistance(JUMP_DIST)))
			jumping = false;

	static bool firstFall = false;

	float dis = GRAVITY * (float)(glfwGetTime() - lastTime);
	float y = Cam.Position.y;
	if (!MoveAxis(1, -framerate.GetMovementDistance(dis)))
	{
		lastTime = glfwGetTime();
		jumping = false;
		firstFall = true;
	}
	else if(firstFall)
	{
		//not fall at first
		firstFall = false;
		Cam.Position.y = y;
	}
}

