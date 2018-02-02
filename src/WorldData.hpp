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
	glm::ivec2 chunk_pos;
	int index;
	uint8_t block;
};

class WorldData
{
private:
	std::atomic_bool running_;
	std::mutex queue_mutex_;
	std::thread insert_block_thread_;
	std::queue<DBInsertBlockInfo> insert_block_queue_;
	std::condition_variable condition_var_;

	sqlite3 *db_;
	std::mutex db_mutex_;
	sqlite3_stmt *delete_block_stmt_, *load_blocks_stmt_, *insert_block_stmt_;
	std::string data_filename_, seed_filename_;

	void InsertBlockWorker();
public:
	explicit WorldData(const std::string &name);
	~WorldData();
	void LoadBlocks(const glm::ivec2 &chunk_pos, uint8_t (&grid)[CHUNK_INFO_SIZE * WORLD_HEIGHT]);
	void InsertBlock(const glm::ivec2 &chunk_pos, int index, uint8_t block);

	void LoadWorld(World &world);
	void SaveWorld(World &world);
};


#endif //WORLDDATA_HPP
