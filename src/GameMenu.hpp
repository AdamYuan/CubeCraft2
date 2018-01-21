//
// Created by adamyuan on 1/21/18.
//

#ifndef GAMEMENU_HPP
#define GAMEMENU_HPP

#include <string>

struct GLFWwindow;

class GameMenu
{
private:
	GLFWwindow *Window;
	bool isQuit, enterGame;
public:
	explicit GameMenu(GLFWwindow *window);
	bool EnterGame() { return enterGame; }
	bool IsQuit() { return isQuit; }
	std::string GetWorldName() { return "world"; }

	void Update();
};


#endif //GAMEMENU_HPP
