//
// Created by adamyuan on 1/6/18.
//

#ifndef UI_HPP
#define UI_HPP

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

namespace UI
{
	extern void Init(GLFWwindow *window);
	extern void CaptureEvent(GLFWwindow *window, bool value);
	extern void NewFrame();
	extern void Render();
	extern void Shutdown();
};


#endif
