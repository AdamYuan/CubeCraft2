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
#include "Settings.hpp"

struct ChunkRenderVertex;
struct LightBFSNode;
struct Chunk;
struct World;

typedef std::function<uint8_t(int, int, int)> GetDataFunc;
typedef std::function<void(int, int, int, uint8_t)> SetDataFunc;


namespace ChunkAlgorithm
{
	struct FaceLighting
	{
		uint8_t SunLight[4], TorchLight[4], AO[4];
		void SetValues(
				int face,
				const uint8_t (&neighbours)[27],
				const uint8_t (&sunlightNeighbours)[27],
				const uint8_t (&torchlightNeighbours)[27]);
		bool operator== (const FaceLighting &f) const;
		bool operator!= (const FaceLighting &f) const;
	};

	inline bool ShowFace(uint8_t now, uint8_t neighbour)
	{
		bool trans = BlockMethods::IsTransparent(now), transN = BlockMethods::IsTransparent(neighbour);
		if(!now)
			return false;
		if(!trans && !transN)
			return false;
		return !(trans && neighbour);
	}

	extern void Meshing(World const *wld, const glm::ivec3 &ChunkPos, std::vector<ChunkRenderVertex> &result);
	extern void MeshingThreaded(const uint8_t (&Grid)[EXCHUNK_INFO_SIZE],
								const uint8_t (&Light)[EXCHUNK_INFO_SIZE],
								const glm::ivec3 &ChunkPos,
								std::vector<ChunkRenderVertex> &result);
	extern void ApplyMesh(Chunk *chk, const std::vector<ChunkRenderVertex> &mesh);
	extern void SunLightBFS(World *wld, std::queue<LightBFSNode> &Queue);
	extern void SunLightBFSThreaded(const uint8_t (&Grid)[LICHUNK_INFO_SIZE], uint8_t (&Result)[LICHUNK_INFO_SIZE],
									int Highest, std::queue<LightBFSNode> &Queue);
	extern void SunLightRemovalBFS(World *wld, std::queue<LightBFSNode> &RemovalQueue,
								   std::queue<LightBFSNode> &SunLightQueue);
}


#endif //CHUNKALGORITHM_HPP
