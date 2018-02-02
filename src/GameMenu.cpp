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

GameMenu::GameMenu(GLFWwindow *window) : window_(window),
										 is_quit_(false), enter_game_(false), state_(MenuState::Main)
{
	std::fill(input_buf_, input_buf_ + WORLD_NAME_LENGTH, 0);
	std::fill(seed_buf_, seed_buf_ + WORLD_NAME_LENGTH, 0);

	UI::CaptureEvent(window, true);

	fs::path saveFolder(SAVES_DIR);
	if(!fs::exists(saveFolder))
		fs::create_directory(saveFolder);
}

void GameMenu::Update()
{
	int width, height;
	glfwGetFramebufferSize(window_, &width, &height);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	Renderer::RenderMenuBg();

	UI::NewFrame();

	if(state_ == MenuState::Main)
		MainMenu();
	else if(state_ == MenuState::WorldSelection)
		WorldList();
	else if(state_ == MenuState::CreateWorld)
		CreateWorldDialog();
	else if(state_ == MenuState::DeleteWorld)
		DeleteWorldDialog();
	else if(state_ == MenuState::EditWorld)
		EditWorldDialog();
	else if(state_ == MenuState::Settings)
		EditSettings();

	UI::Render();
	glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


void GameMenu::MainMenu()
{
	BeginCenterWindow("MENU");
	{
		ImGui::Text("CubeCraft");
		if(ImGui::Button("Start", s_buttonSize))
			GoToWorldList();
		if(ImGui::Button("Settings", s_buttonSize))
			state_ = MenuState::Settings;
		if(ImGui::Button("Quit", s_buttonSize))
			is_quit_ = true;
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
			state_ = MenuState::Main;
		ImGui::SameLine();
		if(ImGui::Button("Create world"))
			state_ = MenuState::CreateWorld;
		if(!world_vector_.empty())
		{
			ImGui::SameLine();
			if(ImGui::Button("Delete world"))
				state_ = MenuState::DeleteWorld;
			ImGui::SameLine();
			if(ImGui::Button("Edit world"))
			{
				strcpy(input_buf_, world_vector_[current_index_].c_str());
				state_ = MenuState::EditWorld;
			}
			ImGui::SameLine();
			if(ImGui::Button("Play"))
				enter_game_ = true;
		}
		ImGui::BeginChild("list", ImVec2(0, 0), true);
		ImGui::PushItemWidth(-1);
		for(size_t i=0; i<world_vector_.size(); ++i)
		{
			if(ImGui::Selectable(world_vector_[i].c_str(), current_index_ == i))
				current_index_ = i;
		}
		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::End();
	}
	ImGui::PopStyleColor();
}

void GameMenu::UpdateWorldVector()
{
	current_index_ = 0;
	world_vector_.clear();
	for (auto &p : fs::directory_iterator(fs::path(SAVES_DIR)))
	{
		if(fs::is_directory(p.path()))
			world_vector_.push_back(p.path().filename().string());
	}
}

void GameMenu::CreateWorldDialog()
{
	BeginCenterWindow("CREATE");
	{
		ImGui::InputText("Name", input_buf_, WORLD_NAME_LENGTH);
		ImGui::InputText("Seed", seed_buf_, WORLD_NAME_LENGTH);
		if(*input_buf_ && ImGui::Button("Create", s_buttonSize))
		{
			auto name = std::string(input_buf_);
			auto pathName = WORLD_DIR(name);
			fs::path worldPath(pathName.c_str());
			if(fs::exists(worldPath))
			{
				strcpy(input_buf_, "WORLD EXISTED");
			} else
			{
				if(fs::create_directory(worldPath))
				{
					{
						//calculate seed and write
						int seed = 0;
						for (char i : seed_buf_)
							if(i)
							{
								seed *= 10;
								seed += (int) (i - '0');
							}
						std::ofstream out(pathName + SEED_FILE_NAME);
						out << seed << std::endl;
						out.close();
					}
					std::fill(input_buf_, input_buf_ + WORLD_NAME_LENGTH, 0);
					std::fill(seed_buf_, seed_buf_ + WORLD_NAME_LENGTH, 0);
					GoToWorldList();
				}
				else
					strcpy(input_buf_, "INVALID WORLD NAME");
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
			state_ = MenuState::Main;
	}
	EndCenterWindow();
}

void GameMenu::EditWorldDialog()
{
	BeginCenterWindow("EDIT");
	{
		ImGui::InputText("Name", input_buf_, WORLD_NAME_LENGTH);
		auto name = std::string(input_buf_);
		auto pathName = WORLD_DIR(name);
		fs::path worldPath(pathName.c_str());
		if (!fs::exists(worldPath) && !name.empty() && ImGui::Button("Rename", s_buttonSize))
		{
			fs::rename(fs::path(WORLD_DIR(world_vector_[current_index_])), worldPath);
		}
		if (ImGui::Button("Back", s_buttonSize))
		{
			std::fill(input_buf_, input_buf_ + WORLD_NAME_LENGTH, 0);
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
