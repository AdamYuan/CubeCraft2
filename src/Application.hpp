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

	World world;

	Player GamePlayer;
	UI GameUI;
	glm::mat4 ViewMatrix;

	MyGL::Matrices Matrices;
	MyGL::FrameRateManager FramerateManager;

	float FPS = 0.0f;
	bool control, showFramewire;

	static void focusCallback(GLFWwindow* window, int focused);
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	inline static Application *getCallbackInstance(GLFWwindow* window);

public:
	Application();
	void Run();
	void InitWindow();
	void Render();
	void RenderUI();
	void LogicProcess();

};

#endif
