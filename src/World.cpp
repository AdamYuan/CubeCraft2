//
// Created by adamyuan on 12/23/17.
//

#include "World.hpp"
#include "Resource.hpp"
#include <glm/gtx/string_cast.hpp>

glm::ivec3 World::s_center;
bool World::s_meshChanged;

World::World() : Running(true), ThreadsSupport(std::thread::hardware_concurrency() - 1),
				 RunningThreads(0)
{
	RenderVector.reserve((CHUNK_LOADING_RANGE*2+1)*(CHUNK_LOADING_RANGE*2+1)*WORLD_HEIGHT);
	std::cout << "Max thread support: " << ThreadsSupport << std::endl;
	for(unsigned i=0; i<ThreadsSupport; ++i)
	{
		Threads.emplace_back(&World::ChunkLoadingWorker, this);
		Threads.emplace_back(&World::ChunkMeshingWorker, this);
		Threads.emplace_back(&World::ChunkInitialLightingWorker, this);
	}
}

World::~World()
{
	Running = false;
	Mutex.unlock();
	Cond.notify_all();
	for(auto &i : Threads)
		i.join();

	SuperChunk.clear();
}

void World::SetChunk(const glm::ivec3 &pos)
{
	SuperChunk[pos] = std::make_unique<Chunk>(pos);
}

ChunkPtr World::GetChunk(const glm::ivec3 &pos)
{
	if(!SuperChunk.count(pos)) {
		//std::cout << "Accessing Undefined Chunk: " << glm::to_string(pos) << std::endl;
		return nullptr;
	}
	return SuperChunk[pos].get();
}

void World::Update(const glm::ivec3 &center)
{
	s_meshChanged = false;
	s_center = center;
	//remove all chunks out of the range
	for(auto iter = SuperChunk.begin(); iter != SuperChunk.end(); ) {
		if (iter->first.x < center.x - CHUNK_DELETING_RANGE ||
			iter->first.x > center.x + CHUNK_DELETING_RANGE ||
			iter->first.z < center.z - CHUNK_DELETING_RANGE ||
			iter->first.z > center.z + CHUNK_DELETING_RANGE)
			iter = SuperChunk.erase(iter);
		else
			++iter;
	}

	//generate chunks in the range
	glm::ivec3 i;
	for(i.x = center.x - CHUNK_LOADING_RANGE; i.x <= center.x + CHUNK_LOADING_RANGE; ++i.x)
		for(i.z = center.z - CHUNK_LOADING_RANGE; i.z <= center.z + CHUNK_LOADING_RANGE; ++i.z)
			for(i.y = 0; i.y < WORLD_HEIGHT; ++i.y)
				if(!SuperChunk.count(i))
					SetChunk(i);

	//set lock
	if(Mutex.try_lock())
	{
		UpdateChunkLoadingList();
		UpdateChunkSunLightingList();
		UpdateChunkMeshingList();

		for(unsigned _=0; _<ThreadsSupport; ++_)
			Cond.notify_one();

		Mutex.unlock();
	}

	//update render list
	if(s_meshChanged)
	{
		RenderVector.clear();
		for(const auto &chk : SuperChunk)
		{
			if(!chk.second->VertexBuffer->Empty() &&
					glm::distance((glm::vec3)chk.second->Position, (glm::vec3)center) < CHUNK_LOADING_RANGE + 1.0f)
				RenderVector.push_back(chk.first);
		}
	}
}

