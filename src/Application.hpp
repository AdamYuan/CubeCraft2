#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>

#include <MyGL/VertexObject.hpp>
#include <MyGL/Matrices.hpp>
#include <MyGL/FrameRate.hpp>


#include "Player.hpp"
#include "World.hpp"

#include <GLFW/glfw3.h>

class Application
{
private:
	GLFWwindow *Window;
	int Width = 720, Height = 480;

	World world;

	Player GamePlayer;
	glm::mat4 ViewMatrix;

	MyGL::Matrices Matrices;
	MyGL::FrameRateManager FramerateManager;

public:
	Application();
	void Run();
	void InitWindow();
	void Render();
	void LogicProcess();
};

#endif
