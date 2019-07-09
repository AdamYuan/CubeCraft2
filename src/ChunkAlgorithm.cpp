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
			const uint8_t (&sunlight_neighbours)[27],
			const uint8_t (&torchlight_neighbours)[27])
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

			this->ao[v] = (LightLevel)(!trans[0] && !trans[2] ? 0 : 3 - !trans[0] - !trans[1] - !trans[2]);

			//smooth the Light using the average value

			uint8_t counter = 1,
					sunLightSum = sunlight_neighbours[Lookup1[face]],
					torchLightSum = torchlight_neighbours[Lookup1[face]];
			if(canPass[0] || canPass[2])
				for(int i=0; i<3; ++i)
				{
					if(!canPass[i])
						continue;
					counter++;
					sunLightSum += sunlight_neighbours[Lookup3[face][v][i]];
					torchLightSum += torchlight_neighbours[Lookup3[face][v][i]];
				}

			this->sunlight[v] = sunLightSum / counter;
			this->torchlight[v] = torchLightSum / counter;
		}

		flip = ao[0] + ao[2] + sunlight[1] + sunlight[3] + std::max(torchlight[1], torchlight[3]) >
			   ao[1] + ao[3] + sunlight[0] + sunlight[2] + std::max(torchlight[0], torchlight[2]);
	}

	bool FaceLighting::operator==(const FaceLighting &f) const
	{
		for(int i=0; i<4; ++i)
		{
			if(ao[i] != f.ao[i])
				return false;
			if(sunlight[i] != f.sunlight[i])
				return false;
			if(torchlight[i] != f.torchlight[i])
				return false;
		}
		return true;
	}

	bool FaceLighting::operator!=(const FaceLighting &f) const
	{
		for(int i=0; i<4; ++i)
		{
			if(ao[i] != f.ao[i])
				return true;
			if(sunlight[i] != f.sunlight[i])
				return true;
			if(torchlight[i] != f.torchlight[i])
				return true;
		}
		return false;
	}

	void ApplyMesh(Chunk *chk, const std::vector<ChunkRenderVertex> (&vertices)[2],
				   const std::vector<unsigned int> (&indices)[2])
	{
		for(short t=0; t<=1; ++t)
		{
			chk->vertex_buffers_[t].SetData(vertices[t], GL_STATIC_DRAW);
			chk->vertex_buffers_[t].SetIndices(indices[t], GL_STATIC_DRAW);
		}
	}

	void Meshing(World const *wld, const glm::ivec3 &chunk_pos,
				 std::vector<ChunkRenderVertex> (&result_vertices)[2],
				 std::vector<unsigned int> (&result_indices)[2])
	{
		ChunkPtr this_chunk = wld->GetChunk(chunk_pos);
		const glm::ivec3 chk_base = chunk_pos * CHUNK_SIZE;

		MeshingImpl(chunk_pos,
					[&](int x, int y, int z) -> uint8_t
					{
						return IsValidChunkPosition(x, y, z) ? this_chunk->GetBlock(x, y, z)
															 : wld->GetBlock(chk_base + glm::ivec3(x, y, z));
					},
					[&](int x, int y, int z) -> uint8_t
					{
						return IsValidChunkPosition(x, y, z) ? this_chunk->GetSunLight(x, y, z)
															 : wld->GetSunLight(chk_base + glm::ivec3(x, y, z));
					},
					[&](int x, int y, int z) -> uint8_t
					{
						return IsValidChunkPosition(x, y, z) ? this_chunk->GetTorchLight(x, y, z)
															 : wld->GetTorchLight(chk_base + glm::ivec3(x, y, z));
					},
					result_vertices, result_indices
		);
	}

	void MeshingThreaded(const uint8_t (&grid)[EXCHUNK_INFO_SIZE], const uint8_t (&light)[EXCHUNK_INFO_SIZE],
						 const glm::ivec3 &chunk_pos,
						 std::vector<ChunkRenderVertex> (&result_vertices)[2],
						 std::vector<unsigned int> (&result_indices)[2])
	{
		MeshingImpl(chunk_pos,
					[&](int x, int y, int z) -> uint8_t
					{ return grid[ExXYZ(x, y, z)]; },
					[&](int x, int y, int z) -> uint8_t
					{ return light[ExXYZ(x, y, z)] >> 4; },
					[&](int x, int y, int z) -> uint8_t
					{ return light[ExXYZ(x, y, z)] & (uint8_t)0x0F; },
					result_vertices, result_indices
		);
	}

	void SunLightBFS(World *wld, std::queue<LightBFSNode> &que)
	{
		while(!que.empty())
		{
			LightBFSNode node = que.front();

			que.pop();

			for(short face=0; face<6; ++face)
			{
				LightBFSNode neighbour = node;
				neighbour.position = util::FaceExtend(node.position, face);

				//deal with out chunk situations
				if(face>>1 == 1)
					if(neighbour.position.y < 0 || neighbour.position.y >= WORLD_HEIGHT_BLOCK)
						continue;
				if(neighbour.value != 15 || face != Face::Bottom)
					neighbour.value--;

				uint8_t neighbourLevel = wld->GetSunLight(neighbour.position);

				if(BlockMethods::LightCanPass(wld->GetBlock(neighbour.position)) &&
				   neighbourLevel < neighbour.value)
				{
					wld->SetSunLight(neighbour.position, neighbour.value, true);
					que.push(neighbour);
				}
			}
		}
	}

	void SunLightBFSThreaded(const uint8_t (&grid)[LICHUNK_INFO_SIZE],
							 uint8_t (&result)[LICHUNK_INFO_SIZE],
							 int highest, std::queue<LightBFSNode> &que)
	{
		while(!que.empty())
		{
			LightBFSNode node = que.front();
			que.pop();

			for(short face=0; face<6; ++face)
			{
				LightBFSNode neighbour = node;
				neighbour.position = util::FaceExtend(node.position, face);

				int index = LiXYZ(neighbour.position);

				//deal with out chunk situations
				if(face>>1 != 1)
				{
					if(neighbour.position[face>>1] < -14 || neighbour.position[face>>1] >= CHUNK_SIZE + 14)
						continue;
				}
				else if(neighbour.position.y < 0 || neighbour.position.y >= highest)
					continue;

				if(neighbour.value != 15 || face != Face::Bottom)
					neighbour.value--;

				if(BlockMethods::LightCanPass(grid[index]) &&
				   (result[index] >> 4) < neighbour.value)
				{
					//Result[index] = neighbour.Value << 4;
					result[index] = (result[index] & (uint8_t)0xF) | (neighbour.value << 4);
					que.push(neighbour);
				}
			}
		}
	}

	void SunLightRemovalBFS(World *wld, std::queue<LightBFSNode> &removal_queue,
							std::queue<LightBFSNode> &sunlight_queue)
	{
		std::vector<glm::ivec3> SunLightVector;
		while(!removal_queue.empty())
		{
			LightBFSNode node = removal_queue.front();
			removal_queue.pop();

			for (short face = 0; face < 6; ++face)
			{
				glm::ivec3 neighbour = util::FaceExtend(node.position, face);

				if (face >> 1 == 1)
					if (neighbour.y < 0 || neighbour.y >= WORLD_HEIGHT_BLOCK)
						continue;
				if (!BlockMethods::LightCanPass(wld->GetBlock(neighbour)))
					continue;
				uint8_t neighbourLevel = wld->GetSunLight(neighbour);
				if (neighbourLevel == 0)
					continue;

				if (face == Face::Bottom && node.value == 15)
				{
					wld->SetSunLight(neighbour, 0, true);
					removal_queue.push({neighbour, 15});
				} else
				{
					if (neighbourLevel < node.value)
					{
						wld->SetSunLight(neighbour, 0, true);
						removal_queue.push({neighbour, neighbourLevel});
					} else if (neighbourLevel >= node.value)
						SunLightVector.push_back(neighbour);
				}
			}
		}

		for(const glm::ivec3 &i : SunLightVector)
		{
			uint8_t light = wld->GetSunLight(i);
			if(light)
				sunlight_queue.push({i, light});
		}
	}

	void TorchLightBFS(World *wld, std::queue<LightBFSNode> &que)
	{
		while(!que.empty())
		{
			LightBFSNode node = que.front();

			que.pop();

			LightBFSNode neighbour = node;
			neighbour.value--;

			for(short face=0; face<6; ++face)
			{
				neighbour.position = util::FaceExtend(node.position, face);

				if(face>>1 == 1)
					if(neighbour.position.y < 0 || neighbour.position.y >= WORLD_HEIGHT_BLOCK)
						continue;

				uint8_t neighbourLevel = wld->GetTorchLight(neighbour.position);

				if(BlockMethods::LightCanPass(wld->GetBlock(neighbour.position)) &&
				   neighbourLevel < neighbour.value)
				{
					wld->SetTorchLight(neighbour.position, neighbour.value, true);
					que.push(neighbour);
				}
			}
		}
	}
	void TorchLightBFSThreaded(const uint8_t (&grid)[LICHUNK_INFO_SIZE],
							   uint8_t (&result)[LICHUNK_INFO_SIZE],
							   std::queue<LightBFSNode> &que)
	{
		while(!que.empty())
		{
			LightBFSNode node = que.front();
			que.pop();

			LightBFSNode neighbour = node;
			neighbour.value --;

			for(short face=0; face<6; ++face)
			{
				neighbour.position = util::FaceExtend(node.position, face);

				int index = LiXYZ(neighbour.position);
				//deal with out chunk situations
				if(face>>1 != 1)
				{
					if(neighbour.position[face>>1] < -14 || neighbour.position[face>>1] >= CHUNK_SIZE + 14)
						continue;
				}
				else if(neighbour.position.y < 0 || neighbour.position.y >= WORLD_HEIGHT_BLOCK)
					continue;

				if(BlockMethods::LightCanPass(grid[index]) &&
				   (result[index] & (uint8_t)0x0F) < neighbour.value)
				{
					result[index] = (result[index] & (uint8_t)0xF0) | neighbour.value;
					que.push(neighbour);
				}
			}
		}
	}

	void TorchLightRemovalBFS(World *wld, std::queue<LightBFSNode> &removal_queue,
							  std::queue<LightBFSNode> &torchlight_queue)
	{
		std::vector<glm::ivec3> TorchLightVector;
		while(!removal_queue.empty())
		{
			LightBFSNode node = removal_queue.front();
			removal_queue.pop();

			for (short face = 0; face < 6; ++face)
			{
				glm::ivec3 neighbour = util::FaceExtend(node.position, face);

				if (face >> 1 == 1)
					if (neighbour.y < 0 || neighbour.y >= WORLD_HEIGHT_BLOCK)
						continue;
				uint8_t neighbourLevel = wld->GetTorchLight(neighbour);

				if (neighbourLevel == 0)
					continue;

				if (neighbourLevel < node.value)
				{
					wld->SetTorchLight(neighbour, 0, true);
					removal_queue.push({neighbour, neighbourLevel});
				} else if (neighbourLevel >= node.value)
					TorchLightVector.push_back(neighbour);
			}
		}

		for(const glm::ivec3 &i : TorchLightVector)
		{
			uint8_t light = wld->GetTorchLight(i);
			if(light)
				torchlight_queue.push({i, light});
		}
	}

	void MeshingImpl(const glm::ivec3 &chunk_pos,
					 const std::function<uint8_t(int, int, int)> &get_block_func,
					 const std::function<uint8_t(int, int, int)> &get_sunlight_func,
					 const std::function<uint8_t(int, int, int)> &get_torchlight_func,
					 std::vector<ChunkRenderVertex> (&result_vertices)[2],
					 std::vector<unsigned int> (&result_indices)[2])
	{
		//generate LightingInfo(including AO, sunlight and torchlight)
		FaceLighting lighting_data[CHUNK_INFO_SIZE][6] = {};

		Block neighbours[27];
		LightLevel sunlight_neighbour[27], torchlight_neighbour[27];
		for(int index=0; index<CHUNK_INFO_SIZE; ++index)
		{
			int pos[3], _i = index;
			pos[1] = _i / (CHUNK_SIZE*CHUNK_SIZE);
			_i %= CHUNK_SIZE*CHUNK_SIZE;
			pos[2] = _i / CHUNK_SIZE;
			pos[0] = _i % CHUNK_SIZE;

			Block block;
			if((block = get_block_func(pos[0], pos[1], pos[2])) == Blocks::Air)
				continue;

			bool inited_neighbours = false;

			for(short face=0; face<6; ++face)
			{
				int nei[3] = {pos[0], pos[1], pos[2]};
				nei[face>>1] += 1 - ((face&1)<<1);

				Block block_neighbour = get_block_func(nei[0], nei[1], nei[2]);

				if(!ShowFace(block, block_neighbour))
					continue;

				if(!inited_neighbours)
				{
					inited_neighbours = true;
					int it[3], ind = 0;
					for(it[0] = pos[0]-1; it[0] <= pos[0]+1; ++it[0])
						for(it[1] = pos[1]-1; it[1] <= pos[1]+1; ++it[1])
							for(it[2] = pos[2]-1; it[2] <= pos[2]+1; ++it[2], ++ind)
							{
								neighbours[ind] = get_block_func(it[0], it[1], it[2]);
								sunlight_neighbour[ind] = get_sunlight_func(it[0], it[1], it[2]);
								torchlight_neighbour[ind] = get_torchlight_func(it[0], it[1], it[2]);
							}
				}

				lighting_data[index][face].SetValues(face, neighbours, sunlight_neighbour, torchlight_neighbour);
			}
		}

		//meshing
		unsigned current_index[2] = {0, 0};
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			const std::size_t u = (axis + 1) % 3;
			const std::size_t v = (axis + 2) % 3;

			int x[3] = {0}, q[3] = {0}, mask[CHUNK_SIZE*CHUNK_SIZE];
			FaceLighting *light_mask[CHUNK_SIZE*CHUNK_SIZE];

			// Compute mask
			q[axis] = 1;
			for (x[axis] = -1; x[axis] < CHUNK_SIZE;)
			{
				std::size_t counter = 0;
				for (x[v] = 0; x[v] < CHUNK_SIZE; ++x[v]) {
					for (x[u] = 0; x[u] < CHUNK_SIZE; ++x[u], ++counter) {
						const Block a = get_block_func(x[0], x[1], x[2]);
						const Block b = get_block_func(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

						bool outA = x[axis] < 0, outB = CHUNK_SIZE - 1 <= x[axis];

						if (!outA && ShowFace(a, b))
						{
							mask[counter] = a;
							light_mask[counter] = &lighting_data[XYZ(x[0], x[1], x[2])][axis << 1];

						}
						else if (!outB && ShowFace(b, a))
						{
							mask[counter] = -b;
							light_mask[counter] = &lighting_data[
									XYZ(x[0] + q[0], x[1] + q[1], x[2] + q[2])][(axis << 1) | 1];
						}
						else
						{
							mask[counter] = 0;
							light_mask[counter] = nullptr;
						}
					}
				}

				++x[axis];

				// Generate mesh for mask using lexicographic ordering
				std::size_t width = 0, height = 0;

				counter = 0;
				for (std::size_t j = 0; j < CHUNK_SIZE; ++j) {
					for (std::size_t i = 0; i < CHUNK_SIZE;) {
						int quad_type = mask[counter];
						if (quad_type) {
							const FaceLighting &quad_lighting = *light_mask[counter];
							// Compute width
							for (width = 1;
								 quad_type == mask[counter + width] &&
								 light_mask[counter + width] && quad_lighting == *light_mask[counter + width] &&
								 i + width < CHUNK_SIZE;
								 ++width);

							// Compute height
							bool done = false;
							for (height = 1; j + height < CHUNK_SIZE; ++height) {
								for (std::size_t k = 0; k < width; ++k) {
									size_t ind = counter + k + height * CHUNK_SIZE;
									if (quad_type != mask[ind] ||
										!light_mask[ind] || quad_lighting != *light_mask[ind]) {
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

							auto quad_face = (Face) ((axis << 1) | (quad_type <= 0));

							if (quad_type > 0) {
								dv[v] = height;
								du[u] = width;
							} else {
								quad_type = -quad_type;
								du[v] = height;
								dv[u] = width;
							}

							int quad_tex = BlockMethods::GetTexture((uint8_t)quad_type, quad_face);

							float vx = chunk_pos.x * CHUNK_SIZE + x[0],
									vy = chunk_pos.y * CHUNK_SIZE + x[1],
									vz = chunk_pos.z * CHUNK_SIZE + x[2];

							ChunkRenderVertex
									v00 = {vx, vy, vz,
										   (float) (du[u] + dv[u]), (float) (du[v] + dv[v]),
										   (float) quad_tex, (float) quad_face,
										   (float) quad_lighting.ao[0],
										   (float) quad_lighting.sunlight[0], (float) quad_lighting.torchlight[0]},

									v01 = {vx + du[0], vy + du[1], vz + du[2],
										   (float) dv[u], (float) dv[v],
										   (float) quad_tex, (float) quad_face,
										   (float) quad_lighting.ao[1],
										   (float) quad_lighting.sunlight[1], (float) quad_lighting.torchlight[1]},

									v10 = {vx + du[0] + dv[0], vy + du[1] + dv[1], vz + du[2] + dv[2],
										   0.0f, 0.0f,
										   (float) quad_tex, (float) quad_face,
										   (float) quad_lighting.ao[2],
										   (float) quad_lighting.sunlight[2], (float) quad_lighting.torchlight[2]},

									v11 = {vx + dv[0], vy + dv[1], vz + dv[2],
										   (float) du[u], (float) du[v],
										   (float) quad_tex, (float) quad_face,
										   (float) quad_lighting.ao[3],
										   (float) quad_lighting.sunlight[3], (float) quad_lighting.torchlight[3]};

							if (quad_face == Face::Left) {
								std::swap(v00.u, v01.v);
								std::swap(v00.v, v01.u);
								std::swap(v11.u, v10.v);
								std::swap(v11.v, v10.u);
							} else if (quad_face == Face::Right) {
								std::swap(v11.u, v11.v);
								std::swap(v01.u, v01.v);
								std::swap(v00.u, v00.v);
							} else if (quad_face == Face::Front) {
								std::swap(v00.u, v01.u);
								std::swap(v00.v, v01.v);
								std::swap(v11.u, v10.u);
								std::swap(v11.v, v10.v);
							}

							bool trans = BlockMethods::IsTransparent((uint8_t)quad_type);

							result_vertices[trans].push_back(v00);
							result_vertices[trans].push_back(v01);
							result_vertices[trans].push_back(v10);
							result_vertices[trans].push_back(v11);

							if(quad_lighting.flip)
							{
								//11--------10
								//|       / |
								//|    /    |
								//| /       |
								//00--------01
								result_indices[trans].push_back(current_index[trans]);
								result_indices[trans].push_back(current_index[trans] + 1);
								result_indices[trans].push_back(current_index[trans] + 2);

								result_indices[trans].push_back(current_index[trans]);
								result_indices[trans].push_back(current_index[trans] + 2);
								result_indices[trans].push_back(current_index[trans] + 3);
							}
							else
							{
								//11--------10
								//| \       |
								//|    \    |
								//|       \ |
								//00--------01
								result_indices[trans].push_back(current_index[trans] + 1);
								result_indices[trans].push_back(current_index[trans] + 2);
								result_indices[trans].push_back(current_index[trans] + 3);

								result_indices[trans].push_back(current_index[trans]);
								result_indices[trans].push_back(current_index[trans] + 1);
								result_indices[trans].push_back(current_index[trans] + 3);
							}
							current_index[trans] += 4;

							for (std::size_t b = 0; b < width; ++b)
								for (std::size_t a = 0; a < height; ++a) {
									size_t ind = counter + b + a * CHUNK_SIZE;
									mask[ind] = 0;
									light_mask[ind] = nullptr;
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
}
