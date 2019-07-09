#include "Resource.hpp"
#include "Block.hpp"
#include <unordered_map>

#define TEXTURE_PATH(str) "resources/texture/"#str
#define SHADER_PATH(str) "resources/shader/"#str

extern void generateIcosphereMesh(size_t lod, std::vector<uint32_t>& indices, std::vector<glm::vec3>& positions);

namespace Resource
{
	mygl3::Shader ChunkShader, LineShader, SkyShader, SunShader, BgShader;
	mygl3::Texture2D SkyTexture, SunTexture, MoonTexture;
	mygl3::Texture2DArray ChunkTexture;
	mygl3::VertexObject<false> CrosshairObject, SunObject, MoonObject, BgObject;
	mygl3::VertexObject<true> SkyObject;

	GLint ChunkShader_sampler, ChunkShader_skySampler, ChunkShader_camera, ChunkShader_viewDistance, ChunkShader_dayTime, ChunkShader_dayLight, ChunkShader_matrix, ChunkShader_selection;
	GLint LineShader_matrix;
	GLint SkyShader_matrix, SkyShader_sampler, SkyShader_dayTime;
	GLint SunShader_matrix, SunShader_sampler;
	GLint BgShader_matrix, BgShader_sampler;


	void InitResources()
	{
		ChunkShader.Initialize();
		ChunkShader.LoadFromFile(SHADER_PATH(block.frag), GL_FRAGMENT_SHADER);
		ChunkShader.LoadFromFile(SHADER_PATH(block.vert), GL_VERTEX_SHADER);
		ChunkShader_sampler = ChunkShader.GetUniform("sampler");
		ChunkShader_skySampler = ChunkShader.GetUniform("skySampler");
		ChunkShader_camera = ChunkShader.GetUniform("camera");
		ChunkShader_viewDistance = ChunkShader.GetUniform("viewDistance");
		ChunkShader_matrix = ChunkShader.GetUniform("matrix");
		ChunkShader_dayLight = ChunkShader.GetUniform("dayLight");
		ChunkShader_dayTime = ChunkShader.GetUniform("dayTime");
		ChunkShader_selection = ChunkShader.GetUniform("selection");

		LineShader.Initialize();
		LineShader.LoadFromFile(SHADER_PATH(line.frag), GL_FRAGMENT_SHADER);
		LineShader.LoadFromFile(SHADER_PATH(line.vert), GL_VERTEX_SHADER);
		LineShader_matrix = LineShader.GetUniform("matrix");

		SkyShader.Initialize();
		SkyShader.LoadFromFile(SHADER_PATH(sky.frag), GL_FRAGMENT_SHADER);
		SkyShader.LoadFromFile(SHADER_PATH(sky.vert), GL_VERTEX_SHADER);
		SkyShader_matrix = SkyShader.GetUniform("matrix");
		SkyShader_dayTime = SkyShader.GetUniform("dayTime");
		SkyShader_sampler = SkyShader.GetUniform("sampler");

		SunShader.Initialize();
		SunShader.LoadFromFile(SHADER_PATH(sun.frag), GL_FRAGMENT_SHADER);
		SunShader.LoadFromFile(SHADER_PATH(sun.vert), GL_VERTEX_SHADER);
		SunShader_matrix = SunShader.GetUniform("matrix");
		SunShader_sampler = SunShader.GetUniform("sampler");

		BgShader.Initialize();
		BgShader.LoadFromFile(SHADER_PATH(bg.frag), GL_FRAGMENT_SHADER);
		BgShader.LoadFromFile(SHADER_PATH(bg.vert), GL_VERTEX_SHADER);
		BgShader_matrix = BgShader.GetUniform("matrix");
		BgShader_sampler = BgShader.GetUniform("sampler");

		ChunkTexture.Initialize();
		ChunkTexture.Load(mygl3::ImageLoader(TEXTURE_PATH(blocks.png), BLOCKS_TEXTURE_NUM).GetInfo(), true);
		ChunkTexture.SetSizeFilter(GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST);
		ChunkTexture.SetWrapFilter(GL_REPEAT);

		SkyTexture.Initialize();
		SkyTexture.Load(mygl3::ImageLoader(TEXTURE_PATH(sky.png)).GetInfo());
		SkyTexture.SetSizeFilter(GL_LINEAR, GL_LINEAR);
		SkyTexture.SetWrapFilter(GL_CLAMP_TO_EDGE);

		SunTexture.Initialize();
		SunTexture.Load(mygl3::ImageLoader(TEXTURE_PATH(sun.png)).GetInfo());
		SunTexture.SetSizeFilter(GL_NEAREST, GL_NEAREST);
		SunTexture.SetWrapFilter(GL_CLAMP_TO_EDGE);

		MoonTexture.Initialize();
		MoonTexture.Load(mygl3::ImageLoader(TEXTURE_PATH(moon.png)).GetInfo());
		MoonTexture.SetSizeFilter(GL_NEAREST, GL_NEAREST);
		MoonTexture.SetWrapFilter(GL_CLAMP_TO_EDGE);


		static constexpr float ch_size = 10.0, ch_width = 1.0f;
		static constexpr float crosshair_vertices[] = {-ch_width, -ch_size, -ch_width, ch_size, ch_width, -ch_size,
												  ch_width, -ch_size, -ch_width, ch_size, ch_width, ch_size,
												  -ch_size, ch_width, ch_size, -ch_width, -ch_size, -ch_width,
												  ch_size, ch_width, ch_size, -ch_width, -ch_size, ch_width,
												  -ch_width, -ch_width, -ch_width, ch_width, ch_width, -ch_width,
												  ch_width, -ch_width, -ch_width, ch_width, ch_width, ch_width};
		CrosshairObject.Initialize();
		CrosshairObject.SetData(crosshair_vertices, 36, GL_STATIC_DRAW);
		CrosshairObject.SetAttributes(ATTR_POSITION, 2);

		std::vector<uint32_t> sphereIndices;
		std::vector<glm::vec3> spherePositions;
		generateIcosphereMesh(3, sphereIndices, spherePositions);
		SkyObject.Initialize();
		SkyObject.SetData(spherePositions, GL_STATIC_DRAW);
		SkyObject.SetIndices(sphereIndices, GL_STATIC_DRAW);
		SkyObject.SetAttributes(ATTR_POSITION, 3);

		static constexpr float sun_size = 0.4f;
		static constexpr float sun_vertices[] = {
				sun_size, -1.0f, -sun_size, 1.0f, 0.0f,
				-sun_size, -1.0f, -sun_size, 0.0f, 0.0f,
				-sun_size, -1.0f, sun_size, 0.0f, 1.0f,
				sun_size, -1.0f, -sun_size, 1.0f, 0.0f,
				-sun_size, -1.0f, sun_size, 0.0f, 1.0f,
				sun_size, -1.0f, sun_size, 1.0f, 1.0f
		};
		SunObject.Initialize();
		SunObject.SetData(sun_vertices, 30, GL_STATIC_DRAW);
		SunObject.SetAttributes(ATTR_POSITION, 3, ATTR_TEXCOORD, 2);

		static constexpr float moon_size = sun_size / 4.0f;
		static constexpr float moon_vertices[] = {
				-moon_size, 1.0f, moon_size, 0.0f, 1.0f,
				-moon_size, 1.0f, -moon_size, 0.0f, 0.0f,
				moon_size, 1.0f, -moon_size, 1.0f, 0.0f,

				moon_size, 1.0f, moon_size, 1.0f, 1.0f,
				-moon_size, 1.0f, moon_size, 0.0f, 1.0f,
				moon_size, 1.0f, -moon_size, 1.0f, 0.0f
		};
		MoonObject.Initialize();
		MoonObject.SetData(moon_vertices, 30, GL_STATIC_DRAW);
		MoonObject.SetAttributes(ATTR_POSITION, 3, ATTR_TEXCOORD, 2);


		static constexpr float bg_time = 0.8f;
		static constexpr float bg_vertices[] = {
				-1.0f,  1.0f,  bg_time, 1.0f,
				-1.0f, -1.0f,  bg_time, 0.4f,
				1.0f, -1.0f,  bg_time, 0.4f,

				-1.0f,  1.0f,  bg_time, 1.0f,
				1.0f, -1.0f,  bg_time, 0.4f,
				1.0f,  1.0f,  bg_time, 1.0f
		};
		BgObject.Initialize();
		BgObject.SetData(bg_vertices, 24, GL_STATIC_DRAW);
		BgObject.SetAttributes(ATTR_POSITION, 2, ATTR_TEXCOORD, 2);
	}
}

