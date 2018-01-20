//
// Created by adamyuan on 1/19/18.
//

#include "WorldData.hpp"
#include "Player.hpp"
#include <fstream>

WorldData::WorldData(const std::string &name)
{
	PlayerFileName = name+"_player.dat";
	TimeFileName = name+"_time.dat";
	static const char *createQuery =
			"create table if not exists block ("
					"	x int not null,"
					"	y int not null,"
					"	i int not null,"
					"	b int not null,"
					"	primary key(x, y, i)"
					");";
	sqlite3_open((name + ".db").c_str(), &DB);
	sqlite3_exec(DB, createQuery, nullptr, nullptr, nullptr);
	sqlite3_exec(DB, "PRAGMA synchronous=OFF", nullptr, nullptr, nullptr);

	static const char *insertBlockQuery =
			"insert or replace into block (x, y, i, b)"
					"values (?, ?, ?, ?);";
	sqlite3_prepare_v2(DB, insertBlockQuery, -1, &InsertBlockStmt, nullptr);

	static const char *loadBlocksQuery =
			"select i, b from block where x = ? and y = ?;";
	sqlite3_prepare_v2(DB, loadBlocksQuery, -1, &LoadBlocksStmt, nullptr);

	static const char *deleteBlockQuery =
			"delete from block where x = ? and y = ? and i = ?;";
	sqlite3_prepare_v2(DB, deleteBlockQuery, -1, &DeleteBlockStmt, nullptr);
}

void WorldData::LoadBlocks(const glm::ivec2 &chunkPos, uint8_t (&Grid)[CHUNK_INFO_SIZE * WORLD_HEIGHT])
{
	sqlite3_reset(LoadBlocksStmt);
	sqlite3_bind_int(LoadBlocksStmt, 1, chunkPos.x);
	sqlite3_bind_int(LoadBlocksStmt, 2, chunkPos.y);
	while(sqlite3_step(LoadBlocksStmt) == SQLITE_ROW)
	{
		int index = sqlite3_column_int(LoadBlocksStmt, 0);
		auto block = (uint8_t)sqlite3_column_int(LoadBlocksStmt, 1);
		if(Grid[index] != block)
			Grid[index] = block;
		else
			deleteBlock(chunkPos, index);
	}
}

void WorldData::InsertBlock(const glm::ivec2 &chunkPos, int index, uint8_t block)
{
	sqlite3_reset(InsertBlockStmt);
	sqlite3_bind_int(InsertBlockStmt, 1, chunkPos.x);
	sqlite3_bind_int(InsertBlockStmt, 2, chunkPos.y);
	sqlite3_bind_int(InsertBlockStmt, 3, index);
	sqlite3_bind_int(InsertBlockStmt, 4, (int)block);
	sqlite3_step(InsertBlockStmt);
}

void WorldData::deleteBlock(const glm::ivec2 &chunkPos, int index)
{
	sqlite3_reset(DeleteBlockStmt);
	sqlite3_bind_int(DeleteBlockStmt, 1, chunkPos.x);
	sqlite3_bind_int(DeleteBlockStmt, 2, chunkPos.y);
	sqlite3_bind_int(DeleteBlockStmt, 3, index);
	sqlite3_step(DeleteBlockStmt);
}

WorldData::~WorldData()
{
	sqlite3_finalize(InsertBlockStmt);
	sqlite3_finalize(DeleteBlockStmt);
	sqlite3_finalize(LoadBlocksStmt);
	sqlite3_close(DB);
}

#define PRECISION 30

float WorldData::LoadTime()
{
	float t = .25f * DAY_TIME;

	std::ifstream file(TimeFileName);
	if(file.is_open())
		file >> t;

	return t;
}

void WorldData::SaveTime(float time)
{
	std::ofstream file(TimeFileName);
	file.precision(PRECISION);
	file << time << std::endl;
}

void WorldData::LoadPlayer(Player &player)
{
	glm::vec3 pos(0.0f, 300.0f, 0.0f);
	bool flying = false;

	std::ifstream file(PlayerFileName);

	if(file.is_open())
		file >> pos.x >> pos.y >> pos.z >> flying;

	player.Position = pos;
	player.flying = flying;
}

void WorldData::SavePlayer(const Player &player)
{
	std::ofstream file(PlayerFileName);
	file.precision(PRECISION);
	file << player.Position.x << ' ' << player.Position.y << ' ' << player.Position.z << ' '
		 << player.flying << std::endl;
}