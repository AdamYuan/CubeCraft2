//
// Created by adamyuan on 12/23/17.
//

#include "World.hpp"
#include "ChunkAlgorithm.hpp"
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

World::World(const std::string &name) : world_data_(name), player_(*this)
{
	running_threads_ = render_order_update_counter_ = 0;
	running_ = true;
	name_ = name;
	xz_pos_changed_ = pos_changed_ = false;
	center_ = glm::ivec3(INT_MAX);

	cmp2 = std::bind(&World::cmp2_impl, this, std::placeholders::_1, std::placeholders::_2);
	cmp3 = std::bind(&World::cmp3_impl, this, std::placeholders::_1, std::placeholders::_2);
	rcmp2 = std::bind(&World::rcmp2_impl, this, std::placeholders::_1, std::placeholders::_2);
	rcmp3 = std::bind(&World::rcmp3_impl, this, std::placeholders::_1, std::placeholders::_2);

	glm::ivec3 _;
	int index = 0;
	for(_.x = -1; _.x <= 1; ++_.x)
		for(_.y = -1; _.y <= 1; ++_.y)
			for(_.z = -1; _.z <= 1; ++_.z)
				meshing_lookup_array_[index++] = _;

	world_data_.LoadWorld(*this);

	const size_t _SIZE = ((size_t)Setting::ChunkLoadRange*2+1) * ((size_t)Setting::ChunkLoadRange*2+1) * WORLD_HEIGHT;
	pre_meshing_vector_.reserve(_SIZE);
	pre_lighting_vector_.reserve(_SIZE);

	for(unsigned i=0; i<Setting::LoadingThreadsNum; ++i)
	{
		threads_.emplace_back(&World::LoadingWorker, this);
		threads_.emplace_back(&World::LightingWorker, this);
		threads_.emplace_back(&World::MeshingWorker, this);
		threads_.emplace_back(&World::MeshUpdateWorker, this);
	}
}

