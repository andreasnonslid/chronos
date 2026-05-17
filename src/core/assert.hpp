#pragma once
#include <cassert>

#define CHRONOS_ASSERT(cond) assert(cond)

#ifdef NDEBUG
#define CHRONOS_UNREACHABLE() __builtin_unreachable()
#else
#define CHRONOS_UNREACHABLE() (assert(false && "unreachable"), __builtin_unreachable())
#endif
