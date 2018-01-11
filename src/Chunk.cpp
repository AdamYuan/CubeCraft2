#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include "Chunk.hpp"
#include "Resource.hpp"

#include "FastNoiseSIMD/FastNoiseSIMD.h"

//static methods
Chunk::Chunk(const glm::ivec3 &pos) : LoadedTerrain(false),
									  InitializedMesh(false), InitializedLighting(false),
									  Position(pos)
{
	VertexBuffer = MyGL::NewVertexObject();
}
void Chunk::SetBlock(const glm::ivec3 &pos, Block b)
{
	if(IsValidPosition(pos))
		Grid[XYZ(pos)] = b;
}
Block Chunk::GetBlock(const glm::ivec3 &pos) const
{
	if(IsValidPosition(pos))
		return Grid[XYZ(pos)];
	return Blocks::Air;
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
						int ind = Chunk::XYZ(_x, h, _z);
						if(Result[ind] == Blocks::Air)
							Result[ind] = Blocks::Leaves;
					}
			}
			if(x >= 0 && x < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE)
			{
				for(int h = heights[i] + 1; h <= heights[i] + treeHeight; ++h)
					Result[Chunk::XYZ(x, h, z)] = Blocks::Wood;
				Result[Chunk::XYZ(x, heights[i], z)] = Blocks::Dirt;
			}
		}
	FastNoiseSIMD::FreeNoiseSet(treeMap);
	Done = true;
}

void ChunkLoadingInfo::ApplyTerrain(ChunkPtr (&chk)[WORLD_HEIGHT])
{
	for(int i=0; i<WORLD_HEIGHT; ++i) {
		std::copy(this->Result + i*CHUNK_INFO_SIZE,
				  this->Result + i*CHUNK_INFO_SIZE + CHUNK_INFO_SIZE,
				  chk[i]->Grid);

		chk[i]->LoadedTerrain = true;
	}
}


//FaceLighting (for meshing)
void FaceLighting::SetValues(
		int face,
		const Block (&neighbours)[27],
		const LightLevel (&sunlightNeighbours)[27],
		const LightLevel (&torchlightNeighbours)[27])
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

	const int Lookup3[6][4][3]=
			{
					{{21, 18, 19},	{21, 24, 25},	{23, 26, 25},	{23, 20, 19}},
					{{3, 0, 1},		{5, 2, 1},		{5, 8, 7},		{3, 6, 7}},
					{{15, 6, 7},	{17, 8, 7},		{17, 26, 25},	{15, 24, 25}},
					{{9, 0, 1},		{9, 18, 19},	{11, 20, 19},	{11, 2, 1}},
					{{11, 2, 5},	{11, 20, 23},	{17, 26, 23},	{17, 8, 5}},
					{{9, 0, 3},		{15, 6, 3},		{15, 24, 21},	{9, 18, 21}}
			};
	const int Lookup1[6] = {22, 4, 16, 10, 14, 12};

	Block sides[3];
	bool trans[3];
	for(int v=0; v<4; ++v)
	{
		for(int i=0; i<3; ++i)
		{
			sides[i] = neighbours[Lookup3[face][v][i]];
			trans[i] = BlockMethods::IsTransparent(sides[i]);
		}

		this->AO[v] = (LightLevel)(!trans[0] && !trans[2] ? 0 : 3 - !trans[0] - !trans[1] - !trans[2]);

		//smooth the Light using the average value

		LightLevel counter = 1,
				sunLightSum = sunlightNeighbours[Lookup1[face]],
				torchLightSum = torchlightNeighbours[Lookup1[face]];
		if(!sides[0] || !sides[2])
			for(int i=0; i<3; ++i)
			{
				if(sides[i])
					continue;
				counter++;
				sunLightSum += sunlightNeighbours[Lookup3[face][v][i]];
				torchLightSum += torchlightNeighbours[Lookup3[face][v][i]];
			}

		this->SunLight[v] = sunLightSum / counter;
		this->TorchLight[v] = torchLightSum / counter;
	}
}

