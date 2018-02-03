//
// Created by adamyuan on 1/19/18.
//

#include "WorldData.hpp"
#include "World.hpp"
#include <fstream>

WorldData::WorldData(const std::string &name) : running_(true)
{
	std::string dir_path = WORLD_DIR(name);
	data_filename_ = dir_path + DATA_FILE_NAME;
	seed_filename_ = dir_path + SEED_FILE_NAME;
	static const char *create_query =
			"create table if not exists block ("
					"	x int not null,"
					"	y int not null,"
					"	i int not null,"
					"	b int not null,"
					"	primary key(x, y, i)"
					");";
	sqlite3_open((dir_path + DB_NAME).c_str(), &db_);
	sqlite3_exec(db_, create_query, nullptr, nullptr, nullptr);

	static const char *insert_block_query =
			"insert or replace into block (x, y, i, b)"
					"values (?, ?, ?, ?);";
	sqlite3_prepare_v2(db_, insert_block_query, -1, &insert_block_stmt_, nullptr);

	static const char *load_blocks_query =
			"select i, b from block where x = ? and y = ?;";
	sqlite3_prepare_v2(db_, load_blocks_query, -1, &load_blocks_stmt_, nullptr);

	static const char *delete_block_query =
			"delete from block where x = ? and y = ? and i = ?;";
	sqlite3_prepare_v2(db_, delete_block_query, -1, &delete_block_stmt_, nullptr);

	insert_block_thread_ = std::thread(&WorldData::InsertBlockWorker, this);
}

void WorldData::LoadBlocks(const glm::ivec2 &chunk_pos, uint8_t (&grid)[CHUNK_INFO_SIZE * WORLD_HEIGHT])
{
	std::lock_guard<std::mutex> lockGuard(db_mutex_);
	if(!running_)
		return;
	sqlite3_reset(load_blocks_stmt_);
	sqlite3_bind_int(load_blocks_stmt_, 1, chunk_pos.x);
	sqlite3_bind_int(load_blocks_stmt_, 2, chunk_pos.y);
	while(sqlite3_step(load_blocks_stmt_) == SQLITE_ROW)
	{
		int index = sqlite3_column_int(load_blocks_stmt_, 0);
		auto block = (uint8_t)sqlite3_column_int(load_blocks_stmt_, 1);
		if(grid[index] != block)
			grid[index] = block;
		else
		{
			//delete block from database
			sqlite3_reset(delete_block_stmt_);
			sqlite3_bind_int(delete_block_stmt_, 1, chunk_pos.x);
			sqlite3_bind_int(delete_block_stmt_, 2, chunk_pos.y);
			sqlite3_bind_int(delete_block_stmt_, 3, index);
			sqlite3_step(delete_block_stmt_);
		}
	}
}

void WorldData::InsertBlock(const glm::ivec2 &chunk_pos, int index, uint8_t block)
{
	std::lock_guard<std::mutex> lk(queue_mutex_);
	insert_block_queue_.push({chunk_pos, index, block});
	condition_var_.notify_one();
}

WorldData::~WorldData()
{
	running_ = false;

	condition_var_.notify_one();
	insert_block_thread_.join();

	std::lock_guard<std::mutex> lk(db_mutex_);
	sqlite3_finalize(insert_block_stmt_);
	sqlite3_finalize(delete_block_stmt_);
	sqlite3_finalize(load_blocks_stmt_);
	sqlite3_close(db_);
}


void WorldData::InsertBlockWorker()
{
	DBInsertBlockInfo info;
	while(true)
	{
		std::unique_lock<std::mutex> db_unique_lock(queue_mutex_);
		condition_var_.wait(db_unique_lock, [this]{return !running_ || !insert_block_queue_.empty();});
		if(!running_)
			return;

		info = insert_block_queue_.front();
		insert_block_queue_.pop();
		db_unique_lock.unlock();

		std::lock_guard<std::mutex> db_lock_guard(db_mutex_);
		sqlite3_reset(insert_block_stmt_);
		sqlite3_bind_int(insert_block_stmt_, 1, info.chunk_pos.x);
		sqlite3_bind_int(insert_block_stmt_, 2, info.chunk_pos.y);
		sqlite3_bind_int(insert_block_stmt_, 3, info.index);
		sqlite3_bind_int(insert_block_stmt_, 4, (int)info.block);
		sqlite3_step(insert_block_stmt_);
	}
}

void WorldData::LoadWorld(World &world)
{
	world.timer_ = .25f;
	world.player_.flying_ = false;
	world.player_.position_ = glm::vec3(0.0f, 260.0f, 0.0f);

	std::ifstream data_file(data_filename_);
	if(data_file.is_open())
	{
		data_file >> world.timer_;

		Player &player = world.player_;
		data_file >> player.position_.x >> player.position_.y >> player.position_.z
				 >> player.flying_
				 >> player.camera_.Yaw >> player.camera_.Pitch
				 >> player.using_block_;

		data_file.close();
	}

	world.initial_time_ = (float)glfwGetTime() - world.timer_ * DAY_TIME;

	//get seed
	std::ifstream seed_file(seed_filename_);
	seed_file >> world.seed_;
}

void WorldData::SaveWorld(World &world)
{
	std::ofstream data_file(data_filename_);
	data_file.precision(30);

	Player &player = world.player_;
	data_file << world.timer_ << ' '
			 << player.position_.x << ' ' << player.position_.y << ' ' << player.position_.z << ' '
		 	 << player.flying_ << ' '
			 << player.camera_.Yaw << ' ' << player.camera_.Pitch << ' '
			 << player.using_block_ << std::endl;
}
