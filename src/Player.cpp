//
// Created by adamyuan on 12/27/17.
//

//DO NOT CHANGE THIS ORDER
#include "World.hpp"

#include "Player.hpp"

#include <glm/gtc/matrix_transform.hpp>


#define MOUSE_SENSITIVITY 0.17f

Player::Player(World &wld) : position_(camera_.Position), flying_(false),
							 kBoundingBox({-0.25, -1.375, -0.25}, {0.25, 0.25, 0.25}),
							 world_(&wld), using_block_(1)
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
			world_->SetBlock(selection_, Blocks::Air, true);
			lastTime = glfwGetTime();
			leftFirst = false;
		}
	}
	else if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		leftFirst = true;
		if((rightFirst || glfwGetTime() - lastTime >= INTERVAL) &&
				!GetBoundingBox().Intersect(BlockMethods::GetBlockAABB(new_block_selection_)))
		{
			world_->SetBlock(new_block_selection_, using_block_, true);
			lastTime = glfwGetTime();
			rightFirst = false;
		}
	}
	else
		leftFirst = rightFirst = true;

	double x, y;
	glfwGetCursorPos(win, &x, &y);
	camera_.ProcessMouseMovement((float) (width / 2 - x), (float) (height / 2 - y),
							 MOUSE_SENSITIVITY);
	glfwSetCursorPos(win, width / 2, height / 2);
}

void Player::KeyControl(GLFWwindow *win, const MyGL::FrameRateManager &framerate)
{

	float dist = framerate.GetMovementDistance(WALK_SPEED);

	glm::vec3 oldPos = camera_.Position;

	if(glfwGetKey(win, GLFW_KEY_W))
		camera_.MoveForward(dist, 0);
	if(glfwGetKey(win, GLFW_KEY_S))
		camera_.MoveForward(dist, 180);
	if(glfwGetKey(win, GLFW_KEY_A))
		camera_.MoveForward(dist, 90);
	if(glfwGetKey(win, GLFW_KEY_D))
		camera_.MoveForward(dist, -90);

	if(flying_)
	{
		jumping_ = false;
		if(glfwGetKey(win, GLFW_KEY_SPACE))
			camera_.MoveUp(dist);
		if(glfwGetKey(win, GLFW_KEY_LEFT_SHIFT))
			camera_.MoveUp(-dist);
	}
	//jumping
	if(!flying_ && glfwGetKey(win, GLFW_KEY_SPACE))
	{
		glm::vec3 vec = oldPos;
		//check player is on a solid block
		if(!HitTest(vec, 1, -0.001f))
			jumping_ = true;
	}

	glm::vec3 velocity = camera_.Position - oldPos;
	if(!flying_ && velocity != glm::vec3(0.0f))
		velocity = glm::normalize(velocity) * dist;
	camera_.Position = oldPos;

	Move(velocity);
}


