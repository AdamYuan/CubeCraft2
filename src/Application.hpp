#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>

#include <mygl2/vertexbuffer.hpp>
#include <mygl2/utils/framerate.hpp>
#include <mygl2/utils/frustum.hpp>


#include "Player.hpp"
#include "World.hpp"
#include "WorldController.hpp"
#include "GameMenu.hpp"

#include <GLFW/glfw3.h>

class Application
{
private:
	GLFWwindow *window_;//must be the first
	int width_, height_;
	bool in_game_;

	std::unique_ptr<WorldController> world_controller_;
	std::unique_ptr<GameMenu> game_menu_;

	void InitWindow();

public:
	Application();
	~Application();
	void Run();
};

#endif
