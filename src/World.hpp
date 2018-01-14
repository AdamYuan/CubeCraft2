//
// Created by adamyuan on 12/23/17.
//

#ifndef WORLD_HPP
#define WORLD_HPP

#include "Chunk.hpp"
#include "Setting.hpp"

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
	void ChunkMeshUpdateWorker();
	bool Running;

	//chunk update
	void ProcessChunkUpdates();
	std::unordered_set<glm::ivec3> MeshDirectlyUpdateSet, MeshThreadedUpdateSet;
	std::unordered_map<glm::ivec3, std::unique_ptr<ChunkMeshingInfo>> MeshUpdateInfoMap;
	std::vector<glm::ivec3> MeshUpdateVector;
	std::queue<LightBFSNode> SunLightQueue, SunLightRemovalQueue,
			TorchLightQueue, TorchLightRemovalQueue;

	//day night cycle
	float InitialTime, Time;
	//0.0f - 1024.0f(1.0f = 1 second)
	glm::mat4 SunModelMatrix;
	glm::vec3 SunPosition;

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
		glm::ivec3 arr[8];
		short size = 1, _size;

		glm::ivec3 cPos = BlockPosToChunkPos(bPos), _bPos = bPos - cPos * CHUNK_SIZE, tmp;
		arr[0] = cPos;
		for(short axis = 0; axis < 3; ++ axis)
			if(_bPos[axis] == 0)
			{
				_size = size;
				for(short i=0; i<_size; ++i)
				{
					tmp = arr[i];
					tmp[axis]--;
					arr[size++] = tmp;
				}
			}
			else if(_bPos[axis] == CHUNK_SIZE-1)
			{
				_size = size;
				for(short i=0; i<_size; ++i)
				{
					tmp = arr[i];
					tmp[axis]++;
					arr[size++] = tmp;
				}
			}
		for(short i=0; i<size; ++i)
			chunkPos.insert(arr[i]);
	}

	void Update(const glm::ivec3 &center);

	Block GetBlock(const glm::ivec3 &pos) const;
	void SetBlock(const glm::ivec3 &pos, Block blk, bool checkUpdate);

	LightLevel GetSunLight(const glm::ivec3 &pos) const;
	void SetSunLight(const glm::ivec3 &pos, LightLevel val, bool checkUpdate);

	LightLevel GetTorchLight(const glm::ivec3 &pos) const;
	void SetTorchLight(const glm::ivec3 &pos, LightLevel val, bool checkUpdate);

	uint GetRunningThreadNum() const;

	static inline glm::ivec3 BlockPosToChunkPos(const glm::ivec3 &pos)
	{
		return glm::ivec3((pos.x + (pos.x < 0)) / CHUNK_SIZE - (pos.x < 0),
						  (pos.y + (pos.y < 0)) / CHUNK_SIZE - (pos.y < 0),
						  (pos.z + (pos.z < 0)) / CHUNK_SIZE - (pos.z < 0));
	}

	std::unordered_set<glm::ivec3> RenderSet;

	inline float GetTime() const
	{
		return Time / DAY_TIME;
	}
	inline float GetDayLight() const
	{
		return glm::clamp(SunPosition.y * 0.96f + 0.6f, 0.02f, 1.0f);
	}
	inline glm::mat4 GetSunModelMatrix() const
	{ return SunModelMatrix; }
};


#endif //WORLD_HPP
