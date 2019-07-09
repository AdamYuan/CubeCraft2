//
// Created by adamyuan on 1/21/18.
//

#ifndef WORLDCONTROLLER_HPP
#define WORLDCONTROLLER_HPP

#include <string>
#include <mygl3/utils/matrices.hpp>
#include "World.hpp"

struct GLFWwindow;
class WorldController
{
private:
	GLFWwindow *window_;
	int width_, height_;
	World world_;
	mygl3::Matrices matrices_;
	mygl3::Framerate framerate_;

	bool control_, show_ui_, is_quit_;
	static void FocusCallback(GLFWwindow *window, int focused);
	static void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
	static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void ScrollCallback(GLFWwindow *window, double x_offset, double y_offset);
	void Render();
	void RenderUI();
	void LogicProcess();

public:
	explicit WorldController(GLFWwindow *window, const std::string &world_name);
	bool IsQuit();
	void Resize(int width, int height);
	void Update();
	~WorldController();
};


#endif //WORLDCONTROLLER_HPP
