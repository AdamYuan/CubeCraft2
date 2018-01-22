//
// Created by adamyuan on 1/11/18.
//

#include "Resource.hpp"
#include "ChunkAlgorithm.hpp"
#include "Chunk.hpp"
#include "World.hpp"

#include <glm/gtx/string_cast.hpp>

namespace ChunkAlgorithm
{
//FaceLighting (for meshing)
	void FaceLighting::SetValues(
			int face,
			const uint8_t (&neighbours)[27],
			const uint8_t (&sunlightNeighbours)[27],
			const uint8_t (&torchlightNeighbours)[27])
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

		uint8_t sides[3];
		bool trans[3], canPass[3];

		for(int v=0; v<4; ++v)
		{
			for(int i=0; i<3; ++i)
			{
				sides[i] = neighbours[Lookup3[face][v][i]];
				trans[i] = BlockMethods::IsTransparent(sides[i]);
				canPass[i] = BlockMethods::LightCanPass(sides[i]);
			}

			this->AO[v] = (LightLevel)(!trans[0] && !trans[2] ? 0 : 3 - !trans[0] - !trans[1] - !trans[2]);

			//smooth the Light using the average value

			uint8_t counter = 1,
					sunLightSum = sunlightNeighbours[Lookup1[face]],
					torchLightSum = torchlightNeighbours[Lookup1[face]];
			if(canPass[0] || canPass[2])
				for(int i=0; i<3; ++i)
				{
					if(!canPass[i])
						continue;
					counter++;
					sunLightSum += sunlightNeighbours[Lookup3[face][v][i]];
					torchLightSum += torchlightNeighbours[Lookup3[face][v][i]];
				}

			this->SunLight[v] = sunLightSum / counter;
			this->TorchLight[v] = torchLightSum / counter;
		}

		Flip = AO[0] + AO[2] + SunLight[1] + SunLight[3] + std::max(TorchLight[1], TorchLight[3]) >
			   AO[1] + AO[3] + SunLight[0] + SunLight[2] + std::max(TorchLight[0], TorchLight[2]);
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

	void ApplyMesh(Chunk *chk, const std::vector<ChunkRenderVertex> &mesh)
	{
		chk->VertexBuffer->SetDataVec(mesh);
		chk->VertexBuffer->SetAttributes(4,
										 Resource::ATTR_POSITION, 3,
										 Resource::ATTR_TEXCOORD, 3,
										 Resource::ATTR_CHUNK_FACE, 1,
										 Resource::ATTR_CHUNK_LIGHTING, 3);
	}

