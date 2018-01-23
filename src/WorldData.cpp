//
// Created by adamyuan on 1/19/18.
//

#include "WorldData.hpp"
#include "Player.hpp"
#include <fstream>

WorldData::WorldData(const std::string &name) : Running(true)
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

	InsertBlockThread = std::thread(&WorldData::InsertBlockWorker, this);
}

void WorldData::LoadBlocks(const glm::ivec2 &chunkPos, uint8_t (&Grid)[CHUNK_INFO_SIZE * WORLD_HEIGHT])
{
	if(!Running)
		return;
	std::lock_guard<std::mutex> lockGuard(DBMutex);
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
		{
			//delete block from database
			sqlite3_reset(DeleteBlockStmt);
			sqlite3_bind_int(DeleteBlockStmt, 1, chunkPos.x);
			sqlite3_bind_int(DeleteBlockStmt, 2, chunkPos.y);
			sqlite3_bind_int(DeleteBlockStmt, 3, index);
			sqlite3_step(DeleteBlockStmt);
		}
	}
}

void WorldData::InsertBlock(const glm::ivec2 &chunkPos, int index, uint8_t block)
{
	std::lock_guard<std::mutex> lockGuard(QueueMutex);
	InsertBlockQueue.push({chunkPos, index, block});
	Cond.notify_one();
}

WorldData::~WorldData()
{
	Running = false;

	Cond.notify_one();
	InsertBlockThread.join();

	std::lock_guard<std::mutex> lockGuard(DBMutex);
	sqlite3_finalize(InsertBlockStmt);
	sqlite3_finalize(DeleteBlockStmt);
	sqlite3_finalize(LoadBlocksStmt);
	sqlite3_close(DB);
}

#define PRECISION 30

float WorldData::LoadTime()
{
	float t = .25f;

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
		file >> pos.x >> pos.y >> pos.z >> flying >> player.Cam.Yaw >> player.Cam.Pitch >> player.UsingBlock;

	player.Position = pos;
	player.flying = flying;
}

void WorldData::SavePlayer(const Player &player)
{
	std::ofstream file(PlayerFileName);
	file.precision(PRECISION);
	file << player.Position.x << ' ' << player.Position.y << ' ' << player.Position.z << ' '
		 << player.flying << ' ' << player.Cam.Yaw << ' ' << player.Cam.Pitch << ' '
		 << player.UsingBlock << std::endl;
}

void WorldData::InsertBlockWorker()
{
	while(true)
	{
		std::unique_lock<std::mutex> lk(QueueMutex);
		Cond.wait(lk, [this]{return !Running || !InsertBlockQueue.empty();});
		if(!Running)
			return;

		auto i = InsertBlockQueue.front();
		InsertBlockQueue.pop();
		lk.unlock();

		std::lock_guard<std::mutex> dbLk(DBMutex);
		sqlite3_reset(InsertBlockStmt);
		sqlite3_bind_int(InsertBlockStmt, 1, i.chunkPos.x);
		sqlite3_bind_int(InsertBlockStmt, 2, i.chunkPos.y);
		sqlite3_bind_int(InsertBlockStmt, 3, i.index);
		sqlite3_bind_int(InsertBlockStmt, 4, (int)i.block);
		sqlite3_step(InsertBlockStmt);
	}
}