void World::UpdateChunkLoadingList()
{
	for(auto iter = LoadingInfoMap.begin(); iter != LoadingInfoMap.end(); )
	{
		if(iter->second->Done)
		{
			glm::ivec3 pos(iter->first.x, 0, iter->first.y);
			if(SuperChunk.count(pos))
			{
				ChunkPtr arr[WORLD_HEIGHT];
				for(; pos.y < WORLD_HEIGHT; pos.y++)
					arr[pos.y] = GetChunk(pos);

				iter->second->ApplyTerrain(arr);
			}
			iter = LoadingInfoMap.erase(iter);
		}
		else
			++iter;
	}

	glm::ivec2 iter;
	for(iter.x = s_center.x - CHUNK_LOADING_RANGE; iter.x <= s_center.x + CHUNK_LOADING_RANGE; ++iter.x)
		for(iter.y = s_center.z - CHUNK_LOADING_RANGE; iter.y <= s_center.z + CHUNK_LOADING_RANGE; ++iter.y)
		{
			if(!GetChunk({iter.x, 0, iter.y})->LoadedTerrain && !LoadingInfoMap.count(iter))
			{
				LoadingInfoMap[iter] = std::make_unique<ChunkLoadingInfo>(iter);
				LoadingVector.push_back(iter);
			}
		}
	std::sort(LoadingVector.begin(), LoadingVector.end(), cmp2);
}

void World::UpdateChunkSunLightingList()
{
	for(auto iter = InitialLightingInfoMap.begin(); iter != InitialLightingInfoMap.end(); )
	{
		if(iter->second->Done)
		{
			glm::ivec3 pos(iter->first.x, 0, iter->first.y);
			if(SuperChunk.count(pos))
			{
				ChunkPtr arr[WORLD_HEIGHT];
				for(; pos.y<WORLD_HEIGHT; ++pos.y)
					arr[pos.y] = GetChunk(pos);

				iter->second->ApplySunLight(arr);
			}
			iter = InitialLightingInfoMap.erase(iter);
		}
		else
			++iter;
	}

	glm::ivec2 iter;
	for(iter.x = s_center.x - CHUNK_LOADING_RANGE + 1; iter.x < s_center.x + CHUNK_LOADING_RANGE; ++iter.x)
		for(iter.y = s_center.z - CHUNK_LOADING_RANGE + 1; iter.y < s_center.z + CHUNK_LOADING_RANGE; ++iter.y)
		{
			glm::ivec3 i(iter.x, 0, iter.y);
			if(!GetChunk(i)->FirstSunLighted && !InitialLightingInfoMap.count(iter))
			{
				bool flag = true;
				ChunkPtr arr[WORLD_HEIGHT * 9];

				int ind = 0;
				for(i.x = iter.x-1; flag && i.x <= iter.x+1; ++i.x)
					for(i.z = iter.y-1; i.z <= iter.y+1; ++i.z) {
						i.y = 0;
						if (!GetChunk(i)->LoadedTerrain) {
							flag = false;
							break;
						}
						for (; i.y < WORLD_HEIGHT; i.y++) {
							arr[ind++] = GetChunk(i);
						}
					}

				if(flag)
				{
					InitialLightingVector.push_back(iter);
					InitialLightingInfoMap[iter] = std::make_unique<ChunkInitialLightingInfo>(arr);
				}
			}
		}

	std::sort(InitialLightingVector.begin(), InitialLightingVector.end(), cmp2);
}

void World::UpdateChunkMeshingList()
{
	for(auto iter = MeshingInfoMap.begin(); iter != MeshingInfoMap.end(); )
	{
		if(iter->second->Done)
		{
			if(SuperChunk.count(iter->first))
			{
				iter->second->ApplyMesh(GetChunk(iter->first));
				s_meshChanged = true;
				//std::cout << "Meshed: " << glm::to_string(iter->first) << std::endl;
			}
			iter = MeshingInfoMap.erase(iter);
		}
		else
			++iter;
	}

	glm::ivec3 pos, lookUp[27];
	int index = 0;
	for(pos.x = -1; pos.x <= 1; ++pos.x)
		for(pos.y = -1; pos.y <= 1; ++pos.y)
			for(pos.z = -1; pos.z <= 1; ++pos.z)
				lookUp[index++] = pos;

	ChunkPtr neighbours[27] = {nullptr};

	for(pos.x = s_center.x - CHUNK_LOADING_RANGE + 1; pos.x < s_center.x + CHUNK_LOADING_RANGE; ++pos.x)
		for(pos.z = s_center.z - CHUNK_LOADING_RANGE + 1; pos.z < s_center.z + CHUNK_LOADING_RANGE; ++pos.z)
			for(pos.y = 0; pos.y < WORLD_HEIGHT; ++pos.y)
			{
				if(GetChunk(pos)->FirstSunLighted && !GetChunk(pos)->Meshed && !MeshingInfoMap.count(pos))
				{
					bool flag = true;
					for(int i=0; i<27; ++i)
					{
						neighbours[i] = GetChunk(pos + lookUp[i]);
						//check that all the terrain are loaded
						if(neighbours[i] && (!neighbours[i]->LoadedTerrain || !neighbours[i]->FirstSunLighted))
							flag = false;
					}

					if(flag)
					{
						MeshingVector.push_back(pos);
						MeshingInfoMap[pos] = std::make_unique<ChunkMeshingInfo>(neighbours);
					}
				}
			}

	std::sort(MeshingVector.begin(), MeshingVector.end(), cmp3);
}

