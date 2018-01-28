#include <iostream>
#include <GL/glew.h>
#include "Application.hpp"
#include "Resource.hpp"
#include "UI.hpp"

Application::Application() : Width(720), Height(480), InGame(false)
{
	InitWindow();
}

void Application::InitWindow()
{
	glfwInit();

	//glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);

	Window = glfwCreateWindow(Width, Height, "CubeCraft2", nullptr, nullptr);

	if(Window == nullptr)
	{
		printf("ERROR WHEN CREATING WINDOW");
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(Window);

	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK)
	{
		printf("ERROR WHEN INITIALIZE GLEW");
		exit(EXIT_FAILURE);
	}

	UI::Init(Window);
	Resource::InitResources();
	Setting::InitSetting();
}

void Application::Run()
{
	gameMenu = std::make_unique<GameMenu>(Window);
	while(!glfwWindowShouldClose(Window))
	{
		if(InGame)
		{
			worldController->Update();

			if(worldController->IsQuit())
			{
				worldController.reset();
				gameMenu = std::make_unique<GameMenu>(Window);
				InGame = false;
			}
		}
		else
		{
			gameMenu->Update();
			if(gameMenu->EnterGame())
			{
				std::string worldName = gameMenu->GetWorldName();
				gameMenu.reset();
				worldController = std::make_unique<WorldController>(Window, worldName);
				InGame = true;
			}
			else if(gameMenu->IsQuit()) //close game
				glfwSetWindowShouldClose(Window, true);
		}

		glfwSwapBuffers(Window);
		glfwPollEvents();
	}
}

Application::~Application()
{
	UI::Shutdown();
	Setting::SaveSetting();
	glfwDestroyWindow(Window);
}

