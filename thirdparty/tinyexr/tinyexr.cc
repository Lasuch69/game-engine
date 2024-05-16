#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// -- Edit Start --
#define TINYEXR_USE_MINIZ 0
#include <zlib.h>
// -- Edit End --

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"
