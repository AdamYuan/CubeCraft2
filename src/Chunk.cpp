#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include <FastNoiseSIMD/FastNoiseSIMD.h>
#include "Chunk.hpp"
#include "Resource.hpp"
#include "ChunkAlgorithm.hpp"
#include "WorldData.hpp"

using namespace std::placeholders; //for std::bind

//static methods
Chunk::Chunk(const glm::ivec3 &pos) : loaded_terrain_(false),
									  initialized_mesh_(false), initialized_lighting_(false),
									  position_(pos)
{
	vertex_buffers_[0] = MyGL::NewVertexObject(true);
	vertex_buffers_[1] = MyGL::NewVertexObject(true);
}

//Terrain Generation

ChunkLoadingInfo::ChunkLoadingInfo(const glm::ivec2 &pos, int seed, WorldData &data) : position_(pos), seed_(seed), world_data_(data) {}

void ChunkLoadingInfo::Process()
{
	std::fill(std::begin(result_), std::end(result_), Blocks::Air);

	FastNoiseSIMD* fastNoise = FastNoiseSIMD::NewFastNoiseSIMD();
	fastNoise->SetSeed(seed_);

	constexpr int _SIZE = CHUNK_SIZE + 4, _SIZE2 = _SIZE * _SIZE;

	//generate height map
	int heights[_SIZE*_SIZE], highest = 0;
	fastNoise->SetFractalOctaves(4);
	fastNoise->SetFrequency(0.002f);
	float *heightMap = fastNoise->GetSimplexFractalSet(position_.y*CHUNK_SIZE, position_.x*CHUNK_SIZE, 0, _SIZE, _SIZE, 1);
	for (int i = 0; i < _SIZE2; ++i)
	{
		heights[i] = (int)(heightMap[i] * 50.0f + 100.0f);
		highest = std::max(highest, heights[i]);
	}
	FastNoiseSIMD::FreeNoiseSet(heightMap);

	//generate cave map
	const int CAVEMAP_SIZE = _SIZE2 * (highest + 1);
	bool isCave[CAVEMAP_SIZE];
	fastNoise->SetFrequency(0.012f);
	fastNoise->SetFractalOctaves(1);
	fastNoise->SetCellularDistanceFunction(FastNoiseSIMD::CellularDistanceFunction::Natural);
	fastNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::Distance2Cave);
	float *caveMap = fastNoise->GetCellularSet(0, position_.y*CHUNK_SIZE, position_.x*CHUNK_SIZE, highest + 1, _SIZE, _SIZE);
	for(int i=0; i<CAVEMAP_SIZE; ++i)
		isCave[i] = caveMap[i] > .86f;
	FastNoiseSIMD::FreeNoiseSet(caveMap);

	//main terrain generation
	for (int y = 0; y <= highest; y++) {
		int index = 0;
		for (int z = 0; z < CHUNK_SIZE; z++)
			for (int x = 0; x < CHUNK_SIZE; x++, index++)
			{
				int ind = y * CHUNK_SIZE * CHUNK_SIZE + index;
				int ind2_2d = (z + 2) * _SIZE + x + 2; //index 2d base on (CHUNK_SIZE + 4)
				int ind2_3d = y * _SIZE2 + ind2_2d;

				if(y == 0)
				{
					result_[ind] = Blocks::Bedrock;
					continue;
				}

				if (isCave[ind2_3d] || y > heights[ind2_2d])
					continue;

				if (y == heights[ind2_2d])
					result_[ind] = Blocks::Grass;
				else if(y >= heights[ind2_2d] - heights[ind2_2d] / 70)
					result_[ind] = Blocks::Dirt;
				else
					result_[ind] = Blocks::Stone;
			}
	}

	//tree generation
	fastNoise->SetFrequency(1.0f);
	float *treeMap = fastNoise->GetWhiteNoiseSet(position_.y*CHUNK_SIZE, position_.x*CHUNK_SIZE, 0,
												 _SIZE, _SIZE, 1);
	for (int z = -2, i = 0; z < CHUNK_SIZE + 2; ++z)
		for (int x = -2; x < CHUNK_SIZE + 2; ++x, ++i)
		{
			const bool treeExist = (int) ((treeMap[i] + 1.0f) * 128.0f) == 0;
			if(!treeExist || isCave[heights[i]*_SIZE*_SIZE + i])
				continue;
			const int treeHeight = (int) ((treeMap[i] + 1.0f) * 10000.0f) % 5 + 7;
			const int leavesHeight = (int) ((treeMap[i] + 1.0f) * 10000.0f) % 4 + 2;

			//leaves
			for(int h = heights[i] + treeHeight - leavesHeight + 1; h <= heights[i] + treeHeight; ++h)
			{
				int xMin = std::max(x-2, 0);
				int xMax = std::min(x+2, CHUNK_SIZE-1);
				int zMin = std::max(z-2, 0);
				int zMax = std::min(z+2, CHUNK_SIZE-1);
				for(int _z = zMin; _z <= zMax; ++_z)
					for(int _x = xMin; _x <= xMax; ++_x)
					{
						if(_z == z && _x == x)
							continue;
						int ind = XYZ(_x, h, _z);
						if(result_[ind] == Blocks::Air)
							result_[ind] = Blocks::Leaves;
					}
			}
			if(x >= 0 && x < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE)
			{
				for(int h = heights[i] + 1; h <= heights[i] + treeHeight; ++h)
					result_[XYZ(x, h, z)] = Blocks::Wood;
				result_[XYZ(x, heights[i], z)] = Blocks::Dirt;
			}
		}
	FastNoiseSIMD::FreeNoiseSet(treeMap);

	world_data_.LoadBlocks(position_, result_);
}

