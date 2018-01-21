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
	bool isQuit;
public:
	explicit GameMenu(GLFWwindow *window);
	bool IsQuit();
	void Update();
	std::string GetWorldName();
};


#endif //GAMEMENU_HPP
