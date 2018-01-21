//
// Created by adamyuan on 1/21/18.
//

#include "GameMenu.hpp"
#include "UI.hpp"
#include "Renderer.hpp"
#include <GLFW/glfw3.h>

GameMenu::GameMenu(GLFWwindow *window) : Window(window), isQuit(false), enterGame(false)
{
	UI::CaptureEvent(window, true);
}

void GameMenu::Update()
{
	int width, height;
	glfwGetFramebufferSize(Window, &width, &height);
	glViewport(0, 0, width, height);

	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	Renderer::RenderMenuBg();

	UI::NewFrame();

	static ImVec2 buttonSize(250.0f, 0.0f);

	ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // Transparent background
	if (ImGui::Begin("MENU", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize
					 |ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::Text("CubeCraft");
		if(ImGui::Button("Start", buttonSize))
			enterGame = true;
		if(ImGui::Button("Quit", buttonSize))
			isQuit = true;

		ImGui::End();
	}
	ImGui::PopStyleColor();

	UI::Render();
	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

