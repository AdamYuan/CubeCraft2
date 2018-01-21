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

void UI::Init(GLFWwindow *window)
{
	ImGui_ImplGlfwGL3_Init(window);
	ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", 16);
}

void UI::CaptureEvent(GLFWwindow *window, bool value)
{
	ImGui::GetIO().WantCaptureKeyboard = value;
	ImGui::GetIO().WantCaptureMouse = value;
	if(value)
		ImGui_ImplGlfwGL3_InstallCallbacks(window);
}

void UI::Shutdown()
{
	ImGui_ImplGlfwGL3_Shutdown();
}