bool World::cmp2(const glm::ivec2 &l, const glm::ivec2 &r)
{
	return glm::length((glm::vec2)l - glm::vec2(s_center.x, s_center.z)) >
		   glm::length((glm::vec2)r - glm::vec2(s_center.x, s_center.z));
}
bool World::cmp3(const glm::ivec3 &l, const glm::ivec3 &r)
{
	return glm::length((glm::vec3)l - (glm::vec3)s_center) > glm::length((glm::vec3)r - (glm::vec3)s_center);
}

void World::ChunkLoadingWorker()
{
	while(Running)
	{
		glm::ivec2 pos;

		std::unique_lock<std::mutex> lk(Mutex);
		Cond.wait(lk, [this]{return !Running ||
				(RunningThreads < ThreadsSupport && !LoadingVector.empty());});
		if(!Running)
			return;
		pos = LoadingVector.back();
		LoadingVector.pop_back();
		lk.unlock();

		RunningThreads ++;
		LoadingInfoMap[pos]->Process();
		RunningThreads --;
	}
}

void World::ChunkInitialLightingWorker()
{
	while(Running)
	{
		glm::ivec2 pos;

		std::unique_lock<std::mutex> lk(Mutex);
		Cond.wait(lk, [this]{return !Running ||
				(RunningThreads < ThreadsSupport && !InitialLightingVector.empty());});
		if(!Running)
			return;
		pos = InitialLightingVector.back();
		InitialLightingVector.pop_back();
		lk.unlock();

		RunningThreads ++;

		InitialLightingInfoMap[pos]->Process();

		RunningThreads --;
	}
}

void World::ChunkMeshingWorker()
{
	while(Running)
	{
		glm::ivec3 pos;

		std::unique_lock<std::mutex> lk(Mutex);
		Cond.wait(lk, [this]{return !Running ||
				(RunningThreads < ThreadsSupport && !MeshingVector.empty());});
		if(!Running)
			return;
		pos = MeshingVector.back();
		MeshingVector.pop_back();
		lk.unlock();

		RunningThreads ++;

		MeshingInfoMap[pos]->Process();

		RunningThreads --;
	}
}

void World::Render(const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &position)
{
	glm::mat4 matrix = projection * view;
	frustum.CalculatePlanes(matrix);

	static constexpr float range = CHUNK_SIZE*CHUNK_LOADING_RANGE;
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	Resource::ChunkShader->Use();

	//texture
	glActiveTexture(GL_TEXTURE0);
	Resource::ChunkTexture->Bind();

	Resource::ChunkShader->PassInt(Resource::UNIF_SAMPLER, 0);

	Resource::ChunkShader->PassFloat("viewDistance", range);
	Resource::ChunkShader->PassMat4("matrix", matrix);
	Resource::ChunkShader->PassVec3("camera", position);

	for(const glm::ivec3 &pos : RenderVector)
	{
		glm::vec3 center((glm::vec3)(pos * CHUNK_SIZE) + glm::vec3(CHUNK_SIZE/2));
		ChunkPtr chk = GetChunk(pos);
		if (chk && frustum.CubeInFrustum(center, CHUNK_SIZE/2))
			chk->VertexBuffer->Render(GL_TRIANGLES);
	}

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}
