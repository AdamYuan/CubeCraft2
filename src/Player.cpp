//
// Created by adamyuan on 12/27/17.
//

//DO NOT CHANGE THIS ORDER
#include "World.hpp"

#include "Player.hpp"

#include <glm/gtc/matrix_transform.hpp>


#define MOUSE_SENSITIVITY 0.17f

Player::Player(World &wld) : Position(Cam.Position), flying(false),
							 BoundingBox({-0.25, -1.5, -0.25}, {0.25, 0.25, 0.25}),
							 wld(&wld), UsingBlock(1)
{

}

void Player::MouseControl(GLFWwindow *win, int width, int height)
{
	static constexpr double INTERVAL = 0.18;
	static double lastTime = glfwGetTime();
	static bool leftFirst = true, rightFirst = true;
	if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		rightFirst = true;
		if(leftFirst || glfwGetTime() - lastTime >= INTERVAL)
		{
			wld->SetBlock(Selection, Blocks::Air, true);
			lastTime = glfwGetTime();
			leftFirst = false;
		}
	}
	else if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		leftFirst = true;
		if((rightFirst || glfwGetTime() - lastTime >= INTERVAL) &&
				!GetBoundingBox().Intersect(BlockMethods::GetBlockAABB(NewBlockSelection)))
		{
			wld->SetBlock(NewBlockSelection, UsingBlock, true);
			lastTime = glfwGetTime();
			rightFirst = false;
		}
	}
	else
		leftFirst = rightFirst = true;

	double x, y;
	glfwGetCursorPos(win, &x, &y);
	Cam.ProcessMouseMovement((float) (width / 2 - x), (float) (height / 2 - y),
							 MOUSE_SENSITIVITY);
	glfwSetCursorPos(win, width / 2, height / 2);
}

void Player::KeyControl(GLFWwindow *win, const MyGL::FrameRateManager &framerate)
{
	glm::ivec3 chunkPos = GetChunkPosition();
	if(!wld->ChunkExist(chunkPos))
		return;
	if(!wld->GetChunk(chunkPos)->LoadedTerrain)
		return;

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

void Player::UpdatePhysics(const MyGL::FrameRateManager &framerate)
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

glm::ivec3 Player::GetChunkPosition() const
{
	return World::BlockPosToChunkPos(glm::floor(Cam.Position));
}

void Player::Control(bool focus, GLFWwindow *win, int width, int height, const MyGL::FrameRateManager &framerate,
					 const glm::mat4 &projection)
{
	//positions
	if(focus)
	{
		MouseControl(win, width, height);
		KeyControl(win, framerate);
	}
	UpdatePhysics(framerate);

	//update matrix
	ViewMatrix = Cam.GetViewMatrix();

	if(focus)
		UpdateSelection(width, height, projection);
}

void Player::UpdateSelection(int width, int height, const glm::mat4 &projection)
{
	float radius = 10.0f;

	glm::vec3 origin = Cam.Position;
	// From "A Fast Voxel Traversal Algorithm for Ray Tracing"
	// by John Amanatides and Andrew Woo, 1987
	// <http://www.cse.yorku.ca/~amana/research/grid.pdf>
	// <http://citeseer.ist.psu.edu/viewdoc/summary?doi=10.1.1.42.3443>
	// Extensions to the described algorithm:
	//   • Imposed a distance limit.
	//   • The face passed through to reach the current cube is provided to
	//     the callback.

	// The foundation of this algorithm is a parameterized representation of
	// the provided ray,
	//                    origin + t * direction,
	// except that t is not actually stored; rather, at any given point in the
	// traversal, we keep track of the *greater* t values which we would have
	// if we took a step sufficient to cross a cube boundary along that axis
	// (i.e. change the integer part of the coordinate) in the variables
	// tMaxX, tMaxY, and tMaxZ.

	// Cube containing origin point.
	glm::vec3 xyz = glm::floor(origin);
	// Break out direction vector.
	glm::vec3 direction = glm::unProject(glm::vec3(width / 2, height / 2, 1.0f),
										 glm::mat4(), projection * ViewMatrix,
										 glm::vec4(0.0f, 0.0f, width, height)) - origin;
	// Direction to increment x,y,z when stepping.
	glm::ivec3 step = glm::sign(direction);
	// See description above. The initial values depend on the fractional
	// part of the origin.
	glm::vec3 tMax = glm::vec3(intBound(origin.x, direction.x),
							   intBound(origin.y, direction.y),
							   intBound(origin.z, direction.z));
	// The change in t when taking a step (always positive).
	glm::vec3 tDelta = (glm::vec3)step / direction;
	// Buffer for reporting faces to the callback.
	glm::ivec3 face;

	// Rescale from units of 1 cube-edge to units of 'direction' so we can
	// compare with 't'.
	radius /= glm::sqrt(direction.x*direction.x + direction.y*direction.y + direction.z*direction.z);

	while (true)
	{
		// Invoke the callback, unless we are not *yet* within the bounds of the
		// world.
		if (BlockMethods::HaveHitbox(wld->GetBlock(xyz))) {
			Selection = xyz;
			NewBlockSelection = Selection + face;
			return;
		}

		// tMaxX stores the t-value at which we cross a cube boundary along the
		// X axis, and similarly for Y and Z. Therefore, choosing the least tMax
		// chooses the closest cube boundary. Only the first case of the four
		// has been commented in detail.
		if (tMax.x < tMax.y) {
			if (tMax.x < tMax.z) {
				if (tMax.x > radius) break;
				// Update which cube we are now in.
				xyz.x += step.x;
				// Adjust tMaxX to the next X-oriented boundary crossing.
				tMax.x += tDelta.x;
				// Record the normal vector of the cube face we entered.
				face[0] = -step.x;
				face[1] = 0;
				face[2] = 0;
			} else {
				if (tMax.z > radius) break;
				xyz.z += step.z;
				tMax.z += tDelta.z;
				face[0] = 0;
				face[1] = 0;
				face[2] = -step.z;
			}
		} else {
			if (tMax.y < tMax.z) {
				if (tMax.y > radius) break;
				xyz.y += step.y;
				tMax.y += tDelta.y;
				face[0] = 0;
				face[1] = -step.y;
				face[2] = 0;
			} else {
				// Identical to the second case, repeated for simplicity in
				// the conditionals.
				if (tMax.z > radius) break;
				xyz.z += step.z;
				tMax.z += tDelta.z;
				face[0] = 0;
				face[1] = 0;
				face[2] = -step.z;
			}
		}
	}

	NewBlockSelection = Selection = glm::ivec3(INT_MAX);
}
