#include "Block.hpp"

/*enum Blocks
{
	Air = 0,
	Dirt = 1,
	Grass = 2,
	Stone = 3
};*/

struct BlockProperty
{
	bool Transparent, LightCanPass;
	LightLevel Light;
	int Textures[6];
};

namespace BlockMethods
{

static BlockProperty BlockProperties[BLOCKS_NUM] =
{
	{true,	true,	0,	{}},
	{false,	false,	0,	{0, 0, 0, 0, 0, 0}},
	{false,	false,	0,	{1, 1, 2, 0, 1, 1}},
	{false,	false,	0,	{3, 3, 3, 3, 3, 3}}
};

int GetTexture(Block block, Face face)
{
	return BlockProperties[block].Textures[face];
}

bool IsTransparent(Block block)
{
	return BlockProperties[block].Transparent;
}

LightLevel GetLightLevel(Block block)
{
	return BlockProperties[block].Light;
}

bool LightCanPass(Block block)
{
	return BlockProperties[block].LightCanPass;
}

}
