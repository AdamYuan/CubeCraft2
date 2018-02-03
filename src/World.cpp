//
// Created by adamyuan on 12/23/17.
//

#include "World.hpp"
#include "ChunkAlgorithm.hpp"
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

World::World(const std::string &name)
		: thread_pool_((size_t)Setting::LoadingThreadsNum),
		  running_threads_(0), xz_pos_changed_(false), pos_changed_(false),
		  world_data_(name), player_(*this), center_(INT_MAX), render_order_update_counter_(0)
{
	cmp2 = std::bind(&World::cmp2_impl, this, std::placeholders::_1, std::placeholders::_2);
	cmp3 = std::bind(&World::cmp3_impl, this, std::placeholders::_1, std::placeholders::_2);
	rcmp2 = std::bind(&World::rcmp2_impl, this, std::placeholders::_1, std::placeholders::_2);
	rcmp3 = std::bind(&World::rcmp3_impl, this, std::placeholders::_1, std::placeholders::_2);

	name_ = name;
	loading_thread_num_ = lighting_thread_num_ = meshing_thread_num_ = 0;
	glm::ivec3 _;
	int index = 0;
	for(_.x = -1; _.x <= 1; ++_.x)
		for(_.y = -1; _.y <= 1; ++_.y)
			for(_.z = -1; _.z <= 1; ++_.z)
				meshing_lookup_array_[index++] = _;

	world_data_.LoadWorld(*this);

	const size_t _SIZE = ((size_t)Setting::ChunkLoadRange*2+1) * ((size_t)Setting::ChunkLoadRange*2+1) * WORLD_HEIGHT;
	pre_meshing_vector_.reserve(_SIZE);
	pre_initial_lighting_vector_.reserve(_SIZE);
}

World::~World()
{
	{
		std::lock_guard<std::mutex> lk(mutex_);
		chunk_map_.clear();
	}
	world_data_.SaveWorld(*this);
}



void World::Update(const glm::ivec3 &center)
{
	//update time
	timer_ = ((float)glfwGetTime() - initial_time_) / DAY_TIME;
	timer_ = std::fmod(timer_, 1.0f);

	float radians = timer_ * 6.28318530718f;
	sun_model_matrix_ = glm::rotate(glm::mat4(1.0f), radians, glm::vec3(0.0f, 0.0f, 1.0f));
	sun_position_ = glm::vec3(sun_model_matrix_ * glm::vec4(0.0f, -1.0f, 0.0f, 1.0f));

	//global flags
	xz_pos_changed_ = false, pos_changed_ = false;
	if(center != center_)
	{
		pos_changed_ = true;
		if(center.x != center_.x || center.z != center_.z)
			xz_pos_changed_ = true;

		center_ = center;
	}

	{
		//set lock
		std::lock_guard<std::mutex> lk(mutex_);

		for(short t=0; t<=1; ++t)
		{
			if(!render_addition_set_[t].empty())
			{
				render_set_[t].insert(render_addition_set_[t].begin(), render_addition_set_[t].end());
				render_addition_set_[t].clear();
			}
			if(!render_removal_set_[t].empty())
			{
				for(const auto &i : render_removal_set_[t])
					render_set_[t].erase(i);
				render_removal_set_[t].clear();
			}
		}

		if(render_order_update_counter_ == RENDER_ORDER_UPDATE_FREQUENCY)
		{
			transparent_render_vector_ = std::vector<glm::ivec3>(render_set_[1].begin(), render_set_[1].end());
			std::sort(transparent_render_vector_.begin(), transparent_render_vector_.end(), rcmp3);
			render_order_update_counter_ = 0;
		}
		else
			render_order_update_counter_ ++;

		if(xz_pos_changed_)
		{
			//remove all chunks out of the range
			for(auto iter = chunk_map_.begin(); iter != chunk_map_.end(); ) {
				if (iter->first.x < center.x - Setting::ChunkDeleteRange ||
					iter->first.x > center.x + Setting::ChunkDeleteRange ||
					iter->first.z < center.z - Setting::ChunkDeleteRange ||
					iter->first.z > center.z + Setting::ChunkDeleteRange)
				{
					render_set_[0].erase(iter->first);
					render_set_[1].erase(iter->first);
					iter = chunk_map_.erase(iter);
				}
				else
					++iter;
			}
			//generate chunks in the range
			glm::ivec3 i;
			for(i.x = center.x - Setting::ChunkLoadRange; i.x <= center.x + Setting::ChunkLoadRange; ++i.x)
				for(i.z = center.z - Setting::ChunkLoadRange; i.z <= center.z + Setting::ChunkLoadRange; ++i.z)
					for(i.y = 0; i.y < WORLD_HEIGHT; ++i.y)
						if(!ChunkExist(i))
							SetChunk(i);
		}

		ProcessChunkUpdates();
		UpdateChunkLoadingList();
		UpdateChunkSunLightingList();
		UpdateChunkMeshingList();
	}
}

