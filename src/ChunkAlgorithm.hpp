//
// Created by adamyuan on 1/11/18.
//

#ifndef CHUNKALGORITHM_HPP
#define CHUNKALGORITHM_HPP

#include <cinttypes>
#include <functional>
#include <vector>
#include <queue>
#include <glm/glm.hpp>
#include "Block.hpp"
#include "Setting.hpp"

struct ChunkRenderVertex;
struct LightBFSNode;
struct Chunk;
struct World;

namespace ChunkAlgorithm
{
	struct FaceLighting
	{
		uint8_t sunlight[4], torchlight[4], ao[4];
		bool flip;
		void SetValues(
				int face,
				const uint8_t (&neighbours)[27],
				const uint8_t (&sunlight_neighbours)[27],
				const uint8_t (&torchlight_neighbours)[27]);
		bool operator== (const FaceLighting &f) const;
		bool operator!= (const FaceLighting &f) const;
	};

	inline bool ShowFace(uint8_t now, uint8_t neighbour)
	{
		bool trans = BlockMethods::IsTransparent(now), trans_neighbour = BlockMethods::IsTransparent(neighbour);
		if(!now)
			return false;
		if(!trans && !trans_neighbour)
			return false;
		if(trans && trans_neighbour && now != neighbour && neighbour == Blocks::Glass)
			return true;
		return !(trans && neighbour);
	}

	extern void MeshingImpl(const glm::ivec3 &chunk_pos,
							const std::function<uint8_t(int, int, int)> &get_block_func,
							const std::function<uint8_t(int, int, int)> &get_sunlight_func,
							const std::function<uint8_t(int, int, int)> &get_torchlight_func,
							std::vector<ChunkRenderVertex> (&result_vertices)[2],
							std::vector<unsigned int> (&result_indices)[2]);
	extern void Meshing(World const *wld, const glm::ivec3 &chunk_pos,
						std::vector<ChunkRenderVertex> (&result_vertices)[2],
						std::vector<unsigned int> (&result_indices)[2]);
	extern void MeshingThreaded(const uint8_t (&grid)[EXCHUNK_INFO_SIZE],
								const uint8_t (&light)[EXCHUNK_INFO_SIZE],
								const glm::ivec3 &chunk_pos,
								std::vector<ChunkRenderVertex> (&result_vertices)[2],
								std::vector<unsigned int> (&result_indices)[2]);
	extern void ApplyMesh(Chunk *chk, bool trans, const std::vector<ChunkRenderVertex> (&vertices)[2],
						  const std::vector<unsigned int> (&indices)[2]);
	extern void SunLightBFS(World *wld, std::queue<LightBFSNode> &que);
	extern void SunLightBFSThreaded(const uint8_t (&grid)[LICHUNK_INFO_SIZE], uint8_t (&result)[LICHUNK_INFO_SIZE],
									int highest, std::queue<LightBFSNode> &que);
	extern void SunLightRemovalBFS(World *wld, std::queue<LightBFSNode> &removal_queue,
								   std::queue<LightBFSNode> &sunlight_queue);

	extern void TorchLightBFS(World *wld, std::queue<LightBFSNode> &que);
	extern void TorchLightBFSThreaded(const uint8_t (&grid)[LICHUNK_INFO_SIZE], uint8_t (&result)[LICHUNK_INFO_SIZE],
									std::queue<LightBFSNode> &que);
	extern void TorchLightRemovalBFS(World *wld, std::queue<LightBFSNode> &removal_queue,
								   std::queue<LightBFSNode> &torchlight_queue);
}


#endif //CHUNKALGORITHM_HPP
