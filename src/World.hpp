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
	int Seed;
	std::string WorldName;
	std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> SuperChunk;

	ThreadPool threadPool;

	int LoadingThreadNum, LightingThreadNum, MeshingThreadNum;
	std::unordered_set<glm::ivec2> LoadingInfoSet;
	std::vector<glm::ivec2> PreLoadingVector;

	std::unordered_set<glm::ivec2> InitialLightingInfoSet;
	std::vector<glm::ivec2> PreInitialLightingVector;

	glm::ivec3 MeshingLookup[27];
	std::unordered_set<glm::ivec3> MeshingInfoSet;
	std::vector<glm::ivec3> PreMeshingVector;

	std::unordered_set<glm::ivec3> MeshUpdateInfoSet;

	void UpdateChunkLoadingList();
	void UpdateChunkSunLightingList();
	void UpdateChunkMeshingList();

	glm::ivec3 LastCenter, Center;
	bool PosChanged;

	inline bool cmp2_impl(const glm::ivec2 &l, const glm::ivec2 &r)
	{ return glm::length((glm::vec2)l - glm::vec2(Center.x, Center.z)) < glm::length((glm::vec2)r - glm::vec2(Center.x, Center.z)); }
	inline bool cmp3_impl(const glm::ivec3 &l, const glm::ivec3 &r)
	{ return glm::length((glm::vec3)l - (glm::vec3)Center) < glm::length((glm::vec3)r - (glm::vec3)Center); }
	std::function<bool(const glm::ivec2&, const glm::ivec2&)> cmp2;
	std::function<bool(const glm::ivec3&, const glm::ivec3&)> cmp3;

	//multi threading stuff
	std::atomic_uint RunningThreads;
	std::mutex Mutex;
	void ChunkLoadingWorker(const glm::ivec2 &pos);
	void ChunkInitialLightingWorker(const glm::ivec2 &pos);
	void ChunkMeshingWorker(const glm::ivec3 &pos);
	void ChunkMeshUpdateWorker(const glm::ivec3 &pos);

	//chunk update
	void ProcessChunkUpdates();
	std::unordered_set<glm::ivec3> MeshDirectlyUpdateSet, MeshThreadedUpdateSet;
	std::queue<LightBFSNode> SunLightQueue, SunLightRemovalQueue,
			TorchLightQueue, TorchLightRemovalQueue;

	//day night cycle
	float InitialTime, Timer; //0 - 1
	glm::mat4 SunModelMatrix;
	glm::vec3 SunPosition;

	//data saving
	WorldData Database;

	friend class WorldData;

public:
	Player player;

	explicit World(const std::string &name);
	~World();

	inline void SetChunk(const glm::ivec3 &pos) { SuperChunk[pos] = std::make_unique<Chunk>(pos); }
	inline bool ChunkExist(const glm::ivec3 &pos) const { return static_cast<bool>(SuperChunk.count(pos)); }
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
				for(short i=0; i<_size; ++i) { tmp = arr[i]; tmp[axis]--; arr[size++] = tmp; }
			}
			else if(_bPos[axis] == CHUNK_SIZE-1)
			{
				_size = size;
				for(short i=0; i<_size; ++i) { tmp = arr[i]; tmp[axis]++; arr[size++] = tmp; }
			}
		for(short i=0; i<size; ++i) chunkPos.insert(arr[i]);
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

	std::unordered_set<glm::ivec3> RenderSet, RenderAdditionSet, RenderRemovalSet;
	//these two are for multi-threading

	inline float GetDayTime() const { return Timer; }
	inline float GetDayLight() const { return glm::clamp(SunPosition.y * 0.96f + 0.6f, 0.2f, 1.0f); }
	inline glm::mat4 GetSunModelMatrix() const { return SunModelMatrix; }
	inline int GetSeed() const { return Seed; }
	inline std::string GetName() const { return WorldName; }
};


#endif //WORLD_HPP