World::~World()
{
	running_ = false;
	mutex_.unlock();
	condition_var_.notify_all();
	for(auto &i : threads_)
		i.join();

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

	{
		std::lock_guard<std::mutex> lk(mutex_);
		UpdateChunkLoadingList();
		UpdateChunkSunLightingList();
		UpdateChunkMeshingList();

		condition_var_.notify_all();
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
			ChunkAlgorithm::ApplyMesh(GetChunk(pos), mesh_vertices, mesh_indices);
			ChunkAlgorithm::ApplyMesh(GetChunk(pos), mesh_vertices, mesh_indices);
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
	for(auto iter = loading_info_map_.begin(); iter != loading_info_map_.end(); )
	{
		if(iter->second->done_)
		{
			glm::ivec3 pos(iter->first.x, 0, iter->first.y);
			if(ChunkExist(pos))
			{
				ChunkPtr arr[WORLD_HEIGHT];
				for(; pos.y < WORLD_HEIGHT; pos.y++)
					arr[pos.y] = GetChunk(pos);

				iter->second->ApplyTerrain(arr);
			}
			iter = loading_info_map_.erase(iter);
		}
		else
			++iter;
	}

	if(xz_pos_changed_)
	{
		glm::ivec2 iter;
		for(iter.x = center_.x - Setting::ChunkLoadRange; iter.x <= center_.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.y = center_.z - Setting::ChunkLoadRange; iter.y <= center_.z + Setting::ChunkLoadRange; ++iter.y)
			{
				if(!GetChunk({iter.x, 0, iter.y})->loaded_terrain_ && !loading_info_map_.count(iter))
				{
					loading_info_map_.emplace(iter, std::make_unique<ChunkLoadingInfo>(iter, seed_, world_data_));
					loading_vector_.push_back(iter);
				}
			}
		std::sort(loading_vector_.begin(), loading_vector_.end(), rcmp2);
	}
}

void World::UpdateChunkSunLightingList()
{
	for(auto iter = lighting_info_map_.begin(); iter != lighting_info_map_.end(); )
	{
		if(iter->second->done_)
		{
			glm::ivec3 pos(iter->first.x, 0, iter->first.y);
			if(ChunkExist(pos))
			{
				ChunkPtr arr[WORLD_HEIGHT];
				for(; pos.y<WORLD_HEIGHT; ++pos.y)
					arr[pos.y] = GetChunk(pos);

				iter->second->ApplyLighting(arr);

			}
			iter = lighting_info_map_.erase(iter);
		}
		else
			++iter;
	}

	if(xz_pos_changed_)
	{
		pre_lighting_vector_.clear();
		glm::ivec2 iter;
		for(iter.x = center_.x - Setting::ChunkLoadRange + 1; iter.x < center_.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.y = center_.z - Setting::ChunkLoadRange + 1; iter.y < center_.z + Setting::ChunkLoadRange; ++iter.y)
				if(!GetChunk({iter.x, 0, iter.y})->initialized_lighting_ && !lighting_info_map_.count(iter))
					pre_lighting_vector_.push_back(iter);

		lighting_list_.sort(rcmp2);
	}

	for(auto iter = pre_lighting_vector_.begin(); iter != pre_lighting_vector_.end(); )
	{
		glm::ivec3 i(iter->x, 0, iter->y);
		if(!GetChunk(i)->loaded_terrain_)
		{
			++iter;
			continue;
		}

		bool flag = true;
		ChunkPtr arr[WORLD_HEIGHT * 9];

		int ind = 0;
		for(i.x = iter->x-1; flag && i.x <= iter->x+1; ++i.x)
			for(i.z = iter->y-1; i.z <= iter->y+1; ++i.z) {
				i.y = 0;
				if (!GetChunk(i)->loaded_terrain_) {
					flag = false;
					break;
				}
				for (; i.y < WORLD_HEIGHT; i.y++) {
					arr[ind++] = GetChunk(i);
				}
			}

		if(flag)
		{
			auto pos = std::lower_bound(lighting_list_.begin(), lighting_list_.end(),
										*iter, rcmp2);
			lighting_list_.insert(pos, *iter);

			lighting_info_map_.emplace(*iter, std::make_unique<ChunkLightingInfo>(arr));

			iter = pre_lighting_vector_.erase(iter);
		}
		else
			++iter;
	}

}

void World::UpdateChunkMeshingList()
{
	//mesh initialize
	for(auto iter = meshing_info_map_.begin(); iter != meshing_info_map_.end(); )
	{
		if(iter->second->done_)
		{
			if(ChunkExist(iter->first))
			{
				ChunkPtr chk = GetChunk(iter->first);
				iter->second->ApplyMesh(chk);
				if(!chk->vertex_buffers_[0]->Empty())
					render_set_[0].insert(iter->first);
				if(!chk->vertex_buffers_[1]->Empty())
					render_set_[1].insert(iter->first);
			}
			iter = meshing_info_map_.erase(iter);
		}
		else
			++iter;
	}

	if(xz_pos_changed_)
	{
		pre_meshing_vector_.clear();

		glm::ivec3 iter;
		for(iter.x = center_.x - Setting::ChunkLoadRange + 1; iter.x < center_.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.z = center_.z - Setting::ChunkLoadRange + 1; iter.z < center_.z + Setting::ChunkLoadRange; ++iter.z)
				for(iter.y = 0; iter.y < WORLD_HEIGHT; ++iter.y)
					if(!GetChunk(iter)->initialized_mesh_ && !meshing_info_map_.count(iter))
						pre_meshing_vector_.push_back(iter);
	}
	if(pos_changed_)
		meshing_list_.sort(rcmp3);


	ChunkPtr neighbours[27] = {nullptr};
	for(auto iter = pre_meshing_vector_.begin(); iter != pre_meshing_vector_.end(); )
	{
		if(!GetChunk(*iter)->initialized_lighting_)
		{
			++iter;
			continue;
		}

		bool flag = true;
		for(int i=0; i<27; ++i)
		{
			neighbours[i] = GetChunk(*iter + meshing_lookup_array_[i]);
			//check that all the terrain are loaded
			if(neighbours[i] && (!neighbours[i]->loaded_terrain_ || !neighbours[i]->initialized_lighting_)) {
				flag = false;
				break;
			}
		}

		if(flag)
		{
			auto pos = std::lower_bound(meshing_list_.begin(), meshing_list_.end(), *iter, rcmp3);
			meshing_list_.insert(pos, *iter);
			meshing_info_map_.emplace(*iter, std::make_unique<ChunkMeshingInfo>(neighbours));

			iter = pre_meshing_vector_.erase(iter);
		}
		else
			++iter;
	}

	//chunk mesh update

	for(auto iter = mesh_update_info_map_.begin(); iter != mesh_update_info_map_.end(); )
	{
		if(iter->second->done_)
		{
			if(ChunkExist(iter->first))
			{
				ChunkPtr chk = GetChunk(iter->first);
				iter->second->ApplyMesh(chk);
				for(short t=0; t<=1; ++t)
				{
					if(chk->vertex_buffers_[t]->Empty())
						render_set_[t].erase(iter->first);
					else
						render_set_[t].insert(iter->first);
				}
			}
			iter = mesh_update_info_map_.erase(iter);
		}
		else
			++iter;
	}

	if(!mesh_threaded_update_set_.empty())
	{
		for(const glm::ivec3 &pos : mesh_threaded_update_set_)
		{
			if(!ChunkExist(pos))
				continue;
			if(!GetChunk(pos)->initialized_mesh_)
				continue;

			for(int i=0; i<27; ++i)
				neighbours[i] = GetChunk(pos + meshing_lookup_array_[i]);
			if(mesh_update_info_map_.count(pos))
				mesh_update_info_map_.erase(pos);
			else
				mesh_update_vector_.push_back(pos);
			mesh_update_info_map_[pos] = std::make_unique<ChunkMeshingInfo>(neighbours);
		}
		mesh_threaded_update_set_.clear();
	}
}


void World::LoadingWorker()
{
	glm::ivec2 pos;
	while(running_)
	{
		std::unique_lock<std::mutex> lk(mutex_);
		condition_var_.wait(lk, [this] { return !running_ || (running_threads_ < Setting::LoadingThreadsNum && !loading_vector_.empty());});
		if(!running_)
			return;
		pos = loading_vector_.back();
		loading_vector_.pop_back();
		lk.unlock();

		running_threads_ ++;
		loading_info_map_.at(pos)->Process();
		running_threads_ --;
	}
}

void World::LightingWorker()
{
	glm::ivec2 pos;
	while(running_)
	{
		std::unique_lock<std::mutex> lk(mutex_);
		condition_var_.wait(lk, [this]{return !running_ ||
											  (running_threads_ < Setting::LoadingThreadsNum && !lighting_list_.empty());});
		if(!running_)
			return;
		pos = lighting_list_.back();
		lighting_list_.pop_back();
		lk.unlock();

		running_threads_ ++;
		lighting_info_map_.at(pos)->Process();
		running_threads_ --;
	}
}

void World::MeshingWorker()
{
	glm::ivec3 pos;
	while(running_)
	{
		std::unique_lock<std::mutex> lk(mutex_);
		condition_var_.wait(lk, [this]{return !running_ ||
											  (running_threads_ < Setting::LoadingThreadsNum && !meshing_list_.empty());});
		if(!running_)
			return;
		pos = meshing_list_.back();
		meshing_list_.pop_back();
		lk.unlock();

		running_threads_ ++;
		meshing_info_map_.at(pos)->Process();
		running_threads_ --;
	}
}

void World::MeshUpdateWorker()
{
	glm::ivec3 pos;
	while(running_)
	{
		std::unique_lock<std::mutex> lk(mutex_);
		condition_var_.wait(lk, [this]{return !running_ ||
											  (running_threads_ < Setting::LoadingThreadsNum && !mesh_update_vector_.empty());});
		if(!running_)
			return;
		pos = mesh_update_vector_.back();
		mesh_update_vector_.pop_back();
		lk.unlock();

		running_threads_ ++;
		mesh_update_info_map_.at(pos)->Process();
		running_threads_ --;
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
