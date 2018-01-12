//
// Created by adamyuan on 12/23/17.
//

#ifndef WORLD_HPP
#define WORLD_HPP

#include "Chunk.hpp"
#include "Settings.hpp"

#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>
#include <atomic>

class World
{
private:
	std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> SuperChunk;

	std::unordered_map<glm::ivec2, std::unique_ptr<ChunkLoadingInfo>> LoadingInfoMap;
	std::vector<glm::ivec2> LoadingVector;

	std::unordered_map<glm::ivec2, std::unique_ptr<ChunkInitialLightingInfo>> InitialLightingInfoMap;
	std::list<glm::ivec2> InitialLightingList;
	std::vector<glm::ivec2> PreInitialLightingVector;

	std::unordered_map<glm::ivec3, std::unique_ptr<ChunkMeshingInfo>> MeshingInfoMap;
	std::list<glm::ivec3> MeshingList;
	std::vector<glm::ivec3> PreMeshingVector;

	void UpdateChunkLoadingList();
	void UpdateChunkSunLightingList();
	void UpdateChunkMeshingList();

	static glm::ivec3 s_center;
	bool PosChanged;

	inline static bool cmp2(const glm::ivec2 &l, const glm::ivec2 &r)
	{
		return glm::length((glm::vec2)l - glm::vec2(s_center.x, s_center.z)) >
			   glm::length((glm::vec2)r - glm::vec2(s_center.x, s_center.z));
	}
	inline static bool cmp3(const glm::ivec3 &l, const glm::ivec3 &r)
	{
		return glm::length((glm::vec3)l - (glm::vec3)s_center) > glm::length((glm::vec3)r - (glm::vec3)s_center);
	}

	//multi threading stuff
	unsigned ThreadsSupport;

	std::atomic_uint RunningThreads;

	std::vector<std::thread> Threads;
	std::mutex Mutex;
	std::condition_variable Cond;
	void ChunkLoadingWorker();
	void ChunkInitialLightingWorker();
	void ChunkMeshingWorker();
	bool Running;

	//chunk update
	void ProcessChunkUpdates();
	std::unordered_set<glm::ivec3> MeshUpdateSet;
	std::queue<LightBFSNode> SunLightQueue, SunLightRemovalQueue;

public:
	World();
	~World();

	inline void SetChunk(const glm::ivec3 &pos)
	{ SuperChunk[pos] = std::make_unique<Chunk>(pos); }
	inline bool ChunkExist(const glm::ivec3 &pos) const
	{ return static_cast<bool>(SuperChunk.count(pos)); }
	inline ChunkPtr GetChunk(const glm::ivec3 &pos) const
	{
		if(!ChunkExist(pos))
			return nullptr;
		return SuperChunk.at(pos).get();
	}
	inline void AddRelatedChunks(const glm::ivec3 &bPos, std::unordered_set<glm::ivec3> &chunkPos)
	{
		std::vector<glm::ivec3> _tmp;
		glm::ivec3 cPos = BlockPosToChunkPos(bPos), _bPos = bPos - cPos * CHUNK_SIZE;
		_tmp.push_back(cPos);
		for(short axis = 0; axis < 3; ++ axis)
			if(_bPos[axis] == 0)
				for(glm::ivec3 i : _tmp)
				{
					i[axis]--;
					_tmp.push_back(i);
				}
			else if(_bPos[axis] == CHUNK_SIZE-1)
				for(glm::ivec3 i : _tmp)
				{
					i[axis]++;
					_tmp.push_back(i);
				}
		for(const glm::ivec3 &i : _tmp) chunkPos.insert(i);
	}

	void Update(const glm::ivec3 &center);

	Block GetBlock(const glm::ivec3 &pos) const;
	void SetBlock(const glm::ivec3 &pos, Block blk, bool checkUpdate);

	DLightLevel GetLight(const glm::ivec3 &pos) const;
	void SetLight(const glm::ivec3 &pos, DLightLevel val, bool checkUpdate);

	uint GetRunningThreadNum() const;

	static inline glm::ivec3 BlockPosToChunkPos(const glm::ivec3 &pos)
	{
		return glm::ivec3((pos.x + (pos.x < 0)) / CHUNK_SIZE - (pos.x < 0),
						  (pos.y + (pos.y < 0)) / CHUNK_SIZE - (pos.y < 0),
						  (pos.z + (pos.z < 0)) / CHUNK_SIZE - (pos.z < 0));
	}

	std::unordered_set<glm::ivec3> RenderSet;
};


#endif //WORLD_HPP
