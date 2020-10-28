#pragma once

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#   define LIB211_HAS_POSIX
#endif

#include "lib211_version.h"
#include "lib211_alloc.h"
#include "lib211_io.h"
#include "lib211_test.h"

#ifdef LIB211_HAS_POSIX
#   include "lib211_program_test.h"
#endif
