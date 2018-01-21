#include <iostream>
#include <GL/glew.h>
#include "Application.hpp"
#include "Resource.hpp"
#include "WorldController.hpp"

Application::Application()
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
    glfwSetWindowUserPointer(Window, reinterpret_cast<void *>(this));

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
	Resource::InitResources();
}

void Application::Run()
{
	WorldController worldController(Window, "world");
	while(!glfwWindowShouldClose(Window))
	{
		worldController.Update();

		glfwSwapBuffers(Window);
		glfwPollEvents();
	}
}

