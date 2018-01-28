//
// Created by adamyuan on 1/21/18.
//

#ifndef GAMEMENU_HPP
#define GAMEMENU_HPP

#include <vector>
#include <string>
#include "Setting.hpp"

struct GLFWwindow;

enum MenuState { Main = 0, WorldSelection, CreateWorld, DeleteWorld, EditWorld, Settings };
class GameMenu
{
private:
	GLFWwindow *Window;
	bool isQuit, enterGame;
	MenuState State;
	size_t CurrentIndex;
	std::vector<std::string> WorldVector;
	void MainMenu();
	void WorldList();
	void EditSettings();
	void CreateWorldDialog();
	void DeleteWorldDialog();
	void EditWorldDialog();

	void UpdateWorldVector();

	void BeginCenterWindow(const char *name);
	void EndCenterWindow();
	inline void GoToWorldList()
	{
		UpdateWorldVector();
		State = MenuState::WorldSelection;
	}
	char InputBuf[WORLD_NAME_LENGTH], SeedBuf[WORLD_NAME_LENGTH];
public:
	explicit GameMenu(GLFWwindow *window);
	bool EnterGame() { return enterGame; }
	bool IsQuit() { return isQuit; }
	std::string GetWorldName() { return WorldVector.at(CurrentIndex); }

	void Update();
};


#endif //GAMEMENU_HPP