void World::ProcessChunkUpdates()
{
	if(!mesh_directly_update_set_.empty())
	{
		ChunkAlgorithm::SunLightRemovalBFS(this, sun_light_removal_queue_, sun_light_queue_);
		ChunkAlgorithm::SunLightBFS(this, sun_light_queue_);
		ChunkAlgorithm::TorchLightRemovalBFS(this, torch_light_removal_queue_, torch_light_queue_);
		ChunkAlgorithm::TorchLightBFS(this, torch_light_queue_);

		for(const glm::ivec3 &pos : mesh_directly_update_set_)
		{
			mesh_threaded_update_set_.erase(pos);

			if(!ChunkExist(pos))
				continue;
			if(!GetChunk(pos)->initialized_mesh_)
				continue;

			std::vector<ChunkRenderVertex> mesh_vertices[2];
			std::vector<unsigned int> mesh_indices[2];
			ChunkAlgorithm::Meshing(this, pos, mesh_vertices, mesh_indices);
			ChunkAlgorithm::ApplyMesh(GetChunk(pos), 0, mesh_vertices, mesh_indices);
			ChunkAlgorithm::ApplyMesh(GetChunk(pos), 1, mesh_vertices, mesh_indices);
			//update render set

			for(short t=0; t<=1; ++t)
			{
				if(!mesh_vertices[t].empty())
					render_set_[t].insert(pos);
				else
					render_set_[t].erase(pos);
			}
		}
		mesh_directly_update_set_.clear();
	}
}

void World::UpdateChunkLoadingList()
{
	if(xz_pos_changed_)
	{
		pre_loading_vector_.clear();
		glm::ivec2 iter;
		for(iter.x = center_.x - Setting::ChunkLoadRange; iter.x <= center_.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.y = center_.z - Setting::ChunkLoadRange; iter.y <= center_.z + Setting::ChunkLoadRange; ++iter.y)
				if(!GetChunk({iter.x, 0, iter.y})->loaded_terrain_ && !loading_info_set_.count(iter))
					pre_loading_vector_.push_back(iter);
		std::sort(pre_loading_vector_.begin(), pre_loading_vector_.end(), rcmp2);
	}
	while(!pre_loading_vector_.empty() && loading_thread_num_ < Setting::LoadingThreadsNum)
	{
		loading_thread_num_ ++;
		thread_pool_.enqueue(&World::ChunkLoadingWorker, this, pre_loading_vector_.back());
		pre_loading_vector_.pop_back();
	}
}

