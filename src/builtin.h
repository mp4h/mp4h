/* MP4H -- A macro processor for HTML documents
   Copyright 1999, Denis Barbier
   All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is a work based on GNU m4 version 1.4n. Below is the
   original copyright.
*/
/* GNU m4 -- A simple macro processor
   Copyright (C) 1998 Free Software Foundation, Inc.
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* Declarations for builtin macros.  */ 

#ifndef BUILTIN_H
#define BUILTIN_H 1

#if defined(HAVE_DIRENT_H) && defined(HAVE_SYS_STAT_H) && \
    defined(HAVE_SYS_TYPES_H) && defined(HAVE_PWD_H) && \
    defined(HAVE_GRP_H)
#define HAVE_FILE_FUNCS 1
#else
#undef HAVE_FILE_FUNCS
#endif

#include "mp4h.h"
#include "regex.h"

#ifdef HAVE_FILE_FUNCS
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#endif

#include <sys/times.h>
#include <math.h>
#include <locale.h>
#include <time.h>

#define _AS_HEADER
#include "version.c"

#define MP4H_BUILTIN_ARGS struct obstack *obs, int argc, token_data **argv, \
                            read_type expansion
#define MP4H_BUILTIN_PROTO struct obstack *, int, token_data **, read_type

#define DECLARE(name) \
  static void name __P ((MP4H_BUILTIN_PROTO))

#define ARG(i)  (i<argc ? TOKEN_DATA_TEXT (argv[i]) : "")
#define ARGBODY (TOKEN_DATA_TEXT (argv[argc]))

enum mathop_type
{
  MATHOP_ADD,                   /* addition */
  MATHOP_SUB,                   /* substraction */
  MATHOP_MUL,                   /* multiplication */
  MATHOP_DIV,                   /* division */
  MATHOP_MIN,                   /* minimum */
  MATHOP_MAX,                   /* maximum */
  MATHOP_MOD,                   /* modulus */
};

enum mathrel_type
{
  MATHREL_GT,
  MATHREL_LT,
  MATHREL_EQ,
  MATHREL_NEQ,
};

typedef enum mathop_type mathop_type;
typedef enum mathrel_type mathrel_type;

const char *skip_space __P ((const char *arg));

typedef struct var_stack var_stack;
struct var_stack
{
    var_stack *prev;
    char *text;
};

/*  Stack for variables.  */
var_stack *vs;

boolean numeric_arg __P ((token_data *macro, const char *arg, boolean, int *valuep));
void shipout_int __P ((struct obstack *obs, int val));

#endif /* BUILTIN_H */
