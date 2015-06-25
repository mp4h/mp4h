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

#define XP_UNIX
#include <js/jsapi.h>

#define mp4h_macro_table                mozjs_LTX_mp4h_macro_table
#define mp4h_init_module                mozjs_LTX_mp4h_init_module
#define mp4h_finish_module              mozjs_LTX_mp4h_finish_module

DECLARE(mozjs_javascript);

#undef DECLARE

/* The table of builtins defined by this module - just one */

builtin mp4h_macro_table[] =
{
  /* name             container   expand    function
                                attributes                      */

  { "javascript",        TRUE,    TRUE,   mozjs_javascript },
  { 0,                  FALSE,    FALSE,  0 },
};

static JSRuntime *rt;
static JSContext *cx;
static JSObject *global;
static JSClass global_class = {
      "global",0,
      JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
      JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
  };

/* obs holds a pointer to the current obstack for output. It is
   initialised from the argument to the javascript tag. */
static struct obstack *javascript_obs;

static JSBool
my_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSString *jstr;
  char *s;

  jstr = JS_ValueToString(cx, argv[0]);
  s = JS_GetStringBytes(jstr);
  obstack_grow(javascript_obs, s, strlen(s));
  obstack_1grow(javascript_obs, '\n');
  return JS_TRUE;
}

static JSFunctionSpec my_functions[] = {
      {"print",         my_print,       1},
      {0}
};

void
mp4h_init_module(struct obstack *obs)
{
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
  JS_InitStandardClasses (cx, global);
  JS_DefineFunctions(cx, global, my_functions);
}

void
mp4h_finish_module(void)
{
  JS_DestroyContext(cx);
  JS_Finish(rt);
}

/* Evaluates javascript commands  */
static void
mozjs_javascript (MP4H_BUILTIN_ARGS)
{
  jsval rval;
  JSBool ok;
  char *script;

  javascript_obs = obs;

  script = xstrdup(ARGBODY);
  remove_special_chars (script, TRUE);

  ok = JS_EvaluateScript(cx, global, script, strlen(script),
          CURRENT_FILE_LINE, &rval);
  if (!ok)
    {
      MP4HERROR ((warning_status, 0,
          _("Warning:%s:%d: execution of JavaScript failed:\n"),
              CURRENT_FILE_LINE));
    }
  xfree ((voidstar) script);
}

