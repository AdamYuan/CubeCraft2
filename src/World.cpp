//
// Created by adamyuan on 12/23/17.
//

#include "World.hpp"
#include "ChunkAlgorithm.hpp"
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

World::World(const std::string &name)
		: threadPool((size_t)Setting::LoadingThreadsNum),
		  RunningThreads(0), PosChanged(false),
		  Database(name), player(*this), LastCenter(INT_MAX),
		  cmp2(std::bind(&World::cmp2_impl, this, std::placeholders::_1, std::placeholders::_2)),
		  cmp3(std::bind(&World::cmp3_impl, this, std::placeholders::_1, std::placeholders::_2))
{
	WorldName = name;
	LoadingThreadNum = LightingThreadNum = MeshingThreadNum = 0;
	glm::ivec3 _;
	int index = 0;
	for(_.x = -1; _.x <= 1; ++_.x)
		for(_.y = -1; _.y <= 1; ++_.y)
			for(_.z = -1; _.z <= 1; ++_.z)
				MeshingLookup[index++] = _;

	Database.LoadWorld(*this);

	const size_t _SIZE = ((size_t)Setting::ChunkLoadRange*2+1) * ((size_t)Setting::ChunkLoadRange*2+1) * WORLD_HEIGHT;
	PreMeshingVector.reserve(_SIZE);
	PreInitialLightingVector.reserve(_SIZE);
}

World::~World()
{
	{
		std::lock_guard<std::mutex> lk(Mutex);
		SuperChunk.clear();
	}
	Database.SaveWorld(*this);
}



