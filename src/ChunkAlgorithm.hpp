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

struct ChunkRenderVertex;
struct LightBFSNode;

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

	extern void Meshing(const GetDataFunc &getBlock, const GetDataFunc &getLight, const glm::ivec3 &ChunkPos,
						std::vector<ChunkRenderVertex> &result);
	extern void SunLightBFS(const GetDataFunc &getLight, const SetDataFunc &setLight, const GetDataFunc &getBlock,
							std::queue<LightBFSNode> Queue,
							const glm::ivec2 &minXZRange = glm::ivec2(0), const glm::ivec2 &maxXZRange = glm::ivec2(0));
}


#endif //CHUNKALGORITHM_HPP