void ChunkLoadingInfo::ApplyTerrain(ChunkPtr (&chk)[WORLD_HEIGHT])
{
	for(int i=0; i<WORLD_HEIGHT; ++i) {
		std::copy(this->result_ + i*CHUNK_INFO_SIZE,
				  this->result_ + i*CHUNK_INFO_SIZE + CHUNK_INFO_SIZE,
				  chk[i]->grid_);

		chk[i]->loaded_terrain_ = true;
	}
}



//Mesh Generation

void ChunkMeshingInfo::Process()
{
	ChunkAlgorithm::MeshingThreaded(grid_, light_, position_, result_vertices_, result_indices_);
}

void ChunkMeshingInfo::ApplyResult(ChunkPtr chk)
{
	chk->mesh_vertices_[0] = std::move(result_vertices_[0]);
	chk->mesh_vertices_[1] = std::move(result_vertices_[1]);
	chk->mesh_indices_[0] = std::move(result_indices_[0]);
	chk->mesh_indices_[1] = std::move(result_indices_[1]);
	chk->initialized_mesh_ = true;
}

ChunkMeshingInfo::ChunkMeshingInfo(ChunkPtr (&chk)[27])
{
	//  structure of the neighbour arrays
	// y
	// |
	// |  6   15  24
	// |    7   16  25
	// |      8   17  26
	// |
	// |  3   12  21
	// |    4   13  22
	// |      5   14  23
	// \-------------------x
	//  \ 0   9   18
	//   \  1   10  19
	//    \   2   11  20
	//     z
	position_ = chk[13]->position_;

	int obj, index, index2, ex_index, y, ex_y;
#define SET grid_[ex_index] = chk[obj]->grid_[index]; light_[ex_index] = chk[obj]->light_[index]
#define COPY std::copy(chk[obj]->grid_ + index, chk[obj]->grid_ + index2, grid_ + ex_index);\
	std::copy(chk[obj]->light_ + index, chk[obj]->light_ + index2, light_ + ex_index)

	for(y = 0, ex_y = 0; y < CHUNK_SIZE; ++y, ++ex_y)
	{
		obj = 3;
		index = XYZ(CHUNK_SIZE-1, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(-1, ex_y, -1);
		SET;

		obj = 5;
		index = XYZ(CHUNK_SIZE-1, y, 0);
		ex_index = ExXYZ(-1, ex_y, CHUNK_SIZE);
		SET;

		obj = 23;
		index = XYZ(0, y, 0);
		ex_index = ExXYZ(CHUNK_SIZE, ex_y, CHUNK_SIZE);
		SET;

		obj = 21;
		index = XYZ(0, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(CHUNK_SIZE, ex_y, -1);
		SET;

		obj = 12;
		index = XYZ(0, y, CHUNK_SIZE-1);
		index2 = XYZ(CHUNK_SIZE, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(0, ex_y, -1);
		COPY;

		obj = 14;
		index = XYZ(0, y, 0);
		index2 = XYZ(CHUNK_SIZE, y, 0);
		ex_index = ExXYZ(0, ex_y, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			obj = 4;
			index = XYZ(CHUNK_SIZE-1, y, z);
			ex_index = ExXYZ(-1, ex_y, z);
			SET;

			obj = 22;
			index = XYZ(0, y, z);
			ex_index = ExXYZ(CHUNK_SIZE, ex_y, z);
			SET;

			obj = 13;
			index = XYZ(0, y, z);
			index2 = XYZ(CHUNK_SIZE, y, z);
			ex_index = ExXYZ(0, ex_y, z);
			COPY;
		}
	}

	if(position_.y != WORLD_HEIGHT - 1)
	{
		y = 0, ex_y = CHUNK_SIZE;

		obj = 6;
		index = XYZ(CHUNK_SIZE-1, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(-1, ex_y, -1);
		SET;

		obj = 8;
		index = XYZ(CHUNK_SIZE-1, y, 0);
		ex_index = ExXYZ(-1, ex_y, CHUNK_SIZE);
		SET;

		obj = 26;
		index = XYZ(0, y, 0);
		ex_index = ExXYZ(CHUNK_SIZE, ex_y, CHUNK_SIZE);
		SET;

		obj = 24;
		index = XYZ(0, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(CHUNK_SIZE, ex_y, -1);
		SET;

		obj = 15;
		index = XYZ(0, y, CHUNK_SIZE-1);
		index2 = XYZ(CHUNK_SIZE, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(0, ex_y, -1);
		COPY;

		obj = 17;
		index = XYZ(0, y, 0);
		index2 = XYZ(CHUNK_SIZE, y, 0);
		ex_index = ExXYZ(0, ex_y, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			obj = 7;
			index = XYZ(CHUNK_SIZE-1, y, z);
			ex_index = ExXYZ(-1, ex_y, z);
			SET;

			obj = 25;
			index = XYZ(0, y, z);
			ex_index = ExXYZ(CHUNK_SIZE, ex_y, z);
			SET;

			obj = 16;
			index = XYZ(0, y, z);
			index2 = XYZ(CHUNK_SIZE, y, z);
			ex_index = ExXYZ(0, ex_y, z);
			COPY;
		}
	}
	else
	{
		std::fill(grid_ + ExXYZ(-1, CHUNK_SIZE, -1), grid_ + EXCHUNK_INFO_SIZE, Blocks::Air);
		std::fill(light_ + ExXYZ(-1, CHUNK_SIZE, -1), light_ + EXCHUNK_INFO_SIZE, 0xf0);
	}

	if(position_.y != 0)
	{
		y = CHUNK_SIZE - 1, ex_y = -1;

		obj = 0;
		index = XYZ(CHUNK_SIZE-1, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(-1, ex_y, -1);
		SET;

		obj = 2;
		index = XYZ(CHUNK_SIZE-1, y, 0);
		ex_index = ExXYZ(-1, ex_y, CHUNK_SIZE);
		SET;

		obj = 20;
		index = XYZ(0, y, 0);
		ex_index = ExXYZ(CHUNK_SIZE, ex_y, CHUNK_SIZE);
		SET;

		obj = 18;
		index = XYZ(0, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(CHUNK_SIZE, ex_y, -1);
		SET;

		obj = 9;
		index = XYZ(0, y, CHUNK_SIZE-1);
		index2 = XYZ(CHUNK_SIZE, y, CHUNK_SIZE-1);
		ex_index = ExXYZ(0, ex_y, -1);
		COPY;

		obj = 11;
		index = XYZ(0, y, 0);
		index2 = XYZ(CHUNK_SIZE, y, 0);
		ex_index = ExXYZ(0, ex_y, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			obj = 1;
			index = XYZ(CHUNK_SIZE-1, y, z);
			ex_index = ExXYZ(-1, ex_y, z);
			SET;

			obj = 19;
			index = XYZ(0, y, z);
			ex_index = ExXYZ(CHUNK_SIZE, ex_y, z);
			SET;

			obj = 10;
			index = XYZ(0, y, z);
			index2 = XYZ(CHUNK_SIZE, y, z);
			ex_index = ExXYZ(0, ex_y, z);
			COPY;
		}
	}
	else
	{
		std::fill(grid_, grid_ + EXCHUNK_SIZE * EXCHUNK_SIZE, Blocks::Air);
		std::copy(light_ + ExXYZ(-1, 0, -1), light_ + ExXYZ(-1, 1, -1), light_);
	}
}

ChunkInitialLightingInfo::ChunkInitialLightingInfo(ChunkPtr (&chk)[WORLD_HEIGHT * 9])
{
	for(int h = 0; h < WORLD_HEIGHT; ++h)
	{
		int arr[9];
		for(int i=0; i<9; ++i)
			arr[i] = WORLD_HEIGHT*i + h;

		// 0 3 6
		// 1 4 7
		// 2 5 8
		for(int y=0; y<CHUNK_SIZE; ++y)
		{
			int height = h * CHUNK_SIZE + y;
			for(int z=-14; z<0; ++z)
			{
				int _z = z + CHUNK_SIZE;
				std::copy(chk[arr[0]]->grid_ + XYZ(CHUNK_SIZE-14, y, _z),
						  chk[arr[0]]->grid_ + XYZ(CHUNK_SIZE, y, _z), grid_ + LiXYZ(-14, height, z));
				std::copy(chk[arr[3]]->grid_ + XYZ(0, y, _z),
						  chk[arr[3]]->grid_ + XYZ(CHUNK_SIZE, y, _z), grid_ + LiXYZ(0, height, z));
				std::copy(chk[arr[6]]->grid_ + XYZ(0, y, _z),
						  chk[arr[6]]->grid_ + XYZ(14, y, _z), grid_ + LiXYZ(CHUNK_SIZE, height, z));
			}
			for(int z=0; z<CHUNK_SIZE; ++z)
			{
				std::copy(chk[arr[1]]->grid_ + XYZ(CHUNK_SIZE-14, y, z),
						  chk[arr[1]]->grid_ + XYZ(CHUNK_SIZE, y, z), grid_ + LiXYZ(-14, height, z));
				std::copy(chk[arr[4]]->grid_ + XYZ(0, y, z),
						  chk[arr[4]]->grid_ + XYZ(CHUNK_SIZE, y, z), grid_ + LiXYZ(0, height, z));
				std::copy(chk[arr[7]]->grid_ + XYZ(0, y, z),
						  chk[arr[7]]->grid_ + XYZ(14, y, z), grid_ + LiXYZ(CHUNK_SIZE, height, z));
			}
			for(int z=CHUNK_SIZE; z<CHUNK_SIZE+14; ++z)
			{
				int _z = z - CHUNK_SIZE;
				std::copy(chk[arr[2]]->grid_ + XYZ(CHUNK_SIZE-14, y, _z),
						  chk[arr[2]]->grid_ + XYZ(CHUNK_SIZE, y, _z), grid_ + LiXYZ(-14, height, z));
				std::copy(chk[arr[5]]->grid_ + XYZ(0, y, _z),
						  chk[arr[5]]->grid_ + XYZ(CHUNK_SIZE, y, _z), grid_ + LiXYZ(0, height, z));
				std::copy(chk[arr[8]]->grid_ + XYZ(0, y, _z),
						  chk[arr[8]]->grid_ + XYZ(14, y, _z), grid_ + LiXYZ(CHUNK_SIZE, height, z));
			}
		}
	}
}

void ChunkInitialLightingInfo::Process()
{
	//Get Highest Layer
	int highest = 0;
	constexpr int LICHUNK_SIZE_2 = LICHUNK_SIZE*LICHUNK_SIZE;
	for(int i=LICHUNK_INFO_SIZE - 1; i>=0; i--)
		if(!CanPass(i))
		{
			highest = i / LICHUNK_SIZE_2;
			break;
		}

	std::fill(result_, result_ + (highest+1) * LICHUNK_SIZE_2, 0x00);
	std::fill(result_ + (highest+1) * LICHUNK_SIZE_2, result_ + LICHUNK_INFO_SIZE, 0xF0);

	std::queue<LightBFSNode> que;
	glm::ivec3 pos;

	pos.y = highest;
	for(pos.x = -14; pos.x < CHUNK_SIZE + 14; ++pos.x)
		for(pos.z = -14; pos.z < CHUNK_SIZE + 14; ++pos.z)
		{
			int index = LiXYZ(pos);
			if(!CanPass(index))
				continue;
			result_[index] = 15 << 4;
			que.push({pos, 15});
		}

	ChunkAlgorithm::SunLightBFSThreaded(grid_, result_, highest, que);

	for(int i=0; i<LICHUNK_INFO_SIZE; ++i)
	{
		const LightLevel level = BlockMethods::GetLightLevel(grid_[i]);
		if(level != 0)
		{
			result_[i] = (result_[i] & (uint8_t)0xF0) | level;

			int _i = i;
			pos.y = _i / LICHUNK_SIZE_2;
			_i %= LICHUNK_SIZE_2;
			pos.z = _i / LICHUNK_SIZE - 14;
			pos.x = _i % LICHUNK_SIZE - 14;

			que.push({pos, level});
		}
	}

	ChunkAlgorithm::TorchLightBFSThreaded(grid_, result_, que);
}

void ChunkInitialLightingInfo::ApplyLighting(ChunkPtr (&chk)[WORLD_HEIGHT])
{
	for(int h=0; h<WORLD_HEIGHT; ++h)
	{
		for(int y=0; y<CHUNK_SIZE; ++y)
		{
			int height = h*CHUNK_SIZE + y;
			for(int z=0; z<CHUNK_SIZE; ++z)
			{
				std::copy(result_ + LiXYZ(0, height, z), result_ + LiXYZ(CHUNK_SIZE, height, z),
						  chk[h]->light_ + XYZ(0, y, z));
			}
		}
		chk[h]->initialized_lighting_ = true;
	}
}
