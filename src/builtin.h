/* MP4H -- A macro-processor for HTML documents
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

#include "mp4h.h"
#include <math.h>
#include <locale.h>

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

const char *skip_space (const char *arg);

typedef struct var_stack var_stack;
struct var_stack
{
    var_stack *prev;
    char *text;
};

/*  Stack for variables.  */
var_stack *vs;

boolean numeric_arg (token_data *macro, const char *arg, boolean, int *valuep);
void shipout_int (struct obstack *obs, int val);
void shipout_string (struct obstack *obs, const char *s, int len);

#endif /* BUILTIN_H */
