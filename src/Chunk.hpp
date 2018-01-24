#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <iostream>
#include <atomic>
#include <vector>
#include <queue>

#include "Block.hpp"
#include "Setting.hpp"

#include <MyGL/VertexObject.hpp>

#include <glm/glm.hpp>

class WorldData;
struct ChunkRenderVertex { float X, Y, Z, U, V, Tex, Face, AO, SunLight, TorchLight; };

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
	glm::ivec3 Pos;
	DLightLevel Value;
};


class Chunk
{
private:
	Block Grid[CHUNK_INFO_SIZE];
	DLightLevel Light[CHUNK_INFO_SIZE];

	friend class ChunkLoadingInfo;
	friend class ChunkMeshingInfo;
	friend class ChunkInitialLightingInfo;
public:
	bool LoadedTerrain, InitializedMesh, InitializedLighting;

	glm::ivec3 Position;

	MyGL::VertexObjectPtr VertexBuffer;

	explicit Chunk(const glm::ivec3 &pos);

	inline void SetBlock(const glm::ivec3 &pos, Block b)
	{ Grid[XYZ(pos)] = b; }
	inline Block GetBlock(const glm::ivec3 &pos) const
	{ return Grid[XYZ(pos)]; }
	inline Block GetBlock(int x, int y, int z) const
	{ return Grid[XYZ(x, y, z)]; }
	inline DLightLevel GetTorchLight(int x, int y, int z) const
	{ return Light[XYZ(x, y, z)] & (uint8_t)0x0F; }
	inline DLightLevel GetTorchLight(const glm::ivec3 &pos) const
	{ return Light[XYZ(pos)] & (uint8_t)0x0F; }
	inline void SetTorchLight(const glm::ivec3 &pos, LightLevel val)
	{
		int index = XYZ(pos);
		Light[index] = (Light[index] & (uint8_t)0xF0) | val;
	}
	inline DLightLevel GetSunLight(int x, int y, int z) const
	{ return (Light[XYZ(x, y, z)] >> 4) & (uint8_t)0x0F; }
	inline DLightLevel GetSunLight(const glm::ivec3 &pos) const
	{ return (Light[XYZ(pos)] >> 4) & (uint8_t)0x0F; }
	inline void SetSunLight(const glm::ivec3 &pos, LightLevel val)
	{
		int index = XYZ(pos);
		Light[index] = (Light[index] & (uint8_t)0x0F) | (val << 4);
	}

	std::vector<ChunkRenderVertex> Mesh;
};
using ChunkPtr = Chunk*;

class ChunkInfo
{
public:
	virtual void Process()= 0;
};

class ChunkLoadingInfo : public ChunkInfo
{
private:
	Block Result[WORLD_HEIGHT * CHUNK_INFO_SIZE];
	glm::ivec2 Position;
	int Seed;
	WorldData &Data;

public:
	explicit ChunkLoadingInfo(const glm::ivec2 &pos, int seed, WorldData &data);
	void Process() override;
	void ApplyTerrain(ChunkPtr (&chk)[WORLD_HEIGHT]);
};




class ChunkMeshingInfo : public ChunkInfo
{
private:
	Block Grid[EXCHUNK_INFO_SIZE];
	DLightLevel Light[EXCHUNK_INFO_SIZE];
	glm::ivec3 Position;

	std::vector<ChunkRenderVertex> Result;

public:
	explicit ChunkMeshingInfo(ChunkPtr (&chk)[27]);
	void Process() override;
	void ApplyResult(ChunkPtr chk);
};



class ChunkInitialLightingInfo : public ChunkInfo
{
private:
	int Highest;
	Block Grid[LICHUNK_INFO_SIZE];
	DLightLevel Result[LICHUNK_INFO_SIZE];

	inline bool CanPass(int index)
	{ return BlockMethods::LightCanPass(Grid[index]); }

public:
	explicit ChunkInitialLightingInfo(ChunkPtr (&chk)[WORLD_HEIGHT * 9]);
	void Process() override;
	void ApplyLighting(ChunkPtr (&chk)[WORLD_HEIGHT]);
};


#endif // CHUNK_HPP
