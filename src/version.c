/*
**  src/version.c -- Version Information for mp4h (syntax: C/C++)
**  [automatically generated and maintained by GNU shtool]
*/

#ifdef _SRC_VERSION_C_AS_HEADER_

#ifndef _SRC_VERSION_C_
#define _SRC_VERSION_C_

#define MP4HVERSION 0x1000192

typedef struct {
    const int   v_hex;
    const char *v_short;
    const char *v_long;
    const char *v_tex;
    const char *v_gnu;
    const char *v_web;
    const char *v_sccs;
    const char *v_rcs;
} mp4hversion_t;

extern mp4hversion_t mp4hversion;

#endif /* _SRC_VERSION_C_ */

#else /* _SRC_VERSION_C_AS_HEADER_ */

#define _SRC_VERSION_C_AS_HEADER_
#include "src/version.c"
#undef  _SRC_VERSION_C_AS_HEADER_

mp4hversion_t mp4hversion = {
    0x1000192,
    "1.0a402",
    "1.0a402 (02-Jul-2000)",
    "This is mp4h, Version 1.0a402 (02-Jul-2000)",
    "mp4h 1.0a402 (02-Jul-2000)",
    "mp4h/1.0a402",
    "@(#)mp4h 1.0a402 (02-Jul-2000)",
    "$Id: mp4h 1.0a402 (02-Jul-2000) $"
};

#endif /* _SRC_VERSION_C_AS_HEADER_ */

