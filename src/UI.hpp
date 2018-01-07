//
// Created by adamyuan on 1/6/18.
//

#ifndef UI_HPP
#define UI_HPP

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

class UI
{
public:
	void Init(GLFWwindow *window, bool captureEvent);
	void NewFrame();
	void Render();
	~UI();
};


#endif
