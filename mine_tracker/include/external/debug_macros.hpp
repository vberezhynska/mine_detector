#pragma once
#include <iostream>

#define ENABLE_DEBUG 1
#define ENABLE_LOG 0

#if ENABLE_DEBUG
#define DEBUG(msg) do { std::cout << "[DEBUG] " << msg << '\n'; } while(0)
#else
#define DEBUG(msg) do {} while(0)
#endif

#if ENABLE_LOG
#define LOG(msg) do { std::cout << "[LOG] " << msg << '\n'; } while(0)
#else
#define LOG(msg) do {} while(0)
#endif