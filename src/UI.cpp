//
// Created by adamyuan on 1/6/18.
//

#include "UI.hpp"

void UI::InitUI(GLFWwindow *window)
{
	ImGui_ImplGlfwGL3_Init(window, true);
	ImGui::GetIO().WantCaptureKeyboard = false;
	ImGui::GetIO().WantCaptureMouse = false;
	ImGui::GetIO().WantMoveMouse = false;
}

void UI::NewFrame()
{
	ImGui_ImplGlfwGL3_NewFrame();
}

void UI::Render()
{
	ImGui::Render();
}

void UI::Shutdown()
{
	ImGui_ImplGlfwGL3_Shutdown();
}
