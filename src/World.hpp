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
#include <atomic>
#include <functional>
#include <ThreadPool.h>

class World
{
private:
	int seed_;
	std::string name_;
	std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> chunk_map_;

	glm::ivec3 meshing_lookup_array_[27];

	//threading
	ThreadPool worker_pool_, copy_pool_;

	//loading
	std::unordered_map<glm::ivec2, std::unique_ptr<ChunkLoadingInfo>> loading_info_map_;
	std::vector<glm::ivec2> loading_vector_;
	//light initialize
	std::unordered_map<glm::ivec2, std::unique_ptr<ChunkLightingInfo>> lighting_info_map_;
	std::list<glm::ivec2> lighting_list_;
	std::vector<glm::ivec2> pre_lighting_vector_;
	//mesh initialize
	std::unordered_map<glm::ivec3, std::unique_ptr<ChunkMeshingInfo>> meshing_info_map_;
	std::list<glm::ivec3> meshing_list_;
	std::vector<glm::ivec3> pre_meshing_vector_;
	//mesh update
	std::unordered_map<glm::ivec3, std::unique_ptr<ChunkMeshingInfo>> mesh_update_info_map_;
	std::vector<glm::ivec3> mesh_update_vector_;

	std::vector<std::thread> threads_;
	std::condition_variable condition_var_;
	std::atomic_uint running_threads_;
	std::mutex mutex_;
	std::atomic_bool running_;

	//global flags
	glm::ivec3 center_;
	bool xz_pos_changed_, pos_changed_;

	//chunk update
	std::unordered_set<glm::ivec3> mesh_directly_update_set_, mesh_threaded_update_set_;
	std::queue<LightBFSNode> sun_light_queue_, sun_light_removal_queue_,
			torch_light_queue_, torch_light_removal_queue_;

	//day night cycle
	float initial_time_, timer_; //0.0f - 1.0f
	glm::mat4 sun_model_matrix_;
	glm::vec3 sun_position_;

	//data saving
	WorldData world_data_;

	Player player_;

	//render lists
	std::unordered_set<glm::ivec3> render_set_[2];
	std::vector<glm::ivec3> transparent_render_vector_;
	int render_order_update_counter_;

	friend class WorldData;

	void ProcessChunkUpdates();

	void UpdateChunkLoadingList();
	void UpdateChunkSunLightingList();
	void UpdateChunkMeshingList();

	void LoadingWorker();
	void LightingWorker();
	void MeshingWorker();
	void MeshUpdateWorker();

	inline bool cmp2_impl(const glm::ivec2 &l, const glm::ivec2 &r)
	{ return glm::length(glm::vec2(l) - glm::vec2(center_.x, center_.z)) < glm::length(glm::vec2(r) - glm::vec2(center_.x, center_.z)); }
	inline bool cmp3_impl(const glm::ivec3 &l, const glm::ivec3 &r)
	{ return glm::length(glm::vec3(l) - (glm::vec3)center_) < glm::length(glm::vec3(r) - (glm::vec3)center_); }
	inline bool rcmp2_impl(const glm::ivec2 &l, const glm::ivec2 &r)
	{ return glm::length(glm::vec2(l) - glm::vec2(center_.x, center_.z)) > glm::length(glm::vec2(r) - glm::vec2(center_.x, center_.z)); }
	inline bool rcmp3_impl(const glm::ivec3 &l, const glm::ivec3 &r)
	{ return glm::length(glm::vec3(l) - (glm::vec3)center_) > glm::length(glm::vec3(r) - (glm::vec3)center_); }
	std::function<bool(const glm::ivec2&, const glm::ivec2&)> cmp2, rcmp2;
	std::function<bool(const glm::ivec3&, const glm::ivec3&)> cmp3, rcmp3;


public:

	explicit World(const std::string &name);
	~World();

	inline void SetChunk(const glm::ivec3 &pos) { chunk_map_[pos] = std::make_unique<Chunk>(pos); }
	inline bool ChunkExist(const glm::ivec3 &pos) const { return (bool)chunk_map_.count(pos); }
	inline ChunkPtr GetChunk(const glm::ivec3 &pos) const
	{
		if(!ChunkExist(pos))
			return nullptr;
		return chunk_map_.at(pos).get();
	}
	inline void AddRelatedChunks(glm::ivec3 block_pos, std::unordered_set<glm::ivec3> &chunk_pos_set)
	{
		short size = 1, _size;
		glm::ivec3 arr[8], chunk_pos = BlockPosToChunkPos(block_pos), tmp;
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
	inline const std::string &GetName() const { return name_; }
	inline Player &GetPlayer() { return player_; }
	inline const std::unordered_set<glm::ivec3> &GetOpaqueRenderSet() const { return render_set_[0]; };
	inline const std::vector<glm::ivec3> &GetTransparentRenderVector() const { return transparent_render_vector_; }
};

#endif //WORLD_HPP
