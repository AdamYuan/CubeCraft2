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

	World *wld;
	bool jumping = false;

	void KeyControl(GLFWwindow *win, const MyGL::FrameRateManager &framerate);
	void MouseControl(GLFWwindow *win, int width, int height);
	void UpdateSelection(int width, int height, const glm::mat4 &projection);
	void UpdatePhysics(const MyGL::FrameRateManager &framerate);

public:

	glm::vec3 &position_;

	uint8_t using_block_;

	//all the move function will return false if the movement is blocked

	explicit Player(World &wld);
	bool flying_;

	void Control(bool focus, GLFWwindow *win, int width, int height, const MyGL::FrameRateManager &framerate,
					 const glm::mat4 &projection);


	bool Move(const glm::vec3 &velocity);
	bool MoveAxis(int axis, float velocity);

	glm::ivec3 GetChunkPosition() const;

	inline AABB GetBoundingBox() const
	{ return {kBoundingBox.min_ + Cam.Position, kBoundingBox.max_ + Cam.Position}; }
	inline glm::mat4 GetViewMatrix()
	{ return view_matrix_; }
	inline glm::ivec3 GetSelection(bool add_direction_vector)
	{ return add_direction_vector ? new_block_selection_ : selection_; }

	MyGL::Camera Cam;
};


#endif
