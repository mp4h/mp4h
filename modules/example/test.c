/* mp4h -- A macro processor for HTML documents
   Copyright 2000-2003, Denis Barbier
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

#define MP4H_MODULE
#include <mp4h.h>
#undef MP4H_MODULE

#define mp4h_macro_table                test_LTX_mp4h_macro_table
#define mp4h_init_module                test_LTX_mp4h_init_module
#define mp4h_finish_module              test_LTX_mp4h_finish_module

module_init_t mp4h_init_module;         /* initialisation function */
module_finish_t mp4h_finish_module;     /* cleanup function */

DECLARE(test);         /* declare test as implementing a builtin */

#undef DECLARE

/* The table of builtins defined by this module - just one */

builtin mp4h_macro_table[] =
{
  /* name             container   expand    function
                                attributes                      */

  { "test",             FALSE,    TRUE,   test },
  { 0,                  FALSE,    FALSE,  0 },
};

void
mp4h_init_module(struct obstack *obs)
{
  char *s = "Test module loaded.";
  obstack_grow (obs, s, strlen(s));
}

void
mp4h_finish_module(void)
{
  return;
}

/* The functions for builtins can be static */

/* This test function prints its attributes in reverse order.  */
static void
test (MP4H_BUILTIN_ARGS)
{
  const char *separator;
  register int i;

  separator = predefined_attribute ("separator", &argc, argv, FALSE);
  for (i=argc-1; i>0; i--)
    {
      if (i < argc-1 && separator)
        obstack_grow (obs, separator, strlen (separator));

      obstack_1grow (obs, CHAR_BGROUP);
      obstack_grow (obs, ARG (i), strlen (ARG (i)));
      obstack_1grow (obs, CHAR_EGROUP);
    }
}
