//
// Created by adamyuan on 1/19/18.
//

#ifndef WORLDDATA_HPP
#define WORLDDATA_HPP

#include "Setting.hpp"
#include <glm/glm.hpp>
#include <string>
#include <sqlite3.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>

class World;
class Player;

struct DBInsertBlockInfo
{
	glm::ivec2 chunkPos;
	int index;
	uint8_t block;
};

class WorldData
{
private:
	std::atomic_bool Running;
	std::mutex QueueMutex;
	std::thread InsertBlockThread;
	std::queue<DBInsertBlockInfo> InsertBlockQueue;
	std::condition_variable Cond;

	sqlite3 *DB;
	std::mutex DBMutex;
	sqlite3_stmt *DeleteBlockStmt, *LoadBlocksStmt, *InsertBlockStmt;
	std::string DataFileName, SeedFileName;

	void InsertBlockWorker();
public:
	explicit WorldData(const std::string &name);
	~WorldData();
	void LoadBlocks(const glm::ivec2 &chunkPos, uint8_t (&Grid)[CHUNK_INFO_SIZE * WORLD_HEIGHT]);
	void InsertBlock(const glm::ivec2 &chunkPos, int index, uint8_t block);

	void LoadWorld(World &world);
	void SaveWorld(World &world);
};


#endif //WORLDDATA_HPP