void World::UpdateChunkSunLightingList()
{
	if(xz_pos_changed_)
	{
		pre_initial_lighting_vector_.clear();
		glm::ivec2 iter;
		for(iter.x = center_.x - Setting::ChunkLoadRange + 1; iter.x < center_.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.y = center_.z - Setting::ChunkLoadRange + 1; iter.y < center_.z + Setting::ChunkLoadRange; ++iter.y)
				if(!GetChunk({iter.x, 0, iter.y})->initialized_lighting_ && !initial_lighting_info_set_.count(iter))
					pre_initial_lighting_vector_.push_back(iter);
		std::sort(pre_initial_lighting_vector_.begin(), pre_initial_lighting_vector_.end(), cmp2);
	}

	for(auto iter = pre_initial_lighting_vector_.begin();
		iter != pre_initial_lighting_vector_.end() && lighting_thread_num_ < Setting::LoadingThreadsNum; )
	{
		glm::ivec3 i(iter->x, 0, iter->y);
		bool flag = true;
		for(i.x = iter->x-1; flag && i.x <= iter->x+1; ++i.x)
			for(i.z = iter->y-1; i.z <= iter->y+1; ++i.z)
				if (!GetChunk(i)->loaded_terrain_) {
					flag = false;
					break;
				}

		if(flag)
		{
			initial_lighting_info_set_.insert(*iter);
			thread_pool_.enqueue(&World::ChunkInitialLightingWorker, this, *iter);
			lighting_thread_num_ ++;

			iter = pre_initial_lighting_vector_.erase(iter);
		}
		else
			++iter;
	}
}

void World::UpdateChunkMeshingList()
{
	//mesh initialize
	if(xz_pos_changed_)
	{
		pre_meshing_vector_.clear();
		glm::ivec3 iter;
		for(iter.x = center_.x - Setting::ChunkLoadRange + 1; iter.x < center_.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.z = center_.z - Setting::ChunkLoadRange + 1; iter.z < center_.z + Setting::ChunkLoadRange; ++iter.z)
				for(iter.y = 0; iter.y < WORLD_HEIGHT; ++iter.y)
					if(!GetChunk(iter)->initialized_mesh_ && !meshing_info_set_.count(iter))
						pre_meshing_vector_.push_back(iter);
	}
	if(pos_changed_)
		std::sort(pre_meshing_vector_.begin(), pre_meshing_vector_.end(), cmp3);


	for(auto iter = pre_meshing_vector_.begin();
		iter != pre_meshing_vector_.end() && meshing_thread_num_ < Setting::LoadingThreadsNum; )
	{
		bool flag = true;
		for (auto i : meshing_lookup_array_)
		{
			ChunkPtr chk = GetChunk(*iter + i);
			//check that all the terrain are loaded
			if(chk && !chk->initialized_lighting_)
			{
				flag = false;
				break;
			}
		}

		if(flag)
		{
			meshing_info_set_.insert(*iter);
			thread_pool_.enqueue(&World::ChunkMeshingWorker, this, *iter);
			meshing_thread_num_ ++;

			iter = pre_meshing_vector_.erase(iter);
		}
		else
			++iter;
	}

	//chunk mesh update
	if(!mesh_threaded_update_set_.empty())
	{
		for(const glm::ivec3 &pos : mesh_threaded_update_set_)
		{
			if(!ChunkExist(pos))
				continue;
			if(!GetChunk(pos)->initialized_mesh_)
				continue;
			if(mesh_update_info_set_.count(pos))
				continue;

			mesh_update_info_set_.insert(pos);
			thread_pool_.enqueue(&World::ChunkMeshUpdateWorker, this, pos);
		}
		mesh_threaded_update_set_.clear();
	}
}


void World::ChunkLoadingWorker(const glm::ivec2 &pos)
{
	glm::ivec3 i(pos.x, 0, pos.y);
	{
		std::lock_guard<std::mutex> lk(mutex_);
		if(!ChunkExist(i))
		{
			loading_thread_num_ --;
			return;
		}
	}

	running_threads_ ++;
	ChunkLoadingInfo info(pos, seed_, world_data_);
	info.Process();
	running_threads_ --;

	{
		std::lock_guard<std::mutex> lk(mutex_);
		loading_info_set_.erase(pos);
		if (ChunkExist(i))
		{
			ChunkPtr arr[WORLD_HEIGHT];
			for (; i.y < WORLD_HEIGHT; i.y++)
				arr[i.y] = GetChunk(i);

			info.ApplyTerrain(arr);
		}
		loading_thread_num_--;
	}
}

