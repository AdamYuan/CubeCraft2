#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <cinttypes>
#include "Util.hpp"

#define BLOCKS_NUM 10
#define BLOCKS_TEXTURE_NUM 11

enum Blocks
{
	Air = 0,
	Dirt = 1,
	Grass = 2,
	Stone = 3,
	Bedrock = 4,
	Wood = 5,
	Leaves = 6,
	Glowstone = 7,
	Plank = 8,
	Glass = 9
};

//they are of the same type but different name
typedef uint8_t Block;
typedef uint8_t LightLevel;
//bits: SSSSTTTT
typedef uint8_t DLightLevel;

struct BlockProperty
{
	char name[10];
	bool transparent, lightCanPass, hitbox;
	LightLevel light_level;
	int textures[6];
};

namespace BlockMethods
{
	extern const BlockProperty BlockProperties[BLOCKS_NUM];

	inline const char *GetName(Block block) { return BlockProperties[block].name; }
	inline int GetTexture(Block block, Face face) { return BlockProperties[block].textures[face]; }
	inline bool IsTransparent(Block block) { return BlockProperties[block].transparent; }
	inline bool HaveHitbox(Block block) { return BlockProperties[block].hitbox; }
	inline LightLevel GetLightLevel(Block block) { return BlockProperties[block].light_level; }
	inline bool LightCanPass(Block block) { return BlockProperties[block].lightCanPass; }
	inline AABB GetBlockAABB(const glm::ivec3 &pos) { return {(glm::vec3)pos, (glm::vec3)pos + glm::vec3(1.0f)}; }
}

#endif // BLOCK_HPP
