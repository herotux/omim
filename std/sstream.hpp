#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <sstream>

using std::istringstream;
using std::ostringstream;
using std::endl;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