void World::ChunkInitialLightingWorker(const glm::ivec2 &pos)
{
	ChunkPtr copy_arr[WORLD_HEIGHT * 9], result_arr[WORLD_HEIGHT];

	{
		std::lock_guard<std::mutex> lk(mutex_);

		bool flag = true;
		int index = 0;
		glm::ivec3 i(pos.x, 0, pos.y);
		for(i.x = pos.x-1; flag && i.x <= pos.x+1; ++i.x)
			for(i.z = pos.y-1; i.z <= pos.y+1; ++i.z) {
				i.y = 0;
				ChunkPtr chk = GetChunk(i);
				if (!chk || !chk->loaded_terrain_) {
					flag = false;
					break;
				}
				copy_arr[index++] = chk;
				for (i.y = 1; i.y < WORLD_HEIGHT; i.y++)
					copy_arr[index++] = GetChunk(i);
			}
		if(!flag)
		{
			lighting_thread_num_ --;
			return;
		}
	}

	running_threads_ ++;
	ChunkInitialLightingInfo info(copy_arr);
	info.Process();
	running_threads_ --;

	{
		std::lock_guard<std::mutex> lk(mutex_);

		glm::ivec3 i(pos.x, 0, pos.y);
		if(ChunkExist(i))
		{
			for(; i.y<WORLD_HEIGHT; ++i.y)
				result_arr[i.y] = GetChunk(i);

			info.ApplyLighting(result_arr);
		}
		initial_lighting_info_set_.erase(pos);
		lighting_thread_num_ --;
	}
}

void World::ChunkMeshingWorker(const glm::ivec3 &pos)
{
	ChunkPtr arr[27];
	{
		std::lock_guard<std::mutex> lk(mutex_);

		bool flag = true;
		for (int i=0; i<27; ++i)
		{
			glm::ivec3 neighbour = pos + meshing_lookup_array_[i];
			arr[i] = GetChunk(neighbour);
			//check that all the terrain are loaded
			if(neighbour.y >= 0 && neighbour.y < WORLD_HEIGHT && !arr[i])
			{
				flag = false;
				break;
			}
		}
		if(!flag)
		{
			meshing_thread_num_ --;
			return;
		}
	}

	running_threads_ ++;
	ChunkMeshingInfo info(arr);
	info.Process();
	running_threads_ --;

	{
		std::lock_guard<std::mutex> lk(mutex_);

		meshing_info_set_.erase(pos);
		ChunkPtr chk = GetChunk(pos);
		if(chk)
		{
			info.ApplyResult(chk);
			for(short t=0; t<=1; ++t)
			{
				if(!chk->mesh_vertices_[t].empty())
					render_addition_set_[t].insert(pos);
			}
		}
		meshing_thread_num_ --;
	}
}

void World::ChunkMeshUpdateWorker(const glm::ivec3 &pos)
{
	ChunkPtr arr[27];

	{
		std::lock_guard<std::mutex> lk(mutex_);

		if(!ChunkExist(pos))
			return;
		if(!GetChunk(pos)->initialized_mesh_)
			return;
		for(int i=0; i<27; ++i)
			arr[i] = GetChunk(pos + meshing_lookup_array_[i]);
	}

	running_threads_ ++;
	ChunkMeshingInfo info(arr);
	info.Process();
	running_threads_ --;

	{
		std::lock_guard<std::mutex> lk(mutex_);

		ChunkPtr chk = GetChunk(pos);
		if(chk)
		{
			info.ApplyResult(chk);
			for(short t=0; t<=1; ++t)
			{
				if(chk->mesh_vertices_[t].empty())
					render_removal_set_[t].insert(pos);
				else
					render_addition_set_[t].insert(pos);
			}
		}
		mesh_update_info_set_.erase(pos);
	}
}

