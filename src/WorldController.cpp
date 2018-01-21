//
// Created by adamyuan on 1/21/18.
//

#include "Renderer.hpp"
#include "WorldController.hpp"
#include "UI.hpp"
#include <glm/gtx/string_cast.hpp>

void WorldController::focusCallback(GLFWwindow *window, int focused)
{
	auto target = (WorldController*)glfwGetWindowUserPointer(window);
	if(!focused)
		target->control = false;
}
void WorldController::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
	auto target = (WorldController*)glfwGetWindowUserPointer(window);
	target->Resize(width, height);
}
void WorldController::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	auto target = (WorldController*)glfwGetWindowUserPointer(window);
	if(action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_ESCAPE)
		{
			glfwSetCursorPos(window, target->Width / 2, target->Height / 2);
			target->control = !target->control;
		}
		else if(key == GLFW_KEY_F)
			target->world.player.flying = !target->world.player.flying;
		else if(key == GLFW_KEY_U)
			target->showUI = !target->showUI;
	}
}
void WorldController::scrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
	auto target = (WorldController*)glfwGetWindowUserPointer(window);

	auto y = (int)yOffset;

	uint8_t &usingBlock = target->world.player.UsingBlock;
	usingBlock -= y;
	if(usingBlock >= BLOCKS_NUM)
		usingBlock = 1;
	else if(usingBlock <= 0)
		usingBlock = BLOCKS_NUM - 1;
}

WorldController::WorldController(GLFWwindow *window, const std::string &worldName) :
		Window(window), world(worldName), control(true), showUI(true), isQuit(false)
{
	glfwSetWindowUserPointer(window, (void*)this);

	glfwSetWindowFocusCallback(window, focusCallback);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetScrollCallback(window, scrollCallback);

	UI::CaptureEvent(window, false);

	int width, height;
	glfwGetFramebufferSize(Window, &width, &height);
	Resize(width, height);
}

WorldController::~WorldController()
{
	glfwSetWindowUserPointer(Window, nullptr);

	glfwSetWindowFocusCallback(Window, nullptr);
	glfwSetFramebufferSizeCallback(Window, nullptr);
	glfwSetKeyCallback(Window, nullptr);
	glfwSetScrollCallback(Window, nullptr);
}

void WorldController::Update()
{
	//logic process
	LogicProcess();
	//render
	Render();
	if(showUI)
	{
		UI::NewFrame();
		RenderUI();
		UI::Render();
	}
	glfwSetInputMode(Window, GLFW_CURSOR, control ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
}

void WorldController::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1, 1, 1, 1);

	glm::mat4 ViewMatrix = world.player.GetViewMatrix();
	glm::mat4 vpMatrix = Matrices.Projection3d * ViewMatrix;

	Renderer::RenderSky(glm::mat3(ViewMatrix), Matrices.Projection3d,
						world.GetSunModelMatrix(), world.GetDayTime());
	Renderer::RenderWorld(world, vpMatrix, world.player.Position, world.player.GetSelection(false));
	if(control)
		Renderer::RenderCrosshair(Matrices.Matrix2dCenter);
}

void WorldController::LogicProcess()
{
	//update frame rate info
	FramerateManager.UpdateFrameRateInfo();

	static int lastTime = 0;
	if(glfwGetTime() > lastTime + 1)
	{
		lastTime = static_cast<int>(glfwGetTime());
		FPS = FramerateManager.GetFps();
	}

	world.player.Control(control, Window, Width, Height, FramerateManager, Matrices.Projection3d);
	world.Update(world.player.GetChunkPosition());
}

void WorldController::RenderUI()
{
	//information box
	if(control)
	{
		const static ImVec2 window_pos = ImVec2(10.0f, 10.0f);
		const static ImVec2 window_pos_pivot = ImVec2(1.0f, 1.0f);

		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // Transparent background
		if (ImGui::Begin("INFO", nullptr,
						 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize
						 |ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::Text("fps: %f", FPS);
			ImGui::Text("running threads: %u", world.GetRunningThreadNum());
			ImGui::Text("position: %s", glm::to_string(world.player.Position).c_str());
			ImGui::Text("chunk position: %s", glm::to_string(world.player.GetChunkPosition()).c_str());
			ImGui::Text("time: %f", world.GetDayTime());
			ImGui::Text("flying [F]: %s", world.player.flying ? "true" : "false");
			ImGui::Text("using block: %s", BlockMethods::GetName(world.player.UsingBlock));

			ImGui::End();
		}
		ImGui::PopStyleColor();
	}
	else //game menu
	{
		ImGui::SetNextWindowPosCenter(ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // Transparent background
		if (ImGui::Begin("MENU", nullptr,
						 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize
						 |ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::Text("Game menu");
			if(ImGui::Button("Continue [ESC]", ImVec2(500.0f, 0.0f)))
			{
				glfwSetCursorPos(Window, Width / 2, Height / 2);
				control = true;
			}
			if(ImGui::Button("Save and Quit", ImVec2(500.0f, 0.0f)))
				isQuit = true;

			ImGui::End();
		}
		ImGui::PopStyleColor();
	}
}

void WorldController::Resize(int width, int height)
{
	Width = width;
	Height = height;
	glViewport(0, 0, width, height);
	Matrices.UpdateMatrices(width, height, WALK_FOVY);

	if(control)
		glfwSetCursorPos(Window, width / 2, height / 2);
}

bool WorldController::IsQuit()
{
	return isQuit;
}

