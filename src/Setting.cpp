//
// Created by adamyuan on 1/23/18.
//
#include "Setting.hpp"
#include <libconfig.h++>
#include <thread>
#include <iostream>

#define CONFIG_FILE_NAME "config.cfg"
#define LOAD_THREAD_NUM_NAME "LoadingThreadsNum"
#define CHUNK_LOAD_RANGE_NAME "ChunkLoadRange"
#define CHUNK_DELETE_RANGE_NAME "ChunkDeleteRange"

namespace Setting
{
	int LoadingThreadsNum, ChunkLoadRange, ChunkDeleteRange;

	void LoadDefault()
	{
		LoadingThreadsNum = std::max(1u, std::thread::hardware_concurrency() - 1);
		ChunkLoadRange = 10;
		ChunkDeleteRange = 15;
	}

	void InitSetting()
	{
		libconfig::Config config;
		try
		{
			config.readFile(CONFIG_FILE_NAME);
			LoadingThreadsNum = config.lookup(LOAD_THREAD_NUM_NAME);
			ChunkLoadRange = config.lookup(CHUNK_LOAD_RANGE_NAME);
			ChunkDeleteRange = config.lookup(CHUNK_DELETE_RANGE_NAME);
		}
		catch(...)
		{
			std::cout << "ERROR WHEN LOADING CONFIG FILE: " << CONFIG_FILE_NAME << std::endl;
			LoadDefault();
		}
	}
	void SaveSetting()
	{
		libconfig::Config config;
		config.getRoot().add(LOAD_THREAD_NUM_NAME, libconfig::Setting::TypeInt) = LoadingThreadsNum;
		config.getRoot().add(CHUNK_LOAD_RANGE_NAME, libconfig::Setting::TypeInt) = ChunkLoadRange;
		config.getRoot().add(CHUNK_DELETE_RANGE_NAME, libconfig::Setting::TypeInt) = ChunkDeleteRange;

		try
		{
			config.writeFile(CONFIG_FILE_NAME);
		}
		catch(libconfig::FileIOException &e)
		{
			std::cout << "ERROR WHEN SAVING CONFIG FILE: " << CONFIG_FILE_NAME << std::endl;
		}
	}
}
