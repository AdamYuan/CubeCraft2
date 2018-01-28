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
	else if(State == MenuState::EditWorld)
		EditWorldDialog();
	else if(State == MenuState::Settings)
		EditSettings();

	UI::Render();
	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


void GameMenu::MainMenu()
{
	BeginCenterWindow("MENU");
	{
		ImGui::Text("CubeCraft");
		if(ImGui::Button("Start", s_buttonSize))
			GoToWorldList();
		if(ImGui::Button("Settings", s_buttonSize))
			State = MenuState::Settings;
		if(ImGui::Button("Quit", s_buttonSize))
			isQuit = true;
	}
	EndCenterWindow();
}

void GameMenu::WorldList()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
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
			if(ImGui::Button("Edit world"))
			{
				strcpy(InputBuf, WorldVector[CurrentIndex].c_str());
				State = MenuState::EditWorld;
			}
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
	ImGui::PopStyleColor();
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
	BeginCenterWindow("CREATE");
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
	}
	EndCenterWindow();
}

void GameMenu::DeleteWorldDialog()
{
	BeginCenterWindow("DELETE");
	{
		ImGui::Text("Are you sure?");
		if(ImGui::Button("Delete", s_buttonSize))
		{
			fs::remove_all(fs::path(WORLD_DIR(GetWorldName())));
			GoToWorldList();
		}
		if(ImGui::Button("Cancel", s_buttonSize))
			GoToWorldList();
	}
	EndCenterWindow();
}

void GameMenu::EditSettings()
{
	BeginCenterWindow("SETTINGS");
	{
		ImGui::SliderInt("Max loading threads", &Setting::LoadingThreadsNum, 1, 64);
		ImGui::SliderInt("Chunk loading range", &Setting::ChunkLoadRange, 4, 30);
		ImGui::SliderInt("Chunk deleting range", &Setting::ChunkDeleteRange, Setting::ChunkLoadRange, 50);
		if (Setting::ChunkDeleteRange < Setting::ChunkLoadRange)
			Setting::ChunkDeleteRange = Setting::ChunkLoadRange;

		if (ImGui::Button("Back", s_buttonSize))
			State = MenuState::Main;
	}
	EndCenterWindow();
}

void GameMenu::EditWorldDialog()
{
	BeginCenterWindow("EDIT");
	{
		ImGui::InputText("Name", InputBuf, WORLD_NAME_LENGTH);
		auto name = std::string(InputBuf);
		auto pathName = WORLD_DIR(name);
		fs::path worldPath(pathName.c_str());
		if (!fs::exists(worldPath) && !name.empty() && ImGui::Button("Rename", s_buttonSize))
		{
			fs::rename(fs::path(WORLD_DIR(WorldVector[CurrentIndex])), worldPath);
		}
		if (ImGui::Button("Back", s_buttonSize))
		{
			std::fill(InputBuf, InputBuf + WORLD_NAME_LENGTH, 0);
			GoToWorldList();
		}
	}
	EndCenterWindow();
}


void GameMenu::BeginCenterWindow(const char *name)
{
	ImGui::SetNextWindowPosCenter();
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::Begin(name, nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize
					 |ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
}

void GameMenu::EndCenterWindow()
{
	ImGui::End();
	ImGui::PopStyleColor();
}