	void Meshing(World const *wld, const glm::ivec3 &ChunkPos, std::vector<ChunkRenderVertex> &result)
	{
		ChunkPtr thisChunk = wld->GetChunk(ChunkPos);
		const glm::ivec3 chkBase = ChunkPos * CHUNK_SIZE;

#define getBlock(x, y, z) (IsValidChunkPosition(x, y, z) ? thisChunk->GetBlock(x, y, z) : wld->GetBlock(chkBase + glm::ivec3(x, y, z)))
#define getSunLight(x, y, z) (IsValidChunkPosition(x, y, z) ? thisChunk->GetSunLight(x, y, z) : wld->GetSunLight(chkBase + glm::ivec3(x, y, z)))
#define getTorchLight(x, y, z) (IsValidChunkPosition(x, y, z) ? thisChunk->GetTorchLight(x, y, z) : wld->GetTorchLight(chkBase + glm::ivec3(x, y, z)))

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
			if((block = getBlock(pos[0], pos[1], pos[2])) == Blocks::Air)
				continue;

			bool initedNeighbours = false;

			for(short face=0; face<6; ++face)
			{
				int nei[3] = {pos[0], pos[1], pos[2]};
				nei[face>>1] += 1 - ((face&1)<<1);

				Block blockN = getBlock(nei[0], nei[1], nei[2]);

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
								neighbours[ind] = getBlock(it[0], it[1], it[2]);
								sunlightNeighbour[ind] = getSunLight(it[0], it[1], it[2]);
								torchlightNeighbour[ind] = getTorchLight(it[0], it[1], it[2]);
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
						const Block a = getBlock(x[0], x[1], x[2]);
						const Block b = getBlock(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

						bool outA = x[axis] < 0, outB = CHUNK_SIZE - 1 <= x[axis];

						if (!outA && ShowFace(a, b))
						{
							mask[counter] = a;
							lightMask[counter] = &LightingData[XYZ(x[0], x[1], x[2])][axis << 1];

						}
						else if (!outB && ShowFace(b, a))
						{
							mask[counter] = -b;
							lightMask[counter] = &LightingData[
									XYZ(x[0] + q[0], x[1] + q[1], x[2] + q[2])][(axis << 1) | 1];
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

							float vx = ChunkPos.x * CHUNK_SIZE + x[0],
									vy = ChunkPos.y * CHUNK_SIZE + x[1],
									vz = ChunkPos.z * CHUNK_SIZE + x[2];

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

							if(QuadLighting.Flip)
							{
								//11--------10
								//|       / |
								//|    /    |
								//| /       |
								//00--------01
								result.push_back(v00);
								result.push_back(v01);
								result.push_back(v10);

								result.push_back(v00);
								result.push_back(v10);
								result.push_back(v11);
							}
							else
							{
								//11--------10
								//| \       |
								//|    \    |
								//|       \ |
								//00--------01
								result.push_back(v01);
								result.push_back(v10);
								result.push_back(v11);

								result.push_back(v00);
								result.push_back(v01);
								result.push_back(v11);
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
	}

	void MeshingThreaded(const uint8_t (&Grid)[EXCHUNK_INFO_SIZE], const uint8_t (&Light)[EXCHUNK_INFO_SIZE],
						 const glm::ivec3 &ChunkPos, std::vector<ChunkRenderVertex> &result)
	{
#define getBlockThreaded(x, y, z) (Grid[ExXYZ((x), (y), (z))])
#define getLightThreaded(x, y, z) (Light[ExXYZ((x), (y), (z))])

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
			if((block = getBlockThreaded(pos[0], pos[1], pos[2])) == Blocks::Air)
				continue;

			bool initedNeighbours = false;

			for(short face=0; face<6; ++face)
			{
				int nei[3] = {pos[0], pos[1], pos[2]};
				nei[face>>1] += 1 - ((face&1)<<1);

				Block blockN = getBlockThreaded(nei[0], nei[1], nei[2]);

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
								neighbours[ind] = getBlockThreaded(it[0], it[1], it[2]);
								sunlightNeighbour[ind] = getLightThreaded(it[0], it[1], it[2]) >> 4;
								torchlightNeighbour[ind] = getLightThreaded(it[0], it[1], it[2]) & (uint8_t)0xF;
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
						const Block a = getBlockThreaded(x[0], x[1], x[2]);
						const Block b = getBlockThreaded(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

						bool outA = x[axis] < 0, outB = CHUNK_SIZE - 1 <= x[axis];

						if (!outA && ShowFace(a, b))
						{
							mask[counter] = a;
							lightMask[counter] = &LightingData[XYZ(x[0], x[1], x[2])][axis << 1];

						}
						else if (!outB && ShowFace(b, a))
						{
							mask[counter] = -b;
							lightMask[counter] = &LightingData[
									XYZ(x[0] + q[0], x[1] + q[1], x[2] + q[2])][(axis << 1) | 1];
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

							float vx = ChunkPos.x * CHUNK_SIZE + x[0],
									vy = ChunkPos.y * CHUNK_SIZE + x[1],
									vz = ChunkPos.z * CHUNK_SIZE + x[2];

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

							if(QuadLighting.Flip)
							{
								//11--------10
								//|       / |
								//|    /    |
								//| /       |
								//00--------01
								result.push_back(v00);
								result.push_back(v01);
								result.push_back(v10);

								result.push_back(v00);
								result.push_back(v10);
								result.push_back(v11);
							}
							else
							{
								//11--------10
								//| \       |
								//|    \    |
								//|       \ |
								//00--------01
								result.push_back(v01);
								result.push_back(v10);
								result.push_back(v11);

								result.push_back(v00);
								result.push_back(v01);
								result.push_back(v11);
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
	}

	void SunLightBFS(World *wld, std::queue<LightBFSNode> &Queue)
	{
		while(!Queue.empty())
		{
			LightBFSNode node = Queue.front();

			Queue.pop();

			for(short face=0; face<6; ++face)
			{
				LightBFSNode neighbour = node;
				neighbour.Pos = Util::FaceExtend(node.Pos, face);

				//deal with out chunk situations
				if(face>>1 == 1)
					if(neighbour.Pos.y < 0 || neighbour.Pos.y >= WORLD_HEIGHT_BLOCK)
						continue;
				if(neighbour.Value != 15 || face != Face::Bottom)
					neighbour.Value--;

				uint8_t neighbourLevel = wld->GetSunLight(neighbour.Pos);

				if(BlockMethods::LightCanPass(wld->GetBlock(neighbour.Pos)) &&
						neighbourLevel < neighbour.Value)
				{
					wld->SetSunLight(neighbour.Pos, neighbour.Value, true);
					Queue.push(neighbour);
				}
			}
		}
	}

	void SunLightBFSThreaded(const uint8_t (&Grid)[LICHUNK_INFO_SIZE],
							 uint8_t (&Result)[LICHUNK_INFO_SIZE],
							 int Highest, std::queue<LightBFSNode> &Queue)
	{
		while(!Queue.empty())
		{
			LightBFSNode node = Queue.front();
			Queue.pop();

			for(short face=0; face<6; ++face)
			{
				LightBFSNode neighbour = node;
				neighbour.Pos = Util::FaceExtend(node.Pos, face);

				int index = LiXYZ(neighbour.Pos);

				//deal with out chunk situations
				if(face>>1 != 1)
				{
					if(neighbour.Pos[face>>1] < -14 || neighbour.Pos[face>>1] >= CHUNK_SIZE + 14)
						continue;
				}
				else if(neighbour.Pos.y < 0 || neighbour.Pos.y >= Highest)
					continue;

				if(neighbour.Value != 15 || face != Face::Bottom)
					neighbour.Value--;

				if(BlockMethods::LightCanPass(Grid[index]) &&
						(Result[index] >> 4) < neighbour.Value)
				{
					//Result[index] = neighbour.Value << 4;
					Result[index] = (Result[index] & (uint8_t)0xF) | (neighbour.Value << 4);
					Queue.push(neighbour);
				}
			}
		}
	}

	void SunLightRemovalBFS(World *wld, std::queue<LightBFSNode> &RemovalQueue,
							std::queue<LightBFSNode> &SunLightQueue)
	{
		std::vector<glm::ivec3> SunLightVector;
		while(!RemovalQueue.empty())
		{
			LightBFSNode node = RemovalQueue.front();
			RemovalQueue.pop();

			for (short face = 0; face < 6; ++face)
			{
				glm::ivec3 neighbour = Util::FaceExtend(node.Pos, face);

				if (face >> 1 == 1)
					if (neighbour.y < 0 || neighbour.y >= WORLD_HEIGHT_BLOCK)
						continue;
				if (!BlockMethods::LightCanPass(wld->GetBlock(neighbour)))
					continue;
				uint8_t neighbourLevel = wld->GetSunLight(neighbour);
				if (neighbourLevel == 0)
					continue;

				if (face == Face::Bottom && node.Value == 15)
				{
					wld->SetSunLight(neighbour, 0, true);
					RemovalQueue.push({neighbour, 15});
				} else
				{
					if (neighbourLevel < node.Value)
					{
						wld->SetSunLight(neighbour, 0, true);
						RemovalQueue.push({neighbour, neighbourLevel});
					} else if (neighbourLevel >= node.Value)
						SunLightVector.push_back(neighbour);
				}
			}
		}

		for(const glm::ivec3 &i : SunLightVector)
		{
			uint8_t light = wld->GetSunLight(i);
			if(light)
				SunLightQueue.push({i, light});
		}
	}

	void TorchLightBFS(World *wld, std::queue<LightBFSNode> &Queue)
	{
		while(!Queue.empty())
		{
			LightBFSNode node = Queue.front();

			Queue.pop();

			LightBFSNode neighbour = node;
			neighbour.Value--;

			for(short face=0; face<6; ++face)
			{
				neighbour.Pos = Util::FaceExtend(node.Pos, face);

				if(face>>1 == 1)
					if(neighbour.Pos.y < 0 || neighbour.Pos.y >= WORLD_HEIGHT_BLOCK)
						continue;

				uint8_t neighbourLevel = wld->GetTorchLight(neighbour.Pos);

				if(BlockMethods::LightCanPass(wld->GetBlock(neighbour.Pos)) &&
				   neighbourLevel < neighbour.Value)
				{
					wld->SetTorchLight(neighbour.Pos, neighbour.Value, true);
					Queue.push(neighbour);
				}
			}
		}
	}
	void TorchLightBFSThreaded(const uint8_t (&Grid)[LICHUNK_INFO_SIZE],
							 uint8_t (&Result)[LICHUNK_INFO_SIZE],
							 std::queue<LightBFSNode> &Queue)
	{
		while(!Queue.empty())
		{
			LightBFSNode node = Queue.front();
			Queue.pop();

			LightBFSNode neighbour = node;
			neighbour.Value --;

			for(short face=0; face<6; ++face)
			{
				neighbour.Pos = Util::FaceExtend(node.Pos, face);

				int index = LiXYZ(neighbour.Pos);
				//deal with out chunk situations
				if(face>>1 != 1)
				{
					if(neighbour.Pos[face>>1] < -14 || neighbour.Pos[face>>1] >= CHUNK_SIZE + 14)
						continue;
				}
				else if(neighbour.Pos.y < 0 || neighbour.Pos.y >= WORLD_HEIGHT_BLOCK)
					continue;

				if(BlockMethods::LightCanPass(Grid[index]) &&
				   (Result[index] & (uint8_t)0x0F) < neighbour.Value)
				{
					Result[index] = (Result[index] & (uint8_t)0xF0) | neighbour.Value;
					Queue.push(neighbour);
				}
			}
		}
	}

	void TorchLightRemovalBFS(World *wld, std::queue<LightBFSNode> &RemovalQueue,
							  std::queue<LightBFSNode> &TorchLightQueue)
	{
		std::vector<glm::ivec3> TorchLightVector;
		while(!RemovalQueue.empty())
		{
			LightBFSNode node = RemovalQueue.front();
			RemovalQueue.pop();

			for (short face = 0; face < 6; ++face)
			{
				glm::ivec3 neighbour = Util::FaceExtend(node.Pos, face);

				if (face >> 1 == 1)
					if (neighbour.y < 0 || neighbour.y >= WORLD_HEIGHT_BLOCK)
						continue;
				uint8_t neighbourLevel = wld->GetTorchLight(neighbour);

				if (neighbourLevel == 0)
					continue;

				if (neighbourLevel < node.Value)
				{
					wld->SetTorchLight(neighbour, 0, true);
					RemovalQueue.push({neighbour, neighbourLevel});
				} else if (neighbourLevel >= node.Value)
					TorchLightVector.push_back(neighbour);
			}
		}

		for(const glm::ivec3 &i : TorchLightVector)
		{
			uint8_t light = wld->GetTorchLight(i);
			if(light)
				TorchLightQueue.push({i, light});
		}
	}
}
