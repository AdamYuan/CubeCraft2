#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#define CHUNK_LOADING_RANGE 10
#define CHUNK_DELETING_RANGE 15

#define CHUNK_SIZE 32 //should be greater than 14
#define WORLD_HEIGHT 8

#define MAX_ADDITION_THREAD 3

constexpr int WORLD_HEIGHT_BLOCK = WORLD_HEIGHT * CHUNK_SIZE;

constexpr int CHUNK_INFO_SIZE = CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE;

constexpr int EXCHUNK_SIZE = CHUNK_SIZE + 2;
constexpr int EXCHUNK_INFO_SIZE = EXCHUNK_SIZE*EXCHUNK_SIZE*EXCHUNK_SIZE;

#endif // SETTINGS_HPP
