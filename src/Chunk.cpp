#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include <FastNoiseSIMD/FastNoiseSIMD.h>
#include "Chunk.hpp"
#include "Resource.hpp"
#include "ChunkAlgorithm.hpp"
#include "WorldData.hpp"

using namespace std::placeholders; //for std::bind

//static methods
Chunk::Chunk(const glm::ivec3 &pos) : LoadedTerrain(false),
									  InitializedMesh(false), InitializedLighting(false),
									  Position(pos)
{
	VertexBuffer = MyGL::NewVertexObject();
}

//Terrain Generation

ChunkLoadingInfo::ChunkLoadingInfo(const glm::ivec2 &pos)
{
	this->Position = pos;
}

void ChunkLoadingInfo::Process()
{
	std::fill(std::begin(Result), std::end(Result), Blocks::Air);

	FastNoiseSIMD* fastNoise = FastNoiseSIMD::NewFastNoiseSIMD();
	constexpr int _SIZE = CHUNK_SIZE + 4, _SIZE2 = _SIZE * _SIZE;

	//generate height map
	int heights[_SIZE*_SIZE], highest = 0;
	fastNoise->SetFractalOctaves(4);
	fastNoise->SetFrequency(0.002f);
	float *heightMap = fastNoise->GetSimplexFractalSet(Position.y*CHUNK_SIZE, Position.x*CHUNK_SIZE, 0, _SIZE, _SIZE, 1);
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
	float *caveMap = fastNoise->GetCellularSet(0, Position.y*CHUNK_SIZE, Position.x*CHUNK_SIZE, highest + 1, _SIZE, _SIZE);
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
					Result[ind] = Blocks::Bedrock;
					continue;
				}

				if (isCave[ind2_3d] || y > heights[ind2_2d])
					continue;

				if (y == heights[ind2_2d])
					Result[ind] = Blocks::Grass;
				else if(y >= heights[ind2_2d] - heights[ind2_2d] / 70)
					Result[ind] = Blocks::Dirt;
				else
					Result[ind] = Blocks::Stone;
			}
	}

	//tree generation
	fastNoise->SetFrequency(1.0f);
	float *treeMap = fastNoise->GetWhiteNoiseSet(Position.y*CHUNK_SIZE, Position.x*CHUNK_SIZE, 0,
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
						if(Result[ind] == Blocks::Air)
							Result[ind] = Blocks::Leaves;
					}
			}
			if(x >= 0 && x < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE)
			{
				for(int h = heights[i] + 1; h <= heights[i] + treeHeight; ++h)
					Result[XYZ(x, h, z)] = Blocks::Wood;
				Result[XYZ(x, heights[i], z)] = Blocks::Dirt;
			}
		}
	FastNoiseSIMD::FreeNoiseSet(treeMap);
	Done = true;
}

void ChunkLoadingInfo::ApplyTerrain(ChunkPtr (&chk)[WORLD_HEIGHT], WorldData &data)
{
	data.LoadBlocks(Position, Result);
	for(int i=0; i<WORLD_HEIGHT; ++i) {
		std::copy(this->Result + i*CHUNK_INFO_SIZE,
				  this->Result + i*CHUNK_INFO_SIZE + CHUNK_INFO_SIZE,
				  chk[i]->Grid);

		chk[i]->LoadedTerrain = true;
	}
}



//Mesh Generation

void ChunkMeshingInfo::Process()
{
	ChunkAlgorithm::MeshingThreaded(Grid, Light, Position, Result);
	Done = true;
}