bool Player::HitTest(glm::vec3 &pos, int axis, float velocity)
{
	pos[axis] += velocity;

	AABB now(kBoundingBox.min_ + pos, kBoundingBox.max_ + pos);

	glm::ivec3 pos_min = glm::floor(now.min_);
	glm::ivec3 pos_max = glm::floor(now.max_);

	int u = (axis + 1) % 3, v = (axis + 2) % 3;

	glm::ivec3 iter;
	bool positive = velocity > 0;
	iter[axis] = positive ? pos_max[axis] : pos_min[axis];

	for(iter[u] = pos_min[u]; iter[u] <= pos_max[u]; ++iter[u])
		for(iter[v] = pos_min[v]; iter[v] <= pos_max[v]; ++iter[v])
		{
			AABB box = BlockMethods::GetBlockAABB(iter);
			if (now.Intersect(box) &&
				BlockMethods::HaveHitbox(world_->GetBlock(iter)))
			{
				//set the right position
				pos[axis] = (float)(iter[axis] + !positive) -
							(positive ? kBoundingBox.max_[axis] : kBoundingBox.min_[axis]);
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
		if(!HitTest(camera_.Position, axis, HIT_TEST_STEP * sign))
			return false;
		velocity -= HIT_TEST_STEP;
	}
	return HitTest(camera_.Position, axis, velocity * sign);
}

void Player::UpdatePhysics(const MyGL::FrameRateManager &framerate)
{
	static double lastTime = glfwGetTime();
	if(flying_)
	{
		lastTime = glfwGetTime();
		return;
	}

	if(jumping_)
		if(!MoveAxis(1, framerate.GetMovementDistance(JUMP_DIST)))
			jumping_ = false;

	static bool firstFall = false;

	float dis = GRAVITY * (float)(glfwGetTime() - lastTime);
	float y = camera_.Position.y;
	if (!MoveAxis(1, -framerate.GetMovementDistance(dis)))
	{
		lastTime = glfwGetTime();
		jumping_ = false;
		firstFall = true;
	}
	else if(firstFall)
	{
		//not fall at first
		firstFall = false;
		camera_.Position.y = y;
	}
}

glm::ivec3 Player::GetChunkPosition() const
{
	return World::BlockPosToChunkPos(glm::floor(camera_.Position));
}

void Player::Control(bool focus, GLFWwindow *win, int width, int height, const MyGL::FrameRateManager &framerate,
					 const glm::mat4 &projection)
{
	glm::ivec3 chunkPos = GetChunkPosition();
	ChunkPtr chk = world_->GetChunk(chunkPos);
	bool flag = (chk && chk->loaded_terrain_) || (chunkPos.y < 0 || chunkPos.y >= WORLD_HEIGHT);
	if(flag)
	{
		//positions
		if(focus)
		{
			MouseControl(win, width, height);
			KeyControl(win, framerate);
		}
		UpdatePhysics(framerate);

	}
	//update matrix
	view_matrix_ = camera_.GetViewMatrix();

	if(focus)
		UpdateSelection(width, height, projection);
}

void Player::UpdateSelection(int width, int height, const glm::mat4 &projection)
{
	float radius = 10.0f;

	glm::vec3 origin = camera_.Position;
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
	glm::vec3 direction = camera_.GetLookDirection();
	// Direction to increment x,y,z when stepping.
	glm::ivec3 step = glm::sign(direction);
	// See description above. The initial values depend on the fractional
	// part of the origin.
	glm::vec3 t_max = glm::vec3(IntBound(origin.x, direction.x),
								IntBound(origin.y, direction.y),
								IntBound(origin.z, direction.z));
	// The change in t when taking a step (always positive).
	glm::vec3 t_delta = (glm::vec3)step / direction;
	// Buffer for reporting faces to the callback.
	glm::ivec3 face;

	// Rescale from units of 1 cube-edge to units of 'direction' so we can
	// compare with 't'.
	radius /= glm::sqrt(direction.x*direction.x + direction.y*direction.y + direction.z*direction.z);

	while (true)
	{
		// Invoke the callback, unless we are not *yet* within the bounds of the
		// world.
		if (BlockMethods::HaveHitbox(world_->GetBlock(xyz))) {
			selection_ = xyz;
			new_block_selection_ = selection_ + face;
			return;
		}

		// tMaxX stores the t-value at which we cross a cube boundary along the
		// X axis, and similarly for Y and Z. Therefore, choosing the least tMax
		// chooses the closest cube boundary. Only the first case of the four
		// has been commented in detail.
		if (t_max.x < t_max.y) {
			if (t_max.x < t_max.z) {
				if (t_max.x > radius) break;
				// Update which cube we are now in.
				xyz.x += step.x;
				// Adjust tMaxX to the next X-oriented boundary crossing.
				t_max.x += t_delta.x;
				// Record the normal vector of the cube face we entered.
				face[0] = -step.x;
				face[1] = 0;
				face[2] = 0;
			} else {
				if (t_max.z > radius) break;
				xyz.z += step.z;
				t_max.z += t_delta.z;
				face[0] = 0;
				face[1] = 0;
				face[2] = -step.z;
			}
		} else {
			if (t_max.y < t_max.z) {
				if (t_max.y > radius) break;
				xyz.y += step.y;
				t_max.y += t_delta.y;
				face[0] = 0;
				face[1] = -step.y;
				face[2] = 0;
			} else {
				// Identical to the second case, repeated for simplicity in
				// the conditionals.
				if (t_max.z > radius) break;
				xyz.z += step.z;
				t_max.z += t_delta.z;
				face[0] = 0;
				face[1] = 0;
				face[2] = -step.z;
			}
		}
	}

	new_block_selection_ = selection_ = glm::ivec3(INT_MAX);
}
