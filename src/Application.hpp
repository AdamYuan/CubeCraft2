#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>

#include <MyGL/VertexObject.hpp>
#include <MyGL/Matrices.hpp>
#include <MyGL/FrameRate.hpp>


#include "Player.hpp"
#include "World.hpp"
#include "UI.hpp"

#include <GLFW/glfw3.h>

class Application
{
private:
	GLFWwindow *Window;//must be the first
	int Width = 720, Height = 480;

	void InitWindow();

public:
	Application();
	void Run();

};

#endif
