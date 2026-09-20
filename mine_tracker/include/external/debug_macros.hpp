#pragma once
#include <iostream>

#define ENABLE_DEBUG 1
#define ENABLE_LOG 0

#if ENABLE_DEBUG
#define DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl;
#else
#define DEBUG(msg)
#endif

#if ENABLE_LOG
#define LOG(msg) std::cout << "[LOG] " << msg << std::endl;
#else
#define LOG(msg)
#endif