#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#define CHUNK_SIZE 32 //should be greater than 14
#define WORLD_HEIGHT 8

#define DAY_TIME 4096.0f //seconds

#define GRAVITY 35.f
#define WALK_SPEED 5.f
#define FLY_SPEED 10.f
#define JUMP_DIST 9.f

#define WALK_FOVY 60

#define RENDER_ORDER_UPDATE_FREQUENCY 20 //frames

constexpr int WORLD_HEIGHT_BLOCK = WORLD_HEIGHT*CHUNK_SIZE;

constexpr int CHUNK_INFO_SIZE = CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE;

constexpr int EXCHUNK_SIZE = CHUNK_SIZE + 2;
constexpr int EXCHUNK_INFO_SIZE = EXCHUNK_SIZE*EXCHUNK_SIZE*EXCHUNK_SIZE;

constexpr int LICHUNK_SIZE = CHUNK_SIZE + 28;
constexpr int LICHUNK_INFO_SIZE = LICHUNK_SIZE*LICHUNK_SIZE*WORLD_HEIGHT_BLOCK;

#define SAVES_DIR "saves/"
#define WORLD_DIR(x) (std::string(SAVES_DIR) + (x) + "/")
#define SEED_FILE_NAME "seed"
#define DB_NAME "block.db"
#define DATA_FILE_NAME "data"

namespace Setting
{
	extern int LoadingThreadsNum, ChunkLoadRange, ChunkDeleteRange;

	extern void InitSetting();
	extern void SaveSetting();
}

#endif // SETTINGS_HPP
