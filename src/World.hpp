//
// Created by adamyuan on 12/23/17.
//

#ifndef WORLD_HPP
#define WORLD_HPP

#include "Chunk.hpp"
#include "Settings.hpp"

#include <MyGL/Frustum.hpp>

#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <condition_variable>
#include <atomic>

class World
{
private:
	std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> SuperChunk;

	std::unordered_map<glm::ivec2, std::unique_ptr<ChunkLoadingInfo>> LoadingInfoMap;
	std::vector<glm::ivec2> LoadingVector;

	std::unordered_map<glm::ivec2, std::unique_ptr<ChunkInitialLightingInfo>> InitialLightingInfoMap;
	std::vector<glm::ivec2> InitialLightingVector;

	std::unordered_map<glm::ivec3, std::unique_ptr<ChunkMeshingInfo>> MeshingInfoMap;
	std::vector<glm::ivec3> MeshingVector;

	void UpdateChunkLoadingList();
	void UpdateChunkSunLightingList();
	void UpdateChunkMeshingList();

	static glm::ivec3 s_center;
	static bool s_meshChanged;
	inline static bool cmp2(const glm::ivec2 &l, const glm::ivec2 &r);
	inline static bool cmp3(const glm::ivec3 &l, const glm::ivec3 &r);

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

	MyGL::Frustum frustum;

	std::vector<glm::ivec3> RenderVector;

public:
	World();
	~World();

	void SetChunk(const glm::ivec3 &pos);
	inline ChunkPtr GetChunk(const glm::ivec3 &pos);

	void Update(const glm::ivec3 &center);

	void Render(const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &position);
};


#endif //WORLD_HPP
