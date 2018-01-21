//
// Created by adamyuan on 1/21/18.
//

#ifndef WORLDCONTROLLER_HPP
#define WORLDCONTROLLER_HPP

#include <string>
#include <MyGL/Matrices.hpp>
#include "World.hpp"
#include "UI.hpp"

struct GLFWwindow;
class WorldController
{
private:
	GLFWwindow *Window;
	int Width = 720, Height = 480;
	World world;
	UI GameUI;
	MyGL::Matrices Matrices;
	MyGL::FrameRateManager FramerateManager;

	float FPS = 0.0f;
	bool control, showUI;
	static void focusCallback(GLFWwindow* window, int focused);
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
	void Resize(int width, int height);
	void Render();
	void RenderUI();
	void LogicProcess();
public:
	explicit WorldController(GLFWwindow *window, const std::string &worldName);
	void Update();
	~WorldController();
};


#endif //WORLDCONTROLLER_HPP
