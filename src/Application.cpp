#include <iostream>
#include <GL/glew.h>
#include "Chunk.hpp"
#include "Application.hpp"
#include "Resource.hpp"
#include "UI.hpp"

#include <glm/gtx/string_cast.hpp>


//glfw callbacks
static bool control = true;
void focusCallback(GLFWwindow*, int focused)
{
	control = focused != 0;
}
static int sWidth, sHeight;
static bool resized = false, showFramewire = false, flying = false;
void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
	sWidth = width;
	sHeight = height;
	resized = true;
}
void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
	if(action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_ESCAPE)
			control = false;
		else if(key == GLFW_KEY_V)
			showFramewire = !showFramewire;
		else if(key == GLFW_KEY_F)
			flying = !flying;
	}
}

Application::Application() : world(), GamePlayer(world)
{
	InitWindow();
}

void Application::InitWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	Window = glfwCreateWindow(Width, Height, "CubeCraft2", nullptr, nullptr);
	if(Window == nullptr)
		throw std::runtime_error("ERROR WHEN CREATING WINDOW");

	glfwMakeContextCurrent(Window);

	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK)
		throw std::runtime_error("ERROR WHEN INITIALIZE GLEW");
	glViewport(0, 0, Width, Height);
	Matrices.UpdateMatrices(Width, Height);

	UI::InitUI(Window);
	Resource::InitResources();

	glfwSetWindowFocusCallback(Window, focusCallback);
	glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);
	glfwSetKeyCallback(Window, keyCallback);

	glCullFace(GL_CCW);
}

void Application::Run()
{
	GamePlayer.SetPosition({GamePlayer.GetPosition().x, 150.0f, GamePlayer.GetPosition().z});
	while(!glfwWindowShouldClose(Window))
	{
		//logic process
		LogicProcess();

		//render
		Render();

		UI::NewFrame();
		RenderUI();
		UI::Render();

		if(control)
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		else
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		glfwSwapBuffers(Window);

		glfwPollEvents();
	}
}

void Application::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

	if(showFramewire)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	world.Render(Matrices.Projection3d, ViewMatrix, GamePlayer.GetPosition());
}

void Application::LogicProcess()
{
	//update frame rate info
	FramerateManager.UpdateFrameRateInfo();


	static int lastTime = 0;
	if(glfwGetTime() > lastTime + 1)
	{
		lastTime = static_cast<int>(glfwGetTime());
		FPS = FramerateManager.GetFps();
	}

	//process size event
	if(resized)
	{
		Width = sWidth;
		Height = sHeight;
		resized = false;
		glViewport(0, 0, Width, Height);
		Matrices.UpdateMatrices(Width, Height, 60);
	}
	//get control when mouse press
	if(glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		glfwSetCursorPos(Window, Width / 2, Height / 2);
		control = true;
	}

	if(control)
	{
		GamePlayer.flying = flying;
		GamePlayer.MouseControl(Window, Width, Height);
		GamePlayer.KeyControl(Window, FramerateManager);
	}

	GamePlayer.PhysicsUpdate(FramerateManager);
	ViewMatrix = GamePlayer.GetViewMatrix();

	world.Update(GamePlayer.GetChunkPosition());
}

Application::~Application()
{
	UI::Shutdown();
}

void Application::RenderUI()
{
	//copied from IMGUI example

	//information box
    const float DISTANCE = 10.0f;
    static int corner = 0;
    ImVec2 window_pos = ImVec2((corner & 1) ? ImGui::GetIO().DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? ImGui::GetIO().DisplaySize.y - DISTANCE : DISTANCE);
    ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // Transparent background
    if (ImGui::Begin("", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::Text("fps: %f", FPS);
		ImGui::Text("memory: %.1f MB", (float)getCurrentRSS() / 1024.0f / 1024.0f);
		ImGui::Text("position: %s", glm::to_string(GamePlayer.GetPosition()).c_str());
		ImGui::Text("chunk position: %s", glm::to_string(GamePlayer.GetChunkPosition()).c_str());
		ImGui::Text("flying [F]: %s", GamePlayer.flying ? "true" : "false");
		ImGui::Text("frame wire [V]: %s", showFramewire ? "true" : "false");

        ImGui::End();
    }
	ImGui::PopStyleColor();
}