/// Seed of Andromeda Icosphere Generator
/// Written by Frank McCoy
/// Use it for whatever, but remember where you got it from.
///
/// SEE: https://www.seedofandromeda.com/blogs/49-procedural-gas-giant-rendering-with-gpu-noise
/// SEE: http://pastebin.com/aFdWi5eQ
const static float GOLDEN_RATIO = 1.61803398875f;

const static int NUM_ICOSOHEDRON_VERTICES = 12;
const static glm::vec3 ICOSOHEDRON_VERTICES[12] = {
		glm::vec3(-1.0f, GOLDEN_RATIO, 0.0f),
		glm::vec3(1.0f, GOLDEN_RATIO, 0.0f),
		glm::vec3(-1.0f, -GOLDEN_RATIO, 0.0f),
		glm::vec3(1.0f, -GOLDEN_RATIO, 0.0f),

		glm::vec3(0.0f, -1.0f, GOLDEN_RATIO),
		glm::vec3(0.0f, 1.0f, GOLDEN_RATIO),
		glm::vec3(0.0f, -1.0, -GOLDEN_RATIO),
		glm::vec3(0.0f, 1.0f, -GOLDEN_RATIO),

		glm::vec3(GOLDEN_RATIO, 0.0f, -1.0f),
		glm::vec3(GOLDEN_RATIO, 0.0f, 1.0f),
		glm::vec3(-GOLDEN_RATIO, 0.0f, -1.0f),
		glm::vec3(-GOLDEN_RATIO, 0.0, 1.0f)
};

