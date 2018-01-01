//
// Created by adamyuan on 12/27/17.
//

//DO NOT CHANGE THIS ORDER
#include "Settings.hpp"
#include "World.hpp"

#include "Player.hpp"
#include <glm/gtx/string_cast.hpp>

#define MOUSE_SENSITIVITY 0.17f

Player::Player() : flying(true), BoundingBox({-0.3, -1.4, -0.3}, {0.3, 0.2, 0.3})
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
	glm::vec3 velocity1 = glm::normalize(velocity) * 0.5f;

	Cam.Position = oldPos;
	while(glm::length(velocity) > 0.5f) {
		Cam.Position = HitTest(wld, Cam.Position, velocity1);
		velocity -= velocity1;
	}
	Cam.Position = HitTest(wld, Cam.Position, velocity);
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

glm::vec3 Player::HitTest(const World &wld, const glm::vec3 &origin, const glm::vec3 &velocity)
{
	AABB old(BoundingBox.Min + origin, BoundingBox.Max + origin);
	AABB now(old.Min + velocity, old.Max + velocity);

	glm::vec3 newPos = origin + velocity;

	glm::ivec3 Min = glm::floor(newPos + BoundingBox.Min);
	glm::ivec3 Max = glm::floor(newPos + BoundingBox.Max);

	std::vector<std::pair<int, int>> faces;

	if(velocity.y < 0)
		faces.emplace_back(1, 1);
	else if(velocity.y > 0)
		faces.emplace_back(1, -1);
	if(velocity.z < 0)
		faces.emplace_back(2, 1);
	else if(velocity.z > 0)
		faces.emplace_back(2, -1);
	if(velocity.x < 0)
		faces.emplace_back(0, 1);
	else if(velocity.x > 0)
		faces.emplace_back(0, -1);

	glm::ivec3 iter;
	for(iter.x = Min.x; !faces.empty() && iter.x <= Max.x; ++iter.x)
		for(iter.y = Min.y; !faces.empty() && iter.y <= Max.y; ++iter.y)
			for (iter.z = Min.z; !faces.empty() && iter.z <= Max.z; ++iter.z)
			{
				if(iter.x != Min.x && iter.x != Max.x && iter.y != Min.y && iter.y != Max.y
						&& iter.z != Min.z && iter.z != Max.z)
					continue;

				AABB box = BlockMethods::GetBlockAABB(iter);
				if (now.Intersect(box) &&
					BlockMethods::HaveHitbox(wld.GetBlock(iter)))
					for(auto i = faces.begin(); i != faces.end(); ++i)
					{
						//TODO: DEBUG
						AABB _now = old;
						_now.Min[i->first] += velocity[i->first];
						_now.Max[i->first] += velocity[i->first];

						if (_now.Intersect(BlockMethods::GetBlockAABB(iter))) {

							newPos[i->first] = (float)iter[i->first] + (float)i->second -
									(i->second > 0 ?
									 BoundingBox.Min[i->first] : BoundingBox.Max[i->first] - 1);

							faces.erase(i);
							break;
						}
					}
			}
	return newPos;
}


