//
// Created by adamyuan on 1/19/18.
//

#ifndef WORLDDATA_HPP
#define WORLDDATA_HPP

#include "Setting.hpp"
#include <glm/glm.hpp>
#include <string>
#include <sqlite3.h>

class Player;
class WorldData
{
private:
	sqlite3 *DB;
	sqlite3_stmt *DeleteBlockStmt, *LoadBlocksStmt, *InsertBlockStmt;
	std::string PlayerFileName, TimeFileName;
	void deleteBlock(const glm::ivec2 &chunkPos, int index);
public:
	explicit WorldData(const std::string &name);
	~WorldData();
	void LoadBlocks(const glm::ivec2 &chunkPos, uint8_t (&Grid)[CHUNK_INFO_SIZE * WORLD_HEIGHT]);
	void InsertBlock(const glm::ivec2 &chunkPos, int index, uint8_t block);

	void LoadPlayer(Player &player);
	void SavePlayer(const Player &player);

	float LoadTime();
	void SaveTime(float time);
};


#endif //WORLDDATA_HPP
