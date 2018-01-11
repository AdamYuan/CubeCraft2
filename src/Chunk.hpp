#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <iostream>
#include <functional>
#include <vector>
#include <queue>

#include "Block.hpp"
#include "Settings.hpp"

#include <MyGL/VertexObject.hpp>

#include <glm/glm.hpp>


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

	static inline int XYZ(const glm::ivec3 &pos)
	{ return pos.x + (pos.y*CHUNK_SIZE + pos.z)*CHUNK_SIZE; }
	static inline int XYZ(int x, int y, int z)
	{ return x + (y*CHUNK_SIZE + z)*CHUNK_SIZE; }
	static inline bool IsValidPosition(const glm::ivec3 &pos)
	{ return !(pos.x < 0 || pos.x >= CHUNK_SIZE || pos.z < 0 || pos.z >= CHUNK_SIZE || pos.y < 0 || pos.y >= CHUNK_SIZE); }

	glm::ivec3 Position;

	MyGL::VertexObjectPtr VertexBuffer;

	explicit Chunk(const glm::ivec3 &pos);
	void SetBlock(const glm::ivec3 &pos, Block b);
	Block GetBlock(const glm::ivec3 &pos) const;
};
using ChunkPtr = Chunk*;

class ChunkInfo
{
public:
	bool Done = false;
	virtual void Process()= 0;
};

class ChunkLoadingInfo : public ChunkInfo
{
private:
	Block Result[WORLD_HEIGHT * CHUNK_INFO_SIZE];
	glm::ivec2 Position;

public:
	explicit ChunkLoadingInfo(const glm::ivec2 &pos);
	void Process() override;
	void ApplyTerrain(ChunkPtr (&chk)[WORLD_HEIGHT]);
};


struct ChunkRenderVertex
{
	float X, Y, Z, U, V, Tex, Face, AO, SunLight, TorchLight;
};


class ChunkMeshingInfo : public ChunkInfo
{
private:
	Block Grid[EXCHUNK_INFO_SIZE];
	DLightLevel Light[EXCHUNK_INFO_SIZE];
	glm::ivec3 Position;

	static inline int ExXYZ(int x, int y, int z)
	{ return x + 1 + ((y + 1) * EXCHUNK_SIZE + z + 1)*EXCHUNK_SIZE; }
	Block GetBlock(int x, int y, int z)
	{ return Grid[ExXYZ(x, y, z)]; }
	DLightLevel GetLight(int x, int y, int z)
	{ return Light[ExXYZ(x, y, z)]; }

	std::vector<ChunkRenderVertex> Result;

public:
	explicit ChunkMeshingInfo(ChunkPtr (&chk)[27]);
	void Process() override;
	void ApplyMesh(ChunkPtr chk);
};


class ChunkInitialLightingInfo : public ChunkInfo
{
private:
	int Highest;
	Block Grid[LICHUNK_INFO_SIZE];
	DLightLevel Result[LICHUNK_INFO_SIZE];

	inline bool CanPass(int index)
	{ return BlockMethods::LightCanPass(Grid[index]); }
	inline int LiXYZ(glm::ivec3 pos)
	{ return pos.x + 14 + (pos.y * LICHUNK_SIZE + pos.z + 14) * LICHUNK_SIZE; }
	inline int LiXYZ(int x, int y, int z)
	{ return x + 14 + (y * LICHUNK_SIZE + z + 14) * LICHUNK_SIZE; }
	inline Block GetBlock(int x, int y, int z)
	{ return Grid[LiXYZ(x, y, z)]; }
	inline DLightLevel GetLight(int x, int y, int z)
	{ return Result[LiXYZ(x, y, z)]; }
	inline void SetLight(int x, int y, int z, DLightLevel val)
	{ Result[LiXYZ(x, y, z)] = val; }

public:
	explicit ChunkInitialLightingInfo(ChunkPtr (&chk)[WORLD_HEIGHT * 9]);
	void Process() override;
	void ApplyLighting(ChunkPtr (&chk)[WORLD_HEIGHT]);
};

#endif // CHUNK_HPP
