#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <iostream>
#include <atomic>
#include <vector>
#include <queue>

#include "Block.hpp"
#include "Setting.hpp"

#include <mygl3/vertexobject.hpp>

#include <glm/glm.hpp>

class WorldData;
struct ChunkRenderVertex { float x, y, z, u, v, tex, face, ao, sun_light, torch_light; };

static inline int XYZ(const glm::ivec3 &pos)
{ return pos.x + (pos.y*CHUNK_SIZE + pos.z)*CHUNK_SIZE; }
static inline int XYZ(int x, int y, int z)
{ return x + (y*CHUNK_SIZE + z)*CHUNK_SIZE; }
static inline int LiXYZ(glm::ivec3 pos)
{ return pos.x + 14 + (pos.y * LICHUNK_SIZE + pos.z + 14) * LICHUNK_SIZE; }
static inline int LiXYZ(int x, int y, int z)
{ return x + 14 + (y * LICHUNK_SIZE + z + 14) * LICHUNK_SIZE; }
static inline int ExXYZ(int x, int y, int z)
{ return x + 1 + ((y + 1) * EXCHUNK_SIZE + z + 1)*EXCHUNK_SIZE; }
static inline bool IsValidChunkPosition(const glm::ivec3 &pos)
{ return !(pos.x < 0 || pos.x >= CHUNK_SIZE || pos.z < 0 || pos.z >= CHUNK_SIZE || pos.y < 0 || pos.y >= CHUNK_SIZE); }
static inline bool IsValidChunkPosition(int x, int y, int z)
{ return !(x < 0 || x >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE); }

struct LightBFSNode
{
	glm::ivec3 position;
	DLightLevel value;
};


class Chunk
{
private:
	Block grid_[CHUNK_INFO_SIZE];
	DLightLevel light_[CHUNK_INFO_SIZE];

	friend class ChunkLoadingInfo;
	friend class ChunkMeshingInfo;
	friend class ChunkLightingInfo;
public:
	bool loaded_terrain_, initialized_mesh_, initialized_lighting_;
	glm::ivec3 position_;
	mygl3::VertexObject<true> vertex_buffers_[2];

	explicit Chunk(const glm::ivec3 &pos);

	inline void SetBlock(const glm::ivec3 &pos, Block b)
	{ grid_[XYZ(pos)] = b; }
	inline Block GetBlock(const glm::ivec3 &pos) const
	{ return grid_[XYZ(pos)]; }
	inline Block GetBlock(int x, int y, int z) const
	{ return grid_[XYZ(x, y, z)]; }
	inline DLightLevel GetTorchLight(int x, int y, int z) const
	{ return light_[XYZ(x, y, z)] & (uint8_t)0x0F; }
	inline DLightLevel GetTorchLight(const glm::ivec3 &pos) const
	{ return light_[XYZ(pos)] & (uint8_t)0x0F; }
	inline void SetTorchLight(const glm::ivec3 &pos, LightLevel val)
	{
		int index = XYZ(pos);
		light_[index] = (light_[index] & (uint8_t)0xF0) | val;
	}
	inline DLightLevel GetSunLight(int x, int y, int z) const
	{ return (light_[XYZ(x, y, z)] >> 4) & (uint8_t)0x0F; }
	inline DLightLevel GetSunLight(const glm::ivec3 &pos) const
	{ return (light_[XYZ(pos)] >> 4) & (uint8_t)0x0F; }
	inline void SetSunLight(const glm::ivec3 &pos, LightLevel val)
	{
		int index = XYZ(pos);
		light_[index] = (light_[index] & (uint8_t)0x0F) | (val << 4);
	}
};
using ChunkPtr = Chunk*;

class ChunkInfo
{
public:
	std::atomic_bool done_;
	virtual void Process() = 0;
	ChunkInfo() : done_(false) {}
};

class ChunkLoadingInfo : public ChunkInfo
{
private:
	Block result_[WORLD_HEIGHT * CHUNK_INFO_SIZE];
	glm::ivec2 position_;
	int seed_;
	WorldData &world_data_;

public:
	explicit ChunkLoadingInfo(const glm::ivec2 &pos, int seed, WorldData &data);
	void Process() override;
	void ApplyTerrain(ChunkPtr (&chk)[WORLD_HEIGHT]);
};

class ChunkMeshingInfo : public ChunkInfo
{
private:
	Block grid_[EXCHUNK_INFO_SIZE];
	DLightLevel light_[EXCHUNK_INFO_SIZE];
	glm::ivec3 position_;

	std::vector<ChunkRenderVertex> result_vertices_[2];
	std::vector<unsigned int> result_indices_[2];

public:
	explicit ChunkMeshingInfo(ChunkPtr (&chk)[27]);
	void Process() override;
	void ApplyMesh(ChunkPtr chk);
};

class ChunkLightingInfo : public ChunkInfo
{
private:
	Block grid_[LICHUNK_INFO_SIZE];
	DLightLevel result_[LICHUNK_INFO_SIZE];

	inline bool CanPass(int index)
	{ return BlockMethods::LightCanPass(grid_[index]); }

public:
	explicit ChunkLightingInfo(ChunkPtr (&chk)[WORLD_HEIGHT * 9]);
	void Process() override;
	void ApplyLighting(ChunkPtr (&chk)[WORLD_HEIGHT]);
};


#endif // CHUNK_HPP
