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

	glm::mat4 ViewMatrix;

	glm::ivec3 Selection, NewBlockSelection;
	inline float intBound(float s, float ds)
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

	uint8_t UsingBlock;

	//all the move function will return false if the movement is blocked

	explicit Player(World &wld);
	bool flying;

	void Control(bool focus, GLFWwindow *win, int width, int height, const MyGL::FrameRateManager &framerate,
					 const glm::mat4 &projection);


	bool Move(const glm::vec3 &velocity);
	bool MoveAxis(int axis, float velocity);

	glm::ivec3 GetChunkPosition() const;

	inline AABB GetBoundingBox() const
	{ return AABB(BoundingBox.Min + Cam.Position, BoundingBox.Max + Cam.Position); }
	inline glm::vec3 GetPosition() const
	{ return Cam.Position; }
	inline void SetPosition(const glm::vec3 &pos)
	{ Cam.Position = pos; }
	inline glm::mat4 GetViewMatrix()
	{ return ViewMatrix; }
	inline glm::ivec3 GetSelection(bool addDirVec)
	{ return addDirVec ? NewBlockSelection : Selection; }
};


#endif
