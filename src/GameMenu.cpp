//
// Created by adamyuan on 1/21/18.
//

#include "GameMenu.hpp"
#include "UI.hpp"
#include <GLFW/glfw3.h>

GameMenu::GameMenu(GLFWwindow *window) : Window(window), isQuit(false)
{
	UI::CaptureEvent(window, true);
}

bool GameMenu::IsQuit()
{
	return isQuit;
}

std::string GameMenu::GetWorldName()
{
	return "world";
}

void GameMenu::Update()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	UI::NewFrame();

	ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // Transparent background
	if (ImGui::Begin("MENU", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize
					 |ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::Text("CubeCraft");
		if(ImGui::Button("Start", ImVec2(100.0f, 0.0f)))
			isQuit = true;

		ImGui::End();
	}
	ImGui::PopStyleColor();

	UI::Render();
	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