void ChunkMeshingInfo::ApplyMesh(ChunkPtr chk)
{
	ChunkAlgorithm::ApplyMesh(chk, Result);
	chk->InitializedMesh = true;
	//std::cout << Result.size() << std::endl;
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
	Position = chk[13]->Position;

	int Obj, Index, Index2, ExIndex, Y, ExY;
#define SET Grid[ExIndex] = chk[Obj]->Grid[Index]; Light[ExIndex] = chk[Obj]->Light[Index]
#define COPY std::copy(chk[Obj]->Grid + Index, chk[Obj]->Grid + Index2, Grid + ExIndex);\
	std::copy(chk[Obj]->Light + Index, chk[Obj]->Light + Index2, Light + ExIndex)

	for(Y = 0, ExY = 0; Y < CHUNK_SIZE; ++Y, ++ExY)
	{
		Obj = 3;
		Index = XYZ(CHUNK_SIZE-1, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(-1, ExY, -1);
		SET;

		Obj = 5;
		Index = XYZ(CHUNK_SIZE-1, Y, 0);
		ExIndex = ExXYZ(-1, ExY, CHUNK_SIZE);
		SET;

		Obj = 23;
		Index = XYZ(0, Y, 0);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, CHUNK_SIZE);
		SET;

		Obj = 21;
		Index = XYZ(0, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, -1);
		SET;

		Obj = 12;
		Index = XYZ(0, Y, CHUNK_SIZE-1);
		Index2 = XYZ(CHUNK_SIZE, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(0, ExY, -1);
		COPY;

		Obj = 14;
		Index = XYZ(0, Y, 0);
		Index2 = XYZ(CHUNK_SIZE, Y, 0);
		ExIndex = ExXYZ(0, ExY, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			Obj = 4;
			Index = XYZ(CHUNK_SIZE-1, Y, z);
			ExIndex = ExXYZ(-1, ExY, z);
			SET;

			Obj = 22;
			Index = XYZ(0, Y, z);
			ExIndex = ExXYZ(CHUNK_SIZE, ExY, z);
			SET;

			Obj = 13;
			Index = XYZ(0, Y, z);
			Index2 = XYZ(CHUNK_SIZE, Y, z);
			ExIndex = ExXYZ(0, ExY, z);
			COPY;
		}
	}

	if(Position.y != WORLD_HEIGHT - 1)
	{
		Y = 0, ExY = CHUNK_SIZE;

		Obj = 6;
		Index = XYZ(CHUNK_SIZE-1, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(-1, ExY, -1);
		SET;

		Obj = 8;
		Index = XYZ(CHUNK_SIZE-1, Y, 0);
		ExIndex = ExXYZ(-1, ExY, CHUNK_SIZE);
		SET;

		Obj = 26;
		Index = XYZ(0, Y, 0);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, CHUNK_SIZE);
		SET;

		Obj = 24;
		Index = XYZ(0, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, -1);
		SET;

		Obj = 15;
		Index = XYZ(0, Y, CHUNK_SIZE-1);
		Index2 = XYZ(CHUNK_SIZE, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(0, ExY, -1);
		COPY;

		Obj = 17;
		Index = XYZ(0, Y, 0);
		Index2 = XYZ(CHUNK_SIZE, Y, 0);
		ExIndex = ExXYZ(0, ExY, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			Obj = 7;
			Index = XYZ(CHUNK_SIZE-1, Y, z);
			ExIndex = ExXYZ(-1, ExY, z);
			SET;

			Obj = 25;
			Index = XYZ(0, Y, z);
			ExIndex = ExXYZ(CHUNK_SIZE, ExY, z);
			SET;

			Obj = 16;
			Index = XYZ(0, Y, z);
			Index2 = XYZ(CHUNK_SIZE, Y, z);
			ExIndex = ExXYZ(0, ExY, z);
			COPY;
		}
	}
	else
	{
		std::fill(Grid + ExXYZ(-1, CHUNK_SIZE, -1), Grid + EXCHUNK_INFO_SIZE, Blocks::Air);
		std::fill(Light + ExXYZ(-1, CHUNK_SIZE, -1), Light + EXCHUNK_INFO_SIZE, 0xf0);
	}

	if(Position.y != 0)
	{
		Y = CHUNK_SIZE - 1, ExY = -1;

		Obj = 0;
		Index = XYZ(CHUNK_SIZE-1, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(-1, ExY, -1);
		SET;

		Obj = 2;
		Index = XYZ(CHUNK_SIZE-1, Y, 0);
		ExIndex = ExXYZ(-1, ExY, CHUNK_SIZE);
		SET;

		Obj = 20;
		Index = XYZ(0, Y, 0);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, CHUNK_SIZE);
		SET;

		Obj = 18;
		Index = XYZ(0, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, -1);
		SET;

		Obj = 9;
		Index = XYZ(0, Y, CHUNK_SIZE-1);
		Index2 = XYZ(CHUNK_SIZE, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(0, ExY, -1);
		COPY;

		Obj = 11;
		Index = XYZ(0, Y, 0);
		Index2 = XYZ(CHUNK_SIZE, Y, 0);
		ExIndex = ExXYZ(0, ExY, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			Obj = 1;
			Index = XYZ(CHUNK_SIZE-1, Y, z);
			ExIndex = ExXYZ(-1, ExY, z);
			SET;

			Obj = 19;
			Index = XYZ(0, Y, z);
			ExIndex = ExXYZ(CHUNK_SIZE, ExY, z);
			SET;

			Obj = 10;
			Index = XYZ(0, Y, z);
			Index2 = XYZ(CHUNK_SIZE, Y, z);
			ExIndex = ExXYZ(0, ExY, z);
			COPY;
		}
	}
	else
	{
		std::fill(Grid, Grid + EXCHUNK_SIZE * EXCHUNK_SIZE, Blocks::Air);
		std::copy(Light + ExXYZ(-1, 0, -1), Light + ExXYZ(-1, 1, -1), Light);
	}
}

ChunkInitialLightingInfo::ChunkInitialLightingInfo(ChunkPtr (&chk)[WORLD_HEIGHT * 9]) : Highest(0)
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
				std::copy(chk[arr[0]]->Grid + XYZ(CHUNK_SIZE-14, y, _z),
						  chk[arr[0]]->Grid + XYZ(CHUNK_SIZE, y, _z), Grid + LiXYZ(-14, height, z));
				std::copy(chk[arr[3]]->Grid + XYZ(0, y, _z),
						  chk[arr[3]]->Grid + XYZ(CHUNK_SIZE, y, _z), Grid + LiXYZ(0, height, z));
				std::copy(chk[arr[6]]->Grid + XYZ(0, y, _z),
						  chk[arr[6]]->Grid + XYZ(14, y, _z), Grid + LiXYZ(CHUNK_SIZE, height, z));
			}
			for(int z=0; z<CHUNK_SIZE; ++z)
			{
				std::copy(chk[arr[1]]->Grid + XYZ(CHUNK_SIZE-14, y, z),
						  chk[arr[1]]->Grid + XYZ(CHUNK_SIZE, y, z), Grid + LiXYZ(-14, height, z));
				std::copy(chk[arr[4]]->Grid + XYZ(0, y, z),
						  chk[arr[4]]->Grid + XYZ(CHUNK_SIZE, y, z), Grid + LiXYZ(0, height, z));
				std::copy(chk[arr[7]]->Grid + XYZ(0, y, z),
						  chk[arr[7]]->Grid + XYZ(14, y, z), Grid + LiXYZ(CHUNK_SIZE, height, z));
			}
			for(int z=CHUNK_SIZE; z<CHUNK_SIZE+14; ++z)
			{
				int _z = z - CHUNK_SIZE;
				std::copy(chk[arr[2]]->Grid + XYZ(CHUNK_SIZE-14, y, _z),
						  chk[arr[2]]->Grid + XYZ(CHUNK_SIZE, y, _z), Grid + LiXYZ(-14, height, z));
				std::copy(chk[arr[5]]->Grid + XYZ(0, y, _z),
						  chk[arr[5]]->Grid + XYZ(CHUNK_SIZE, y, _z), Grid + LiXYZ(0, height, z));
				std::copy(chk[arr[8]]->Grid + XYZ(0, y, _z),
						  chk[arr[8]]->Grid + XYZ(14, y, _z), Grid + LiXYZ(CHUNK_SIZE, height, z));
			}
		}
	}
}