void World::SetBlock(const glm::ivec3 &pos, Block blk, bool check_update)
{
	glm::ivec3 chunk_pos = BlockPosToChunkPos(pos),
			related_pos = pos - chunk_pos * CHUNK_SIZE;

	ChunkPtr chk = GetChunk(chunk_pos);
	if(!chk)
		return;

	uint8_t last_sunlight = chk->GetSunLight(related_pos),
			last_block = chk->GetBlock(related_pos),
			last_torchlight = chk->GetTorchLight(related_pos);
	LightLevel torchlight_now = BlockMethods::GetLightLevel(blk);

	if(last_block == blk)
		return;

	chk->SetBlock(related_pos, blk);

	if(check_update)
	{
		world_data_.InsertBlock({chunk_pos.x, chunk_pos.z}, XYZ(related_pos.x, pos.y, related_pos.z), blk);
		AddRelatedChunks(pos, mesh_directly_update_set_);

		//update lighting
		if(BlockMethods::LightCanPass(blk) != BlockMethods::LightCanPass(last_block))
		{
			if(!BlockMethods::LightCanPass(blk))
			{
				if(last_sunlight)//have sunlight
				{
					sun_light_removal_queue_.push({pos, last_sunlight});
					chk->SetSunLight(related_pos, 0);
				}
			}
			else
			{
				for(short face = 0; face < 6; ++face)
				{
					glm::ivec3 neighbour = util::FaceExtend(pos, face);
					LightLevel light;

					if(!last_sunlight)
					{
						light = GetSunLight(neighbour);
						if(light)
							sun_light_queue_.push({neighbour, light});
					}

					if(!last_torchlight)
					{
						light = GetTorchLight(neighbour);
						if(light)
							torch_light_queue_.push({neighbour, light});
					}
				}
			}
			if(torchlight_now != last_torchlight)
			{
				if(torchlight_now < last_torchlight && last_torchlight)
				{
					torch_light_removal_queue_.push({pos, last_torchlight});
					chk->SetTorchLight(related_pos, 0);
				}
				if(torchlight_now)
				{
					torch_light_queue_.push({pos, torchlight_now});
					chk->SetTorchLight(related_pos, torchlight_now);
				}
			}
		}
	}
}

Block World::GetBlock(const glm::ivec3 &pos) const
{
	glm::ivec3 chunk_pos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chunk_pos);
	if(!chk)
		return Blocks::Air;
	return chk->GetBlock(pos - chunk_pos*CHUNK_SIZE);
}

LightLevel World::GetSunLight(const glm::ivec3 &pos) const
{
	glm::ivec3 chunk_pos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chunk_pos);
	if(pos.y < 0)
		return 0x0;
	else if(pos.y >= WORLD_HEIGHT_BLOCK || !chk)
		return 0xF;
	return chk->GetSunLight(pos - chunk_pos*CHUNK_SIZE);
}

void World::SetSunLight(const glm::ivec3 &pos, LightLevel val, bool check_update)
{
	glm::ivec3 chunk_pos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chunk_pos);
	if(!chk)
		return;
	chk->SetSunLight(pos - chunk_pos*CHUNK_SIZE, val);

	if(check_update)
		AddRelatedChunks(pos, mesh_threaded_update_set_);
}

LightLevel World::GetTorchLight(const glm::ivec3 &pos) const
{
	glm::ivec3 chunk_pos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chunk_pos);
	if(!chk)
		return 0;
	return chk->GetTorchLight(pos - chunk_pos*CHUNK_SIZE);
}

void World::SetTorchLight(const glm::ivec3 &pos, LightLevel val, bool check_update)
{
	glm::ivec3 chunk_pos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chunk_pos);
	if(!chk)
		return;
	chk->SetTorchLight(pos - chunk_pos*CHUNK_SIZE, val);

	if(check_update)
		AddRelatedChunks(pos, mesh_threaded_update_set_);
}

uint World::GetRunningThreadNum() const
{
	return running_threads_;
}
