//
// Created by adamyuan on 12/23/17.
//

#include "World.hpp"
#include "ChunkAlgorithm.hpp"
#include <glm/gtx/string_cast.hpp>

glm::ivec3 World::s_center;

World::World() : Running(true), ThreadsSupport(std::max(1u, std::thread::hardware_concurrency() - 1)),
				 RunningThreads(0), PosChanged(false)
{
	constexpr size_t _SIZE = (CHUNK_LOADING_RANGE*2+1) * (CHUNK_LOADING_RANGE*2+1) * WORLD_HEIGHT;
	LoadingVector.reserve(_SIZE);
	PreMeshingVector.reserve(_SIZE);
	PreInitialLightingVector.reserve(_SIZE);
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



void World::Update(const glm::ivec3 &center)
{
	//global flags
	PosChanged = false;
	s_center = center;

	static glm::ivec3 s_lastCenter = glm::ivec3(INT_MAX);

	if(center.x != s_lastCenter.x || center.z != s_lastCenter.z)
	{
		PosChanged = true;
		s_lastCenter = center;
	}

	if(PosChanged)
	{
		//remove all chunks out of the range
		for(auto iter = SuperChunk.begin(); iter != SuperChunk.end(); ) {
			if (iter->first.x < center.x - CHUNK_DELETING_RANGE ||
				iter->first.x > center.x + CHUNK_DELETING_RANGE ||
				iter->first.z < center.z - CHUNK_DELETING_RANGE ||
				iter->first.z > center.z + CHUNK_DELETING_RANGE)
			{
				RenderSet.erase(iter->first);
				iter = SuperChunk.erase(iter);
			}
			else
				++iter;
		}
		//generate chunks in the range
		glm::ivec3 i;
		for(i.x = center.x - CHUNK_LOADING_RANGE; i.x <= center.x + CHUNK_LOADING_RANGE; ++i.x)
			for(i.z = center.z - CHUNK_LOADING_RANGE; i.z <= center.z + CHUNK_LOADING_RANGE; ++i.z)
				for(i.y = 0; i.y < WORLD_HEIGHT; ++i.y)
					if(!ChunkExist(i))
						SetChunk(i);
	}

	//set lock
	if(Mutex.try_lock())
	{
		UpdateChunkLoadingList();
		UpdateChunkSunLightingList();
		UpdateChunkMeshingList();

		Cond.notify_all();

		Mutex.unlock();
	}
}

void World::UpdateChunkLoadingList()
{
	for(auto iter = LoadingInfoMap.begin(); iter != LoadingInfoMap.end(); )
	{
		if(iter->second->Done)
		{
			glm::ivec3 pos(iter->first.x, 0, iter->first.y);
			if(ChunkExist(pos))
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

	if(PosChanged)
	{
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
}

void World::UpdateChunkSunLightingList()
{
	for(auto iter = InitialLightingInfoMap.begin(); iter != InitialLightingInfoMap.end(); )
	{
		if(iter->second->Done)
		{
			glm::ivec3 pos(iter->first.x, 0, iter->first.y);
			if(ChunkExist(pos))
			{
				ChunkPtr arr[WORLD_HEIGHT];
				for(; pos.y<WORLD_HEIGHT; ++pos.y)
					arr[pos.y] = GetChunk(pos);

				iter->second->ApplyLighting(arr);

			}
			iter = InitialLightingInfoMap.erase(iter);
		}
		else
			++iter;
	}

	if(PosChanged)
	{
		PreInitialLightingVector.clear();
		glm::ivec2 iter;
		for(iter.x = s_center.x - CHUNK_LOADING_RANGE + 1; iter.x < s_center.x + CHUNK_LOADING_RANGE; ++iter.x)
			for(iter.y = s_center.z - CHUNK_LOADING_RANGE + 1; iter.y < s_center.z + CHUNK_LOADING_RANGE; ++iter.y)
				if(!GetChunk({iter.x, 0, iter.y})->InitializedLighting && !InitialLightingInfoMap.count(iter))
					PreInitialLightingVector.push_back(iter);

		InitialLightingList.sort(cmp2);
	}

	for(auto iter = PreInitialLightingVector.begin(); iter != PreInitialLightingVector.end(); )
	{
		glm::ivec3 i(iter->x, 0, iter->y);
		if(!GetChunk(i)->LoadedTerrain)
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
			auto pos = std::lower_bound(InitialLightingList.begin(), InitialLightingList.end(),
										*iter, cmp2);
			InitialLightingList.insert(pos, *iter);

			InitialLightingInfoMap[*iter] = std::make_unique<ChunkInitialLightingInfo>(arr);

			iter = PreInitialLightingVector.erase(iter);
		}
		else
			++iter;
	}

}

void World::UpdateChunkMeshingList()
{
	for(auto iter = MeshingInfoMap.begin(); iter != MeshingInfoMap.end(); )
	{
		if(iter->second->Done)
		{
			if(ChunkExist(iter->first))
			{
				iter->second->ApplyMesh(GetChunk(iter->first));
				if(!GetChunk(iter->first)->VertexBuffer->Empty())
					RenderSet.insert(iter->first);
			}
			iter = MeshingInfoMap.erase(iter);
		}
		else
			++iter;
	}

	if(PosChanged)
	{
		PreMeshingVector.clear();

		glm::ivec3 iter;
		for(iter.x = s_center.x - CHUNK_LOADING_RANGE + 1; iter.x < s_center.x + CHUNK_LOADING_RANGE; ++iter.x)
			for(iter.z = s_center.z - CHUNK_LOADING_RANGE + 1; iter.z < s_center.z + CHUNK_LOADING_RANGE; ++iter.z)
				for(iter.y = 0; iter.y < WORLD_HEIGHT; ++iter.y)
					if(!GetChunk(iter)->InitializedMesh && !MeshingInfoMap.count(iter))
						PreMeshingVector.push_back(iter);

		MeshingList.sort(cmp3);
	}

	glm::ivec3 _, lookUp[27];
	int index = 0;
	for(_.x = -1; _.x <= 1; ++_.x)
		for(_.y = -1; _.y <= 1; ++_.y)
			for(_.z = -1; _.z <= 1; ++_.z)
				lookUp[index++] = _;
	ChunkPtr neighbours[27] = {nullptr};
	for(auto iter = PreMeshingVector.begin(); iter != PreMeshingVector.end(); )
	{
		if(!GetChunk(*iter)->InitializedLighting)
		{
			++iter;
			continue;
		}

		bool flag = true;
		for(int i=0; i<27; ++i)
		{
			neighbours[i] = GetChunk(*iter + lookUp[i]);
			//check that all the terrain are loaded
			if(neighbours[i] && (!neighbours[i]->LoadedTerrain || !neighbours[i]->InitializedLighting)) {
				flag = false;
				break;
			}
		}

		if(flag)
		{
			auto pos = std::lower_bound(MeshingList.begin(), MeshingList.end(), *iter, cmp3);
			MeshingList.insert(pos, *iter);

			MeshingInfoMap[*iter] = std::make_unique<ChunkMeshingInfo>(neighbours);

			iter = PreMeshingVector.erase(iter);
		}
		else
			++iter;
	}
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
									(RunningThreads < ThreadsSupport && !InitialLightingList.empty());});
		if(!Running)
			return;
		pos = InitialLightingList.back();
		InitialLightingList.pop_back();
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
									(RunningThreads < ThreadsSupport && !MeshingList.empty());});
		if(!Running)
			return;
		pos = MeshingList.back();
		MeshingList.pop_back();
		lk.unlock();

		RunningThreads ++;
		MeshingInfoMap[pos]->Process();
		RunningThreads --;
	}
}

void World::SetBlock(const glm::ivec3 &pos, Block blk, bool checkUpdate)
{
	glm::ivec3 chkPos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chkPos);
	if(!chk)
		return;
	chk->SetBlock(pos - chkPos*CHUNK_SIZE, blk);
}

Block World::GetBlock(const glm::ivec3 &pos) const
{
	glm::ivec3 chkPos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chkPos);
	if(!chk)
		return Blocks::Air;
	return chk->GetBlock(pos - chkPos*CHUNK_SIZE);
}


uint World::GetRunningThreadNum() const
{
	return RunningThreads;
}


