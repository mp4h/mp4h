/*
**  src/version.c -- Version Information for mp4h (syntax: C/C++)
**  [automatically generated and maintained by GNU shtool]
*/

#ifdef _SRC_VERSION_C_AS_HEADER_

#ifndef _SRC_VERSION_C_
#define _SRC_VERSION_C_

#define MP4H_VERSION 0x1000193

typedef struct {
    const int   v_hex;
    const char *v_short;
    const char *v_long;
    const char *v_tex;
    const char *v_gnu;
    const char *v_web;
    const char *v_sccs;
    const char *v_rcs;
} mp4h_version_t;

extern mp4h_version_t mp4h_version;

#endif /* _SRC_VERSION_C_ */

#else /* _SRC_VERSION_C_AS_HEADER_ */

#define _SRC_VERSION_C_AS_HEADER_
#include "src/version.c"
#undef  _SRC_VERSION_C_AS_HEADER_

mp4h_version_t mp4h_version = {
    0x1000193,
    "1.0a403",
    "1.0a403 (04-Jul-2000)",
    "This is mp4h, Version 1.0a403 (04-Jul-2000)",
    "mp4h 1.0a403 (04-Jul-2000)",
    "mp4h/1.0a403",
    "@(#)mp4h 1.0a403 (04-Jul-2000)",
    "$Id: mp4h 1.0a403 (04-Jul-2000) $"
};

#endif /* _SRC_VERSION_C_AS_HEADER_ */