const static int NUM_ICOSOHEDRON_INDICES = 60;
const static uint32_t ICOSOHEDRON_INDICES[60] = {
		0, 11, 5,
		0, 5, 1,
		0, 1, 7,
		0, 7, 10,
		0, 10, 11,

		1, 5, 9,
		5, 11, 4,
		11, 10, 2,
		10, 7, 6,
		7, 1, 8,

		3, 9, 4,
		3, 4, 2,
		3, 2, 6,
		3, 6, 8,
		3, 8, 9,

		4, 9, 5,
		2, 4, 11,
		6, 2, 10,
		8, 6, 7,
		9, 8, 1
};

// Hash functions for the unordered map
class Vec3KeyFuncs {
public:
	size_t operator()(const glm::vec3& k)const {
		return std::hash<float>()(k.x) ^ std::hash<float>()(k.y) ^ std::hash<float>()(k.z);
	}

	bool operator()(const glm::vec3& a, const glm::vec3& b)const {
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}
};

inline glm::vec3 findMidpoint(glm::vec3 vertex1, glm::vec3 vertex2) {
	return glm::normalize(glm::vec3((vertex1.x + vertex2.x) / 2.0f, (vertex1.y + vertex2.y) / 2.0f, (vertex1.z + vertex2.z) / 2.0f));
}

/// Generates an icosphere with radius 1.0f.
/// @param lod: Number of subdivisions
/// @param indices: Resulting indices for use with glDrawElements
/// @param positions: Resulting vertex positions
void generateIcosphereMesh(size_t lod, std::vector<uint32_t>& indices, std::vector<glm::vec3>& positions) {
	std::vector<uint32_t> newIndices;
	newIndices.reserve(256);

	std::unordered_map<glm::vec3, uint32_t, Vec3KeyFuncs, Vec3KeyFuncs> vertexLookup;

	indices.resize(NUM_ICOSOHEDRON_INDICES);
	for (uint32_t i = 0; i < NUM_ICOSOHEDRON_INDICES; i++) {
		indices[i] = ICOSOHEDRON_INDICES[i];
	}
	positions.resize(NUM_ICOSOHEDRON_VERTICES);
	for (uint32_t i = 0; i < NUM_ICOSOHEDRON_VERTICES; i++) {
		positions[i] = glm::normalize(ICOSOHEDRON_VERTICES[i]);
		vertexLookup[glm::normalize(ICOSOHEDRON_VERTICES[i])] = i;
	}

	for (size_t i = 0; i < (size_t)lod; i++) {
		for (size_t j = 0; j < indices.size(); j += 3) {
			/*
			j
			mp12   mp13
			j+1    mp23   j+2
			*/
			// Defined in counter clockwise order
			glm::vec3 vertex1 = positions[indices[j + 0]];
			glm::vec3 vertex2 = positions[indices[j + 1]];
			glm::vec3 vertex3 = positions[indices[j + 2]];

			glm::vec3 midPoint12 = findMidpoint(vertex1, vertex2);
			glm::vec3 midPoint23 = findMidpoint(vertex2, vertex3);
			glm::vec3 midPoint13 = findMidpoint(vertex1, vertex3);

			uint32_t mp12Index;
			uint32_t mp23Index;
			uint32_t mp13Index;

			auto iter = vertexLookup.find(midPoint12);
			if (iter != vertexLookup.end()) { // It is in the map
				mp12Index = iter->second;
			} else { // Not in the map
				mp12Index = (uint32_t)positions.size();
				positions.push_back(midPoint12);
				vertexLookup[midPoint12] = mp12Index;
			}

			iter = vertexLookup.find(midPoint23);
			if (iter != vertexLookup.end()) { // It is in the map
				mp23Index = iter->second;
			} else { // Not in the map
				mp23Index = (uint32_t)positions.size();
				positions.push_back(midPoint23);
				vertexLookup[midPoint23] = mp23Index;
			}

			iter = vertexLookup.find(midPoint13);
			if (iter != vertexLookup.end()) { // It is in the map
				mp13Index = iter->second;
			} else { // Not in the map
				mp13Index = (uint32_t)positions.size();
				positions.push_back(midPoint13);
				vertexLookup[midPoint13] = mp13Index;
			}

			newIndices.push_back(indices[j]);
			newIndices.push_back(mp12Index);
			newIndices.push_back(mp13Index);

			newIndices.push_back(mp12Index);
			newIndices.push_back(indices[j + 1]);
			newIndices.push_back(mp23Index);

			newIndices.push_back(mp13Index);
			newIndices.push_back(mp23Index);
			newIndices.push_back(indices[j + 2]);

			newIndices.push_back(mp12Index);
			newIndices.push_back(mp23Index);
			newIndices.push_back(mp13Index);
		}
		indices.swap(newIndices);
		newIndices.clear();
	}
}
