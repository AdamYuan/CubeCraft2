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
#define WORLD_NAME_LENGTH 32
class GameMenu
{
private:
	GLFWwindow *window_;
	bool is_quit_, enter_game_;
	MenuState state_;
	size_t current_index_;
	std::vector<std::string> world_vector_;
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
		state_ = MenuState::WorldSelection;
	}
	char input_buf_[WORLD_NAME_LENGTH], seed_buf_[WORLD_NAME_LENGTH];
public:
	explicit GameMenu(GLFWwindow *window);
	bool EnterGame() const { return enter_game_; }
	bool IsQuit() const { return is_quit_; }
	std::string GetWorldName() const { return world_vector_[current_index_]; }

	void Update();
};


#endif //GAMEMENU_HPP
