//
// Created by adamyuan on 1/23/18.
//
#include "Setting.hpp"
#include <thread>

const unsigned THREADS_SUPPORT = std::max(1u, std::thread::hardware_concurrency() - 1);
const unsigned MAX_THREAD_NUM = THREADS_SUPPORT;

