/* mp4h -- A macro processor for HTML documents
   Copyright 2000-2001, Denis Barbier
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

#define XP_UNIX
#include <mozilla/jsapi.h>

#define mp4h_macro_table                mozjs_LTX_mp4h_macro_table
#define mp4h_init_module                mozjs_LTX_mp4h_init_module
#define mp4h_finish_module              mozjs_LTX_mp4h_finish_module

DECLARE(mozjs);         /* declare test as implementing a builtin */

#undef DECLARE

/* The table of builtins defined by this module - just one */

builtin mp4h_macro_table[] =
{
  /* name             container   expand    function
                                attributes                      */

  { "javascript",        TRUE,    TRUE,   mozjs },
  { 0,                  FALSE,    FALSE,  0 },
};

static JSClass global_class = {
      "global",0,
      JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
      JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
};

void
mp4h_init_module(struct obstack *obs)
{
  return;
}

void
mp4h_finish_module(void)
{
  return;
}

/* This test function prints its attributes in reverse order.  */
static void
mozjs (MP4H_BUILTIN_ARGS)
{
  JSRuntime *rt;
  JSContext *cx;
  JSObject *global;
  jsval rval;
  JSString *jstr;
  JSScript *jscr;
  JSBool ok;
  char *str;
  char *script;

  rt = JS_NewRuntime(0x100000);
  if (!rt)
    {
      MP4HERROR ((warning_status, 0,
          _("Warning:%s:%d: could'nt runtime environment\n%s"),
                CURRENT_FILE_LINE));
      return;
    }
  cx = JS_NewContext(rt, 0x1000);
  if (!cx)
    {
      MP4HERROR ((warning_status, 0,
          _("Warning:%s:%d: could'nt create context\n%s"),
                CURRENT_FILE_LINE));
      return;
    }
  global = JS_NewObject(cx, &global_class, NULL, NULL);
  if (!global)
    {
      MP4HERROR ((warning_status, 0,
          _("Warning:%s:%d: could'nt create global classes\n%s"),
                CURRENT_FILE_LINE));
      return;
    }
  JS_SetGlobalObject(cx, global);

  script = xstrdup(ARGBODY);
  remove_special_chars (script, TRUE);

  printf("SCRIPT:%d:%s:\n", strlen(script), script);
#if 1
  /*
  ok = JS_EvaluateScript(cx, global, script, strlen(script),
    */
  ok = JS_EvaluateScript(cx, global, "print(\"ok\")", 11,
          current_file, current_line, &rval);
#else
  jscr = JS_CompileScript(cx, global, script, strlen(script),
          current_file, current_line);
  if (!jscr)
    {
      MP4HERROR ((warning_status, 0,
          _("Warning:%s:%d: could not compile script:\n%s"),
              CURRENT_FILE_LINE));
    }
  ok = JS_ExecuteScript(cx, global, jscr, &rval);
  JS_DestroyScript(cx, jscr);
#endif
  if (!ok)
    {
      MP4HERROR ((warning_status, 0,
          _("Warning:%s:%d: execution of JavaScript failed:\n"),
              CURRENT_FILE_LINE));
    }
  jstr = JS_ValueToString(cx, rval);
  str = JS_GetStringBytes(jstr);
  obstack_1grow (obs, CHAR_BGROUP);
  obstack_grow (obs, str, strlen (str));
  obstack_1grow (obs, CHAR_EGROUP);
  xfree ((voidstar) script);
}
