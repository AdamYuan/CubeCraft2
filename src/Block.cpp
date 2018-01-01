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
	bool Transparent, LightCanPass, Hitbox;
	LightLevel Light;
	int Textures[6];
};

namespace BlockMethods
{
	const static BlockProperty BlockProperties[BLOCKS_NUM] =
			{
					{true,	true,	false,	0,	{}},
					{false,	false,	true,	0,	{0, 0, 0, 0, 0, 0}},
					{false,	false,	true,	0,	{1, 1, 2, 0, 1, 1}},
					{false,	false,	true,	0,	{3, 3, 3, 3, 3, 3}}
			};

	int GetTexture(Block block, Face face)
	{
		return BlockProperties[block].Textures[face];
	}

	bool IsTransparent(Block block)
	{
		return BlockProperties[block].Transparent;
	}

	bool HaveHitbox(Block block)
	{
		return BlockProperties[block].Hitbox;
	}

	LightLevel GetLightLevel(Block block)
	{
		return BlockProperties[block].Light;
	}

	bool LightCanPass(Block block)
	{
		return BlockProperties[block].LightCanPass;
	}

	AABB GetBlockAABB(const glm::ivec3 &pos)
	{
		return {(glm::vec3)pos, (glm::vec3)pos + glm::vec3(1.0f)};
	}
}
