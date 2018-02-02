//
// Created by adamyuan on 12/23/17.
//

#ifndef WORLD_HPP
#define WORLD_HPP

#include "WorldData.hpp"
#include "Chunk.hpp"
#include "Setting.hpp"
#include "Player.hpp"

#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>
#include <atomic>
#include <functional>

#include <ThreadPool.h>

class World
{
private:
	int seed_;
	std::string name_;
	std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> chunk_map_;

	ThreadPool thread_pool_;

	int loading_thread_num_, lighting_thread_num_, meshing_thread_num_;
	std::unordered_set<glm::ivec2> loading_info_set_;
	std::vector<glm::ivec2> pre_loading_vector_;

	std::unordered_set<glm::ivec2> initial_lighting_info_set_;
	std::vector<glm::ivec2> pre_initial_lighting_vector_;

	glm::ivec3 meshing_lookup_array_[27];
	std::unordered_set<glm::ivec3> meshing_info_set_;
	std::vector<glm::ivec3> pre_meshing_vector_;

	std::unordered_set<glm::ivec3> mesh_update_info_set_;

	void UpdateChunkLoadingList();
	void UpdateChunkSunLightingList();
	void UpdateChunkMeshingList();

	glm::ivec3 last_center_, center_;
	bool pos_changed_;

	inline bool cmp2_impl(const glm::ivec2 &l, const glm::ivec2 &r)
	{ return glm::length((glm::vec2)l - glm::vec2(center_.x, center_.z)) < glm::length((glm::vec2)r - glm::vec2(center_.x, center_.z)); }
	inline bool cmp3_impl(const glm::ivec3 &l, const glm::ivec3 &r)
	{ return glm::length((glm::vec3)l - (glm::vec3)center_) < glm::length((glm::vec3)r - (glm::vec3)center_); }
	std::function<bool(const glm::ivec2&, const glm::ivec2&)> cmp2;
	std::function<bool(const glm::ivec3&, const glm::ivec3&)> cmp3;

	//multi threading stuff
	std::atomic_uint running_threads_;
	std::mutex mutex_;
	void ChunkLoadingWorker(const glm::ivec2 &pos);
	void ChunkInitialLightingWorker(const glm::ivec2 &pos);
	void ChunkMeshingWorker(const glm::ivec3 &pos);
	void ChunkMeshUpdateWorker(const glm::ivec3 &pos);

	//chunk update
	void ProcessChunkUpdates();
	std::unordered_set<glm::ivec3> mesh_directly_update_set_, mesh_threaded_update_set_;
	std::queue<LightBFSNode> sun_light_queue_, sun_light_removal_queue_,
			torch_light_queue_, torch_light_removal_queue_;

	//day night cycle
	float initial_time_, timer_; //0.0f - 1.0f
	glm::mat4 sun_model_matrix_;
	glm::vec3 sun_position_;

	//data saving
	WorldData world_data_;

	friend class WorldData;

public:
	Player player_;
	std::unordered_set<glm::ivec3> render_set_[2], render_addition_set_[2], render_removal_set_[2];
	//these two are for multi-threading

	explicit World(const std::string &name);
	~World();

	inline void SetChunk(const glm::ivec3 &pos) { chunk_map_[pos] = std::make_unique<Chunk>(pos); }
	inline bool ChunkExist(const glm::ivec3 &pos) const { return static_cast<bool>(chunk_map_.count(pos)); }
	inline ChunkPtr GetChunk(const glm::ivec3 &pos) const
	{
		if(!ChunkExist(pos))
			return nullptr;
		return chunk_map_.at(pos).get();
	}
	inline void AddRelatedChunks(glm::ivec3 block_pos, std::unordered_set<glm::ivec3> &chunk_pos_set)
	{
		glm::ivec3 arr[8];
		short size = 1, _size;

		glm::ivec3 chunk_pos = BlockPosToChunkPos(block_pos), tmp;
		block_pos -= chunk_pos * CHUNK_SIZE;
		arr[0] = chunk_pos;
		for(short axis = 0; axis < 3; ++ axis)
			if(block_pos[axis] == 0)
			{
				_size = size;
				for(short i=0; i<_size; ++i) { tmp = arr[i]; tmp[axis]--; arr[size++] = tmp; }
			}
			else if(block_pos[axis] == CHUNK_SIZE-1)
			{
				_size = size;
				for(short i=0; i<_size; ++i) { tmp = arr[i]; tmp[axis]++; arr[size++] = tmp; }
			}
		for(short i=0; i<size; ++i) chunk_pos_set.insert(arr[i]);
	}

	void Update(const glm::ivec3 &center);

	Block GetBlock(const glm::ivec3 &pos) const;
	void SetBlock(const glm::ivec3 &pos, Block blk, bool check_update);

	LightLevel GetSunLight(const glm::ivec3 &pos) const;
	void SetSunLight(const glm::ivec3 &pos, LightLevel val, bool check_update);

	LightLevel GetTorchLight(const glm::ivec3 &pos) const;
	void SetTorchLight(const glm::ivec3 &pos, LightLevel val, bool check_update);

	uint GetRunningThreadNum() const;

	static inline glm::ivec3 BlockPosToChunkPos(const glm::ivec3 &pos)
	{
		return glm::ivec3((pos.x + (pos.x < 0)) / CHUNK_SIZE - (pos.x < 0),
						  (pos.y + (pos.y < 0)) / CHUNK_SIZE - (pos.y < 0),
						  (pos.z + (pos.z < 0)) / CHUNK_SIZE - (pos.z < 0));
	}


	inline float GetDayTime() const { return timer_; }
	inline float GetDayLight() const { return glm::clamp(sun_position_.y * 0.96f + 0.6f, 0.2f, 1.1f); }
	inline glm::mat4 GetSunModelMatrix() const { return sun_model_matrix_; }
	inline int GetSeed() const { return seed_; }
	inline std::string GetName() const { return name_; }
};


#endif //WORLD_HPP
