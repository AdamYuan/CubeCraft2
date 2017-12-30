#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <cinttypes>
#include "Type.hpp"

#define BLOCKS_NUM 4
#define BLOCKS_TEXTURE_NUM 4

enum Blocks
{
	Air = 0,
	Dirt = 1,
	Grass = 2,
	Stone = 3
};

typedef uint8_t Block;
typedef uint8_t LightLevel;
//bits: SSSSTTTT
typedef uint8_t DLightLevel;

enum LightType
{
	Torchlight = 0,
	Sunlight = 1
};

namespace BlockMethods
{

extern int GetTexture(Block block, Face face);
extern bool IsTransparent(Block block);
extern bool LightCanPass(Block block);
extern LightLevel GetLightLevel(Block block);

}

#endif // BLOCK_HPP
