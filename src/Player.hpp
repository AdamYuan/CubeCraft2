//
// Created by adamyuan on 12/27/17.
//

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <mygl2/utils/camera.hpp>
#include <mygl2/utils/framerate.hpp>
#include <GLFW/glfw3.h>

#include "Util.hpp"

class World;

class Player
{
private:
	const AABB kBoundingBox;

	glm::mat4 view_matrix_;
	glm::ivec3 selection_, new_block_selection_;

	bool HitTest(glm::vec3 &pos, int axis, float velocity);
	inline float IntBound(float s, float ds)
	{
		bool sIsInteger = glm::round(s) == s;
		if (ds < 0 && sIsInteger)
			return 0;
		return (ds > 0 ? (s == 0.0f ? 1.0f : glm::ceil(s)) - s : s - glm::floor(s)) / glm::abs(ds);
	}

	World *world_;
	bool jumping_ = false;

	void KeyControl(GLFWwindow *win, const mygl2::Framerate &framerate);
	void MouseControl(GLFWwindow *win, int width, int height);
	void UpdateSelection(int width, int height, const glm::mat4 &projection);
	void UpdatePhysics(const mygl2::Framerate &framerate);

public:
	glm::vec3 &position_;
	uint8_t using_block_;
	bool flying_;
	mygl2::Camera camera_;

	//all the move functions will return false if the movement is blocked

	explicit Player(World &wld);

	void Control(bool focus, GLFWwindow *win, int width, int height, const mygl2::Framerate &framerate,
					 const glm::mat4 &projection);

	bool Move(const glm::vec3 &velocity);
	bool MoveAxis(int axis, float velocity);

	glm::ivec3 GetChunkPosition() const;

	inline AABB GetBoundingBox() const
	{ return {kBoundingBox.min_ + camera_.GetPosition(), kBoundingBox.max_ + camera_.GetPosition()}; }
	inline glm::mat4 GetViewMatrix()
	{ return view_matrix_; }
	inline glm::ivec3 GetSelection(bool add_direction_vector)
	{ return add_direction_vector ? new_block_selection_ : selection_; }
};


#endif
