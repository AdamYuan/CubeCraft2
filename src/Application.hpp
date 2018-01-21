#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>

#include <MyGL/VertexObject.hpp>
#include <MyGL/Matrices.hpp>
#include <MyGL/FrameRate.hpp>


#include "Player.hpp"
#include "World.hpp"
#include "WorldController.hpp"
#include "GameMenu.hpp"

#include <GLFW/glfw3.h>

class Application
{
private:
	GLFWwindow *Window;//must be the first
	int Width, Height;
	bool InGame;

	std::unique_ptr<WorldController> worldController;
	std::unique_ptr<GameMenu> gameMenu;

	void InitWindow();

public:
	Application();
	~Application();
	void Run();
};

#endif
