#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <queue>
using std::queue;
using std::priority_queue;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
