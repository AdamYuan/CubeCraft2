#include <iostream>
#include <GL/glew.h>
#include "Chunk.hpp"
#include "Application.hpp"
#include "Resource.hpp"

static bool control = true;
void focusCallback(GLFWwindow*, int focused)
{
	control = focused != 0;
}
static int sWidth, sHeight;
static bool resized = false;
void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
	sWidth = width;
	sHeight = height;
	resized = true;
}
void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE)
		control = false;
}

Application::Application() : world()
{
	InitWindow();
}

void Application::InitWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	Window = glfwCreateWindow(Width, Height, "CubeCraft2", nullptr, nullptr);
	if(Window == nullptr)
		throw std::runtime_error("ERROR WHEN CREATING WINDOW");

	glfwMakeContextCurrent(Window);

	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK)
		throw std::runtime_error("ERROR WHEN INITIALIZE GLEW");
	glViewport(0, 0, Width, Height);
	Matrices.UpdateMatrices(Width, Height);

	glfwSetWindowFocusCallback(Window, focusCallback);
	glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);
	glfwSetKeyCallback(Window, keyCallback);

	Resource::InitResources();

	glCullFace(GL_CCW);
}

void Application::Run()
{
	while(!glfwWindowShouldClose(Window))
	{
		//NEVER CHANGE THIS ORDER!!!!!!

		//logic process
		LogicProcess();

		//render
		Render();
		glfwSwapBuffers(Window);


		glfwPollEvents();
	}
}

void Application::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

	world.Render(Matrices.Projection3d, ViewMatrix, GamePlayer.GetPosition());
}

void Application::LogicProcess()
{
	//update frame rate info
	FramerateManager.UpdateFrameRateInfo();


	static int lastTime = 0;
	if(glfwGetTime() > lastTime + 1)
	{
		lastTime = static_cast<int>(glfwGetTime());
		std::cout << "fps: " << FramerateManager.GetFps() << std::endl;
	}

	//process size event
	if(resized)
	{
		Width = sWidth;
		Height = sHeight;
		resized = false;
		glViewport(0, 0, Width, Height);
		Matrices.UpdateMatrices(Width, Height);
	}
	//get control when mouse press
	if(glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		glfwSetCursorPos(Window, Width / 2, Height / 2);
		control = true;
	}

	if(control) {
		GamePlayer.Control(Window, Width, Height, FramerateManager);
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
	else
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	ViewMatrix = GamePlayer.GetViewMatrix();

	world.Update(GamePlayer.GetChunkPosition());
}
