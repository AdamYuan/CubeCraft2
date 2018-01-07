//
// Created by adamyuan on 1/6/18.
//

#include "UI.hpp"

void UI::NewFrame()
{
	ImGui_ImplGlfwGL3_NewFrame();
}

void UI::Render()
{
	ImGui::Render();
}

void UI::Init(GLFWwindow *window, bool captureEvent)
{
	ImGui_ImplGlfwGL3_Init(window, captureEvent);
	ImGui::GetIO().WantCaptureKeyboard = captureEvent;
	ImGui::GetIO().WantCaptureMouse = captureEvent;
}

UI::~UI()
{
	ImGui_ImplGlfwGL3_Shutdown();
}