void ChunkInitialLightingInfo::Process()
{
	//Get Highest Layer
	constexpr int LICHUNK_SIZE_2 = LICHUNK_SIZE*LICHUNK_SIZE;
	for(int i=LICHUNK_INFO_SIZE - 1; i>=0; i--)
		if(!CanPass(i))
		{
			Highest = i / LICHUNK_SIZE_2;
			break;
		}

	std::fill(Result, Result + (Highest+1) * LICHUNK_SIZE_2, 0x00);
	std::fill(Result + (Highest+1) * LICHUNK_SIZE_2, Result + LICHUNK_INFO_SIZE, 0xF0);

	std::queue<LightBFSNode> Queue;
	glm::ivec3 pos;

	pos.y = Highest;
	for(pos.x = -14; pos.x < CHUNK_SIZE + 14; ++pos.x)
		for(pos.z = -14; pos.z < CHUNK_SIZE + 14; ++pos.z)
		{
			int index = LiXYZ(pos);
			if(!CanPass(index))
				continue;
			Result[index] = 15 << 4;
			Queue.push({pos, 15});
		}

	ChunkAlgorithm::SunLightBFSThreaded(Grid, Result, Highest, Queue);

	for(int i=0; i<LICHUNK_INFO_SIZE; ++i)
	{
		const LightLevel level = BlockMethods::GetLightLevel(Grid[i]);
		if(level != 0)
		{
			Result[i] = (Result[i] & (uint8_t)0xF0) | level;

			int _i = i;
			pos.y = _i / LICHUNK_SIZE_2;
			_i %= LICHUNK_SIZE_2;
			pos.z = _i / LICHUNK_SIZE - 14;
			pos.x = _i % LICHUNK_SIZE - 14;

			Queue.push({pos, level});
		}
	}

	ChunkAlgorithm::TorchLightBFSThreaded(Grid, Result, Queue);


	Done = true;
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
				std::copy(Result + LiXYZ(0, height, z), Result + LiXYZ(CHUNK_SIZE, height, z),
						  chk[h]->Light + XYZ(0, y, z));
			}
		}
		chk[h]->InitializedLighting = true;
	}
}