void World::Update(const glm::ivec3 &center)
{
	//update time
	Timer = ((float)glfwGetTime() - InitialTime) / DAY_TIME;
	Timer = std::fmod(Timer, 1.0f);

	float radians = Timer * 6.28318530718f;
	SunModelMatrix = glm::rotate(glm::mat4(1.0f), radians, glm::vec3(0.0f, 0.0f, 1.0f));
	SunPosition = glm::vec3(SunModelMatrix * glm::vec4(0.0f, -1.0f, 0.0f, 1.0f));

	//global flags
	PosChanged = false;
	Center = center;

	if(center.x != LastCenter.x || center.z != LastCenter.z)
	{
		PosChanged = true;
		LastCenter = center;
	}

	//set lock
	std::lock_guard<std::mutex> lk(Mutex);

	if(!RenderAdditionSet.empty())
	{
		RenderSet.insert(RenderAdditionSet.begin(), RenderAdditionSet.end());
		RenderAdditionSet.clear();
	}
	if(!RenderRemovalSet.empty())
	{
		for(const auto &i : RenderRemovalSet)
			RenderSet.erase(i);
		RenderRemovalSet.clear();
	}

	if(PosChanged)
	{
		//remove all chunks out of the range
		for(auto iter = SuperChunk.begin(); iter != SuperChunk.end(); ) {
			if (iter->first.x < center.x - Setting::ChunkDeleteRange ||
				iter->first.x > center.x + Setting::ChunkDeleteRange ||
				iter->first.z < center.z - Setting::ChunkDeleteRange ||
				iter->first.z > center.z + Setting::ChunkDeleteRange)
			{
				RenderSet.erase(iter->first);
				iter = SuperChunk.erase(iter);
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

void World::ProcessChunkUpdates()
{
	if(!MeshDirectlyUpdateSet.empty())
	{
		ChunkAlgorithm::SunLightRemovalBFS(this, SunLightRemovalQueue, SunLightQueue);
		ChunkAlgorithm::SunLightBFS(this, SunLightQueue);
		ChunkAlgorithm::TorchLightRemovalBFS(this, TorchLightRemovalQueue, TorchLightQueue);
		ChunkAlgorithm::TorchLightBFS(this, TorchLightQueue);

		for(const glm::ivec3 &pos : MeshDirectlyUpdateSet)
		{
			MeshThreadedUpdateSet.erase(pos);

			if(!ChunkExist(pos))
				continue;
			if(!GetChunk(pos)->InitializedMesh)
				continue;

			std::vector<ChunkRenderVertex> mesh;
			ChunkAlgorithm::Meshing(this, pos, mesh);
			ChunkAlgorithm::ApplyMesh(GetChunk(pos), mesh);
			//update render set
			if(!mesh.empty())
				RenderSet.insert(pos);
			else
				RenderSet.erase(pos);
		}
		MeshDirectlyUpdateSet.clear();
	}
}

void World::UpdateChunkLoadingList()
{
	if(PosChanged)
	{
		PreLoadingVector.clear();
		glm::ivec2 iter;
		for(iter.x = Center.x - Setting::ChunkLoadRange; iter.x <= Center.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.y = Center.z - Setting::ChunkLoadRange; iter.y <= Center.z + Setting::ChunkLoadRange; ++iter.y)
				if(!GetChunk({iter.x, 0, iter.y})->LoadedTerrain && !LoadingInfoSet.count(iter))
					PreLoadingVector.push_back(iter);
		std::sort(PreLoadingVector.begin(), PreLoadingVector.end(), cmp2);
		std::reverse(PreLoadingVector.begin(), PreLoadingVector.end());
	}
	while(!PreLoadingVector.empty() && LoadingThreadNum < Setting::LoadingThreadsNum)
	{
		LoadingThreadNum ++;
		threadPool.enqueue(&World::ChunkLoadingWorker, this, PreLoadingVector.back());
		PreLoadingVector.pop_back();
	}
}

void World::UpdateChunkSunLightingList()
{
	if(PosChanged)
	{
		PreInitialLightingVector.clear();
		glm::ivec2 iter;
		for(iter.x = Center.x - Setting::ChunkLoadRange + 1; iter.x < Center.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.y = Center.z - Setting::ChunkLoadRange + 1; iter.y < Center.z + Setting::ChunkLoadRange; ++iter.y)
				if(!GetChunk({iter.x, 0, iter.y})->InitializedLighting && !InitialLightingInfoSet.count(iter))
					PreInitialLightingVector.push_back(iter);
		std::sort(PreInitialLightingVector.begin(), PreInitialLightingVector.end(), cmp2);
	}

	for(auto iter = PreInitialLightingVector.begin();
		iter != PreInitialLightingVector.end() && LightingThreadNum < Setting::LoadingThreadsNum; )
	{
		glm::ivec3 i(iter->x, 0, iter->y);
		bool flag = true;
		for(i.x = iter->x-1; flag && i.x <= iter->x+1; ++i.x)
			for(i.z = iter->y-1; i.z <= iter->y+1; ++i.z)
				if (!GetChunk(i)->LoadedTerrain) {
					flag = false;
					break;
				}

		if(flag)
		{
			InitialLightingInfoSet.insert(*iter);
			threadPool.enqueue(&World::ChunkInitialLightingWorker, this, *iter);
			LightingThreadNum ++;

			iter = PreInitialLightingVector.erase(iter);
		}
		else
			++iter;
	}
}

void World::UpdateChunkMeshingList()
{
	//mesh initialize
	if(PosChanged)
	{
		PreMeshingVector.clear();
		glm::ivec3 iter;
		for(iter.x = Center.x - Setting::ChunkLoadRange + 1; iter.x < Center.x + Setting::ChunkLoadRange; ++iter.x)
			for(iter.z = Center.z - Setting::ChunkLoadRange + 1; iter.z < Center.z + Setting::ChunkLoadRange; ++iter.z)
				for(iter.y = 0; iter.y < WORLD_HEIGHT; ++iter.y)
					if(!GetChunk(iter)->InitializedMesh && !MeshingInfoSet.count(iter))
						PreMeshingVector.push_back(iter);
		std::sort(PreMeshingVector.begin(), PreMeshingVector.end(), cmp3);
	}

	for(auto iter = PreMeshingVector.begin();
		iter != PreMeshingVector.end() && MeshingThreadNum < Setting::LoadingThreadsNum; )
	{
		bool flag = true;
		for (auto i : MeshingLookup)
		{
			ChunkPtr chk = GetChunk(*iter + i);
			//check that all the terrain are loaded
			if(chk && !chk->InitializedLighting)
			{
				flag = false;
				break;
			}
		}

		if(flag)
		{
			MeshingInfoSet.insert(*iter);
			threadPool.enqueue(&World::ChunkMeshingWorker, this, *iter);
			MeshingThreadNum ++;

			iter = PreMeshingVector.erase(iter);
		}
		else
			++iter;
	}

	//chunk mesh update
	if(!MeshThreadedUpdateSet.empty())
	{
		for(const glm::ivec3 &pos : MeshThreadedUpdateSet)
		{
			if(!ChunkExist(pos))
				continue;
			if(!GetChunk(pos)->InitializedMesh)
				continue;
			if(MeshUpdateInfoSet.count(pos))
				continue;

			MeshUpdateInfoSet.insert(pos);
			threadPool.enqueue(&World::ChunkMeshUpdateWorker, this, pos);
		}
		MeshThreadedUpdateSet.clear();
	}
}


void World::ChunkLoadingWorker(const glm::ivec2 &pos)
{
	glm::ivec3 i(pos.x, 0, pos.y);
	{
		std::lock_guard<std::mutex> lk(Mutex);
		if(!ChunkExist(i))
		{
			LoadingThreadNum --;
			return;
		}
	}

	RunningThreads ++;
	ChunkLoadingInfo info(pos, Seed, Database);
	info.Process();
	RunningThreads --;

	{
		std::lock_guard<std::mutex> lk(Mutex);
		LoadingInfoSet.erase(pos);
		if (ChunkExist(i))
		{
			ChunkPtr arr[WORLD_HEIGHT];
			for (; i.y < WORLD_HEIGHT; i.y++)
				arr[i.y] = GetChunk(i);

			info.ApplyTerrain(arr);
		}
		LoadingThreadNum--;
	}
}

void World::ChunkInitialLightingWorker(const glm::ivec2 &pos)
{
	ChunkPtr copyArr[WORLD_HEIGHT * 9], resultArr[WORLD_HEIGHT];

	{
		std::lock_guard<std::mutex> lk(Mutex);

		bool flag = true;
		int index = 0;
		glm::ivec3 i(pos.x, 0, pos.y);
		for(i.x = pos.x-1; flag && i.x <= pos.x+1; ++i.x)
			for(i.z = pos.y-1; i.z <= pos.y+1; ++i.z) {
				i.y = 0;
				ChunkPtr chk = GetChunk(i);
				if (!chk || !chk->LoadedTerrain) {
					flag = false;
					break;
				}
				copyArr[index++] = chk;
				for (i.y = 1; i.y < WORLD_HEIGHT; i.y++)
					copyArr[index++] = GetChunk(i);
			}
		if(!flag)
		{
			LightingThreadNum --;
			return;
		}
	}

	RunningThreads ++;
	ChunkInitialLightingInfo info(copyArr);
	info.Process();
	RunningThreads --;

	{
		std::lock_guard<std::mutex> lk(Mutex);

		glm::ivec3 i(pos.x, 0, pos.y);
		if(ChunkExist(i))
		{
			for(; i.y<WORLD_HEIGHT; ++i.y)
				resultArr[i.y] = GetChunk(i);

			info.ApplyLighting(resultArr);
		}
		InitialLightingInfoSet.erase(pos);
		LightingThreadNum --;
	}
}

void World::ChunkMeshingWorker(const glm::ivec3 &pos)
{
	ChunkPtr arr[27];
	{
		std::lock_guard<std::mutex> lk(Mutex);

		bool flag = true;
		for (int i=0; i<27; ++i)
		{
			glm::ivec3 neighbour = pos + MeshingLookup[i];
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
			MeshingThreadNum --;
			return;
		}
	}

	RunningThreads ++;
	ChunkMeshingInfo info(arr);
	info.Process();
	RunningThreads --;

	{
		std::lock_guard<std::mutex> lk(Mutex);

		MeshingInfoSet.erase(pos);
		ChunkPtr chk = GetChunk(pos);
		if(chk)
		{
			info.ApplyResult(chk);
			if(!chk->Mesh.empty())
				RenderAdditionSet.insert(pos);
		}
		MeshingThreadNum --;
	}
}

void World::ChunkMeshUpdateWorker(const glm::ivec3 &pos)
{
	ChunkPtr arr[27];

	{
		std::lock_guard<std::mutex> lk(Mutex);

		if(!ChunkExist(pos))
			return;
		if(!GetChunk(pos)->InitializedMesh)
			return;
		for(int i=0; i<27; ++i)
			arr[i] = GetChunk(pos + MeshingLookup[i]);
	}

	RunningThreads ++;
	ChunkMeshingInfo info(arr);
	info.Process();
	RunningThreads --;

	{
		std::lock_guard<std::mutex> lk(Mutex);

		ChunkPtr chk = GetChunk(pos);
		if(chk)
		{
			info.ApplyResult(chk);
			if(chk->Mesh.empty())
				RenderRemovalSet.insert(pos);
			else
				RenderAdditionSet.insert(pos);
		}
		MeshUpdateInfoSet.erase(pos);
	}
}

void World::SetBlock(const glm::ivec3 &pos, Block blk, bool checkUpdate)
{
	glm::ivec3 chkPos = BlockPosToChunkPos(pos),
			bPos = pos - chkPos * CHUNK_SIZE;

	ChunkPtr chk = GetChunk(chkPos);
	if(!chk)
		return;

	uint8_t lastSunLight = chk->GetSunLight(bPos),
			lastBlock = chk->GetBlock(bPos),
			lastTorchLight = chk->GetTorchLight(bPos);
	LightLevel torchlightNow = BlockMethods::GetLightLevel(blk);

	if(lastBlock == blk)
		return;

	chk->SetBlock(bPos, blk);

	if(checkUpdate)
	{
		Database.InsertBlock({chkPos.x, chkPos.z}, XYZ(bPos.x, pos.y, bPos.z), blk);
		AddRelatedChunks(pos, MeshDirectlyUpdateSet);

		//update lighting
		if(BlockMethods::LightCanPass(blk) != BlockMethods::LightCanPass(lastBlock))
		{
			if(!BlockMethods::LightCanPass(blk))
			{
				if(lastSunLight)//have sunlight
				{
					SunLightRemovalQueue.push({pos, lastSunLight});
					chk->SetSunLight(bPos, 0);
				}
			}
			else
			{
				for(short face = 0; face < 6; ++face)
				{
					glm::ivec3 neighbour = Util::FaceExtend(pos, face);
					LightLevel light;

					if(!lastSunLight)
					{
						light = GetSunLight(neighbour);
						if(light)
							SunLightQueue.push({neighbour, light});
					}

					if(!lastTorchLight)
					{
						light = GetTorchLight(neighbour);
						if(light)
							TorchLightQueue.push({neighbour, light});
					}
				}
			}
			if(torchlightNow != lastTorchLight)
			{
				if(torchlightNow < lastTorchLight && lastTorchLight)
				{
					TorchLightRemovalQueue.push({pos, lastTorchLight});
					chk->SetTorchLight(bPos, 0);
				}
				if(torchlightNow)
				{
					TorchLightQueue.push({pos, torchlightNow});
					chk->SetTorchLight(bPos, torchlightNow);
				}
			}
		}
	}
}

Block World::GetBlock(const glm::ivec3 &pos) const
{
	glm::ivec3 chkPos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chkPos);
	if(!chk)
		return Blocks::Air;
	return chk->GetBlock(pos - chkPos*CHUNK_SIZE);
}

LightLevel World::GetSunLight(const glm::ivec3 &pos) const
{
	glm::ivec3 chkPos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chkPos);
	if(pos.y < 0)
		return 0x0;
	else if(pos.y >= WORLD_HEIGHT_BLOCK || !chk)
		return 0xF;
	return chk->GetSunLight(pos - chkPos*CHUNK_SIZE);
}

void World::SetSunLight(const glm::ivec3 &pos, LightLevel val, bool checkUpdate)
{
	glm::ivec3 chkPos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chkPos);
	if(!chk)
		return;
	chk->SetSunLight(pos - chkPos*CHUNK_SIZE, val);

	if(checkUpdate)
		AddRelatedChunks(pos, MeshThreadedUpdateSet);
}

LightLevel World::GetTorchLight(const glm::ivec3 &pos) const
{
	glm::ivec3 chkPos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chkPos);
	if(!chk)
		return 0;
	return chk->GetTorchLight(pos - chkPos*CHUNK_SIZE);
}

void World::SetTorchLight(const glm::ivec3 &pos, LightLevel val, bool checkUpdate)
{
	glm::ivec3 chkPos = BlockPosToChunkPos(pos);

	ChunkPtr chk = GetChunk(chkPos);
	if(!chk)
		return;
	chk->SetTorchLight(pos - chkPos*CHUNK_SIZE, val);

	if(checkUpdate)
		AddRelatedChunks(pos, MeshThreadedUpdateSet);
}

uint World::GetRunningThreadNum() const
{
	return RunningThreads;
}
