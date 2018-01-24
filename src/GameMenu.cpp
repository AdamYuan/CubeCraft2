//
// Created by adamyuan on 1/21/18.
//

#include "GameMenu.hpp"
#include "UI.hpp"
#include "Renderer.hpp"
#include <GLFW/glfw3.h>

#include <experimental/filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::experimental::filesystem;

static const ImVec2 s_buttonSize(250.0f, 0.0f);

GameMenu::GameMenu(GLFWwindow *window) : Window(window),
										 isQuit(false), enterGame(false), State(MenuState::Main)
{
	std::fill(InputBuf, InputBuf + WORLD_NAME_LENGTH, 0);
	std::fill(SeedBuf, SeedBuf + WORLD_NAME_LENGTH, 0);

	UI::CaptureEvent(window, true);

	fs::path saveFolder(SAVES_DIR);
	if(!fs::exists(saveFolder))
		fs::create_directory(saveFolder);
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

	if(State == MenuState::Main)
		MainMenu();
	else if(State == MenuState::WorldSelection)
		WorldList();
	else if(State == MenuState::CreateWorld)
		CreateWorldDialog();
	else if(State == MenuState::DeleteWorld)
		DeleteWorldDialog();

	UI::Render();
	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


void GameMenu::MainMenu()
{

	ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
	if (ImGui::Begin("MENU", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize
					 |ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::Text("CubeCraft");
		if(ImGui::Button("Start", s_buttonSize))
			GoToWorldList();
		if(ImGui::Button("Quit", s_buttonSize))
			isQuit = true;

		ImGui::End();
	}
	ImGui::PopStyleColor();
}

void GameMenu::WorldList()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y),
							 ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
	if (ImGui::Begin("WORLD LIST", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize
					 |ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar))
	{
		if(ImGui::Button("Back"))
			State = MenuState::Main;
		ImGui::SameLine();
		if(ImGui::Button("Create world"))
			State = MenuState::CreateWorld;
		if(!WorldVector.empty())
		{
			ImGui::SameLine();
			if(ImGui::Button("Delete world"))
				State = MenuState::DeleteWorld;
			ImGui::SameLine();
			if(ImGui::Button("Play"))
				enterGame = true;
		}
		ImGui::BeginChild("list", ImVec2(0, 0), true);
		ImGui::PushItemWidth(-1);
		for(size_t i=0; i<WorldVector.size(); ++i)
		{
			if(ImGui::Selectable(WorldVector[i].c_str(), CurrentIndex == i))
				CurrentIndex = i;
		}
		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::End();
	}
}

void GameMenu::UpdateWorldVector()
{
	CurrentIndex = 0;
	WorldVector.clear();
	for (auto &p : fs::directory_iterator(fs::path(SAVES_DIR)))
	{
		if(fs::is_directory(p.path()))
			WorldVector.push_back(p.path().filename().string());
	}
}

void GameMenu::CreateWorldDialog()
{
	ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
	if (ImGui::Begin("CREATE", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize
					 |ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::InputText("Name", InputBuf, WORLD_NAME_LENGTH);
		ImGui::InputText("Seed", SeedBuf, WORLD_NAME_LENGTH);
		if(*InputBuf && ImGui::Button("Create", s_buttonSize))
		{
			auto name = std::string(InputBuf);
			auto pathName = WORLD_DIR(name);
			fs::path worldPath(pathName.c_str());
			if(fs::exists(worldPath))
			{
				strcpy(InputBuf, "WORLD EXISTED");
			} else
			{
				if(fs::create_directory(worldPath))
				{
					{
						//calculate seed and write
						int seed = 0;
						for (char i : SeedBuf)
							if(i)
							{
								seed *= 10;
								seed += (int) (i - '0');
							}
						std::ofstream out(pathName + SEED_FILE_NAME);
						out << seed << std::endl;
						out.close();
					}
					std::fill(InputBuf, InputBuf + WORLD_NAME_LENGTH, 0);
					std::fill(SeedBuf, SeedBuf + WORLD_NAME_LENGTH, 0);
					GoToWorldList();
				}
				else
					strcpy(InputBuf, "INVALID WORLD NAME");
			}
		}
		if(ImGui::Button("Cancel", s_buttonSize))
			GoToWorldList();

		ImGui::End();
	}
	ImGui::PopStyleColor();
}

void GameMenu::DeleteWorldDialog()
{
	ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
	if (ImGui::Begin("DELETE", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize
					 |ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar))
	{
		ImGui::Text("Are you sure?");
		if(ImGui::Button("Delete", s_buttonSize))
		{
			fs::remove_all(fs::path(WORLD_DIR(GetWorldName())));
			GoToWorldList();
		}
		if(ImGui::Button("Cancel", s_buttonSize))
			GoToWorldList();

		ImGui::End();
	}
	ImGui::PopStyleColor();
}