bool FaceLighting::operator==(const FaceLighting &f) const
{
	for(int i=0; i<4; ++i)
	{
		if(AO[i] != f.AO[i])
			return false;
		if(SunLight[i] != f.SunLight[i])
			return false;
		if(TorchLight[i] != f.TorchLight[i])
			return false;
	}
	return true;
}

bool FaceLighting::operator!=(const FaceLighting &f) const
{
	for(int i=0; i<4; ++i)
	{
		if(AO[i] != f.AO[i])
			return true;
		if(SunLight[i] != f.SunLight[i])
			return true;
		if(TorchLight[i] != f.TorchLight[i])
			return true;
	}
	return false;
}


//Mesh Generation



void ChunkMeshingInfo::Process()
{
	//generate LightingInfo(including AO, sunlight and torchlight)
	FaceLighting LightingData[CHUNK_INFO_SIZE][6] = {};

	Block neighbours[27];
	LightLevel sunlightNeighbour[27], torchlightNeighbour[27];
	for(int index=0; index<CHUNK_INFO_SIZE; ++index)
	{
		int pos[3], _i = index;
		pos[1] = _i / (CHUNK_SIZE*CHUNK_SIZE);
		_i %= CHUNK_SIZE*CHUNK_SIZE;
		pos[2] = _i / CHUNK_SIZE;
		pos[0] = _i % CHUNK_SIZE;

		Block block;
		if((block = GetBlock(pos[0], pos[1], pos[2])) == Blocks::Air)
			continue;

		bool initedNeighbours = false;

		for(short face=0; face<6; ++face)
		{
			int nei[3] = {pos[0], pos[1], pos[2]};
			nei[face>>1] += 1 - ((face&1)<<1);

			Block blockN = GetBlock(nei[0], nei[1], nei[2]);

			if(!ShowFace(block, blockN))
				continue;

			if(!initedNeighbours)
			{
				initedNeighbours = true;
				int it[3], ind = 0;
				for(it[0] = pos[0]-1; it[0] <= pos[0]+1; ++it[0])
					for(it[1] = pos[1]-1; it[1] <= pos[1]+1; ++it[1])
						for(it[2] = pos[2]-1; it[2] <= pos[2]+1; ++it[2], ++ind)
						{
							neighbours[ind] = GetBlock(it[0], it[1], it[2]);
							sunlightNeighbour[ind] = GetSunLight(it[0], it[1], it[2]);
							torchlightNeighbour[ind] = GetTorchLight(it[0], it[1], it[2]);
						}
			}

			LightingData[index][face].SetValues(face, neighbours, sunlightNeighbour, torchlightNeighbour);
		}
	}

	//meshing
	for (std::size_t axis = 0; axis < 3; ++axis)
	{
		const std::size_t u = (axis + 1) % 3;
		const std::size_t v = (axis + 2) % 3;

		int x[3] = {0}, q[3] = {0}, mask[CHUNK_SIZE*CHUNK_SIZE];
		FaceLighting *lightMask[CHUNK_SIZE*CHUNK_SIZE];

		// Compute mask
		q[axis] = 1;
		for (x[axis] = -1; x[axis] < CHUNK_SIZE;)
		{
			std::size_t counter = 0;
			for (x[v] = 0; x[v] < CHUNK_SIZE; ++x[v]) {
				for (x[u] = 0; x[u] < CHUNK_SIZE; ++x[u], ++counter) {
					const Block a = GetBlock(x[0], x[1], x[2]);
					const Block b = GetBlock(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

					bool outA = x[axis] < 0, outB = CHUNK_SIZE - 1 <= x[axis];

					if (!outA && ShowFace(a, b))
					{
						mask[counter] = a;
						lightMask[counter] = &LightingData[Chunk::XYZ(x[0], x[1], x[2])][axis << 1];

					}
					else if (!outB && ShowFace(b, a))
					{
						mask[counter] = -b;
						lightMask[counter] = &LightingData[
								Chunk::XYZ(x[0] + q[0], x[1] + q[1], x[2] + q[2])][(axis << 1) | 1];
					}
					else
					{
						mask[counter] = 0;
						lightMask[counter] = nullptr;
					}
				}
			}

			++x[axis];

			// Generate mesh for mask using lexicographic ordering
			std::size_t width = 0, height = 0;

			counter = 0;
			for (std::size_t j = 0; j < CHUNK_SIZE; ++j) {
				for (std::size_t i = 0; i < CHUNK_SIZE;) {
					int QuadType = mask[counter];
					if (QuadType) {
						const FaceLighting &QuadLighting = *lightMask[counter];
						// Compute width
						for (width = 1;
							 QuadType == mask[counter + width] &&
							 lightMask[counter + width] && QuadLighting == *lightMask[counter + width] &&
							 i + width < CHUNK_SIZE;
							 ++width);

						// Compute height
						bool done = false;
						for (height = 1; j + height < CHUNK_SIZE; ++height) {
							for (std::size_t k = 0; k < width; ++k) {
								size_t ind = counter + k + height * CHUNK_SIZE;
								if (QuadType != mask[ind] ||
									!lightMask[ind] || QuadLighting != *lightMask[ind]) {
									done = true;
									break;
								}
							}

							if (done)
								break;
						}

						// Add quad
						x[u] = i;
						x[v] = j;

						int du[3] = {0}, dv[3] = {0};

						Face QuadFace = (Face) ((axis << 1) | (QuadType <= 0));

						if (QuadType > 0) {
							dv[v] = height;
							du[u] = width;
						} else {
							QuadType = -QuadType;
							du[v] = height;
							dv[u] = width;
						}

						int QuadTex = BlockMethods::GetTexture(QuadType, QuadFace);

						float vx = Position.x * CHUNK_SIZE + x[0],
								vy = Position.y * CHUNK_SIZE + x[1],
								vz = Position.z * CHUNK_SIZE + x[2];

						ChunkRenderVertex
								v00 = {vx, vy, vz,
									   (float) (du[u] + dv[u]), (float) (du[v] + dv[v]),
									   (float) QuadTex, (float) QuadFace,
									   (float) QuadLighting.AO[0],
									   (float) QuadLighting.SunLight[0], (float) QuadLighting.TorchLight[0]},

								v01 = {vx + du[0], vy + du[1], vz + du[2],
									   (float) dv[u], (float) dv[v],
									   (float) QuadTex, (float) QuadFace,
									   (float) QuadLighting.AO[1],
									   (float) QuadLighting.SunLight[1], (float) QuadLighting.TorchLight[1]},

								v10 = {vx + du[0] + dv[0], vy + du[1] + dv[1], vz + du[2] + dv[2],
									   0.0f, 0.0f,
									   (float) QuadTex, (float) QuadFace,
									   (float) QuadLighting.AO[2],
									   (float) QuadLighting.SunLight[2], (float) QuadLighting.TorchLight[2]},

								v11 = {vx + dv[0], vy + dv[1], vz + dv[2],
									   (float) du[u], (float) du[v],
									   (float) QuadTex, (float) QuadFace,
									   (float) QuadLighting.AO[3],
									   (float) QuadLighting.SunLight[3], (float) QuadLighting.TorchLight[3]};

						if (QuadFace == Face::Left) {
							std::swap(v00.U, v01.V);
							std::swap(v00.V, v01.U);
							std::swap(v11.U, v10.V);
							std::swap(v11.V, v10.U);
						} else if (QuadFace == Face::Right) {
							std::swap(v11.U, v11.V);
							std::swap(v01.U, v01.V);
							std::swap(v00.U, v00.V);
						} else if (QuadFace == Face::Front) {
							std::swap(v00.U, v01.U);
							std::swap(v00.V, v01.V);
							std::swap(v11.U, v10.U);
							std::swap(v11.V, v10.V);
						}

						if (QuadLighting.AO[0] + QuadLighting.AO[2] +
							QuadLighting.SunLight[0] + QuadLighting.SunLight[2]
							>
							QuadLighting.AO[1] + QuadLighting.AO[3] +
							QuadLighting.SunLight[1] + QuadLighting.SunLight[3]) {
							//11--------10
							//|       / |
							//|    /    |
							//| /       |
							//00--------01
							Result.push_back(v00);
							Result.push_back(v01);
							Result.push_back(v10);

							Result.push_back(v00);
							Result.push_back(v10);
							Result.push_back(v11);
						} else {
							//11--------10
							//| \       |
							//|    \    |
							//|       \ |
							//00--------01
							Result.push_back(v01);
							Result.push_back(v10);
							Result.push_back(v11);

							Result.push_back(v00);
							Result.push_back(v01);
							Result.push_back(v11);
						}

						for (std::size_t b = 0; b < width; ++b)
							for (std::size_t a = 0; a < height; ++a) {
								size_t ind = counter + b + a * CHUNK_SIZE;
								mask[ind] = 0;
								lightMask[ind] = nullptr;
							}

						// Increment counters
						i += width;
						counter += width;
					} else {
						++i;
						++counter;
					}
				}
			}
		}
	}

	Done = true;
}

void ChunkMeshingInfo::ApplyMesh(ChunkPtr chk)
{
	chk->VertexBuffer->SetDataVec(Result);
	chk->VertexBuffer->SetAttributes(4,
										Resource::ATTR_POSITION, 3,
										Resource::ATTR_TEXCOORD, 3,
										Resource::ATTR_CHUNK_FACE, 1,
										Resource::ATTR_CHUNK_LIGHTING, 3);
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
		Index = Chunk::XYZ(CHUNK_SIZE-1, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(-1, ExY, -1);
		SET;

		Obj = 5;
		Index = Chunk::XYZ(CHUNK_SIZE-1, Y, 0);
		ExIndex = ExXYZ(-1, ExY, CHUNK_SIZE);
		SET;

		Obj = 23;
		Index = Chunk::XYZ(0, Y, 0);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, CHUNK_SIZE);
		SET;

		Obj = 21;
		Index = Chunk::XYZ(0, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, -1);
		SET;

		Obj = 12;
		Index = Chunk::XYZ(0, Y, CHUNK_SIZE-1);
		Index2 = Chunk::XYZ(CHUNK_SIZE, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(0, ExY, -1);
		COPY;

		Obj = 14;
		Index = Chunk::XYZ(0, Y, 0);
		Index2 = Chunk::XYZ(CHUNK_SIZE, Y, 0);
		ExIndex = ExXYZ(0, ExY, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			Obj = 4;
			Index = Chunk::XYZ(CHUNK_SIZE-1, Y, z);
			ExIndex = ExXYZ(-1, ExY, z);
			SET;

			Obj = 22;
			Index = Chunk::XYZ(0, Y, z);
			ExIndex = ExXYZ(CHUNK_SIZE, ExY, z);
			SET;

			Obj = 13;
			Index = Chunk::XYZ(0, Y, z);
			Index2 = Chunk::XYZ(CHUNK_SIZE, Y, z);
			ExIndex = ExXYZ(0, ExY, z);
			COPY;
		}
	}

	if(Position.y != WORLD_HEIGHT - 1)
	{
		Y = 0, ExY = CHUNK_SIZE;

		Obj = 6;
		Index = Chunk::XYZ(CHUNK_SIZE-1, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(-1, ExY, -1);
		SET;

		Obj = 8;
		Index = Chunk::XYZ(CHUNK_SIZE-1, Y, 0);
		ExIndex = ExXYZ(-1, ExY, CHUNK_SIZE);
		SET;

		Obj = 26;
		Index = Chunk::XYZ(0, Y, 0);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, CHUNK_SIZE);
		SET;

		Obj = 24;
		Index = Chunk::XYZ(0, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, -1);
		SET;

		Obj = 15;
		Index = Chunk::XYZ(0, Y, CHUNK_SIZE-1);
		Index2 = Chunk::XYZ(CHUNK_SIZE, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(0, ExY, -1);
		COPY;

		Obj = 17;
		Index = Chunk::XYZ(0, Y, 0);
		Index2 = Chunk::XYZ(CHUNK_SIZE, Y, 0);
		ExIndex = ExXYZ(0, ExY, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			Obj = 7;
			Index = Chunk::XYZ(CHUNK_SIZE-1, Y, z);
			ExIndex = ExXYZ(-1, ExY, z);
			SET;

			Obj = 25;
			Index = Chunk::XYZ(0, Y, z);
			ExIndex = ExXYZ(CHUNK_SIZE, ExY, z);
			SET;

			Obj = 16;
			Index = Chunk::XYZ(0, Y, z);
			Index2 = Chunk::XYZ(CHUNK_SIZE, Y, z);
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
		Index = Chunk::XYZ(CHUNK_SIZE-1, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(-1, ExY, -1);
		SET;

		Obj = 2;
		Index = Chunk::XYZ(CHUNK_SIZE-1, Y, 0);
		ExIndex = ExXYZ(-1, ExY, CHUNK_SIZE);
		SET;

		Obj = 20;
		Index = Chunk::XYZ(0, Y, 0);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, CHUNK_SIZE);
		SET;

		Obj = 18;
		Index = Chunk::XYZ(0, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(CHUNK_SIZE, ExY, -1);
		SET;

		Obj = 9;
		Index = Chunk::XYZ(0, Y, CHUNK_SIZE-1);
		Index2 = Chunk::XYZ(CHUNK_SIZE, Y, CHUNK_SIZE-1);
		ExIndex = ExXYZ(0, ExY, -1);
		COPY;

		Obj = 11;
		Index = Chunk::XYZ(0, Y, 0);
		Index2 = Chunk::XYZ(CHUNK_SIZE, Y, 0);
		ExIndex = ExXYZ(0, ExY, CHUNK_SIZE);
		COPY;

		for(int z=0; z<CHUNK_SIZE; ++z)
		{
			Obj = 1;
			Index = Chunk::XYZ(CHUNK_SIZE-1, Y, z);
			ExIndex = ExXYZ(-1, ExY, z);
			SET;

			Obj = 19;
			Index = Chunk::XYZ(0, Y, z);
			ExIndex = ExXYZ(CHUNK_SIZE, ExY, z);
			SET;

			Obj = 10;
			Index = Chunk::XYZ(0, Y, z);
			Index2 = Chunk::XYZ(CHUNK_SIZE, Y, z);
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
				std::copy(chk[arr[0]]->Grid + Chunk::XYZ(CHUNK_SIZE-14, y, _z),
						  chk[arr[0]]->Grid + Chunk::XYZ(CHUNK_SIZE, y, _z), Grid + LiXYZ(-14, height, z));
				std::copy(chk[arr[3]]->Grid + Chunk::XYZ(0, y, _z),
						  chk[arr[3]]->Grid + Chunk::XYZ(CHUNK_SIZE, y, _z), Grid + LiXYZ(0, height, z));
				std::copy(chk[arr[6]]->Grid + Chunk::XYZ(0, y, _z),
						  chk[arr[6]]->Grid + Chunk::XYZ(14, y, _z), Grid + LiXYZ(CHUNK_SIZE, height, z));
			}
			for(int z=0; z<CHUNK_SIZE; ++z)
			{
				std::copy(chk[arr[1]]->Grid + Chunk::XYZ(CHUNK_SIZE-14, y, z),
						  chk[arr[1]]->Grid + Chunk::XYZ(CHUNK_SIZE, y, z), Grid + LiXYZ(-14, height, z));
				std::copy(chk[arr[4]]->Grid + Chunk::XYZ(0, y, z),
						  chk[arr[4]]->Grid + Chunk::XYZ(CHUNK_SIZE, y, z), Grid + LiXYZ(0, height, z));
				std::copy(chk[arr[7]]->Grid + Chunk::XYZ(0, y, z),
						  chk[arr[7]]->Grid + Chunk::XYZ(14, y, z), Grid + LiXYZ(CHUNK_SIZE, height, z));
			}
			for(int z=CHUNK_SIZE; z<CHUNK_SIZE+14; ++z)
			{
				int _z = z - CHUNK_SIZE;
				std::copy(chk[arr[2]]->Grid + Chunk::XYZ(CHUNK_SIZE-14, y, _z),
						  chk[arr[2]]->Grid + Chunk::XYZ(CHUNK_SIZE, y, _z), Grid + LiXYZ(-14, height, z));
				std::copy(chk[arr[5]]->Grid + Chunk::XYZ(0, y, _z),
						  chk[arr[5]]->Grid + Chunk::XYZ(CHUNK_SIZE, y, _z), Grid + LiXYZ(0, height, z));
				std::copy(chk[arr[8]]->Grid + Chunk::XYZ(0, y, _z),
						  chk[arr[8]]->Grid + Chunk::XYZ(14, y, _z), Grid + LiXYZ(CHUNK_SIZE, height, z));
			}
		}
	}
}

void ChunkInitialLightingInfo::Process()
{
	//Get Highest Layer
	constexpr int LICHUNK_SIZE_2 = LICHUNK_SIZE*LICHUNK_SIZE;
	for(int i=0; i<LICHUNK_INFO_SIZE; )
	{
		if(!CanPass(i))
		{
			Highest = std::max(Highest, i / LICHUNK_SIZE_2);
			i /= LICHUNK_SIZE_2;
			i = i * LICHUNK_SIZE_2 + LICHUNK_SIZE_2;
		} else
			++i;
	}

	std::fill(Result, Result + (Highest+1) * LICHUNK_SIZE_2, 0x00);
	std::fill(Result + (Highest+1) * LICHUNK_SIZE_2, Result + LICHUNK_INFO_SIZE, 0xF0);


	std::queue<LightBFSNode> SunLightQueue;

	glm::ivec3 pos;
	pos.y = Highest;
	for(pos.x = -14; pos.x < CHUNK_SIZE + 14; ++pos.x)
		for(pos.z = -14; pos.z < CHUNK_SIZE + 14; ++pos.z)
		{
			int index = LiXYZ(pos);
			if(!CanPass(index))
				continue;
			Result[index] = 15 << 4;
			SunLightQueue.push({pos, 15});
		}

	while(!SunLightQueue.empty())
	{
		LightBFSNode node = SunLightQueue.front();
		SunLightQueue.pop();

		if((Result[LiXYZ(node.Pos)] >> 4) > node.Value)
			continue;

		for(short face=0; face<6; ++face)
		{
			LightBFSNode neighbour = node;
			neighbour.Pos = Util::FaceExtend(node.Pos, face);

			int index = LiXYZ(neighbour.Pos);

			//deal with out chunk situations
			if(face>>1 != 1)
			{
				neighbour.Value--;
				if(neighbour.Pos[face>>1] < -14 || neighbour.Pos[face>>1] >= CHUNK_SIZE + 14)
					continue;
			}
			else
			{
				if(neighbour.Pos.y < 0 || neighbour.Pos.y >= WORLD_HEIGHT_BLOCK)
					continue;
				else if(neighbour.Value == 15 && face == Face::Top)
					continue;
				else if(neighbour.Value != 15)
					neighbour.Value --;
			}

			if(CanPass(index) && (Result[index] >> 4) < neighbour.Value)
			{
				Result[index] = neighbour.Value << 4;
				SunLightQueue.push(neighbour);
			}
		}
	}

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
						  chk[h]->Light + Chunk::XYZ(0, y, z));
			}
		}
		chk[h]->InitializedLighting = true;
	}
}



