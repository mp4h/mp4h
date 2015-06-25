/* mp4h-javascript -- a JavaScript extension to mp4h
   Copyright 2001, Anders Dinsen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
*/

#define MP4H_MODULE
#include <mp4h.h>
#undef MP4H_MODULE

#include <js.h>
#include "mp4h-njs.h"

#define mp4h_macro_table                njs_LTX_mp4h_macro_table
#define mp4h_init_module                njs_LTX_mp4h_init_module
#define mp4h_finish_module              njs_LTX_mp4h_finish_module

DECLARE(mp4m_javascript_njs); /* declare javascript as implementing a builtin */

#undef DECLARE

int output_func(void *, unsigned char *, unsigned int);

/* The table of builtins defined by this module - just one */

builtin mp4h_macro_table[] =
{
  /* name             container   expand    function
                                attributes                      */

  { "javascript",       TRUE,     TRUE,   mp4m_javascript_njs },
  { 0,                  FALSE,    FALSE,  0 },
};

static JSInterpPtr interp = NULL;

static JSInterpPtr
create_interp ()
{
    JSInterpOptions options;
    JSInterpPtr interp;

    js_init_default_options (&options);

    /* Use our own stdout handler (stderr and stdin are left
       untouched. */
    options.s_stdout = output_func;

    /* XXX should stdin be /dev/null? */

    interp = js_create_interp (&options);
    if (interp == NULL) {
        MP4HERROR ((warning_status, 0,
            _("Warning:%s:%d: could'nt create interpreter\n%s"),
                CURRENT_FILE_LINE));
        return (NULL);
    }

    /* And finally, initialise the mp4h module. */
    mp4h_MP4H(interp->vm);

    return interp;
}

/* obs holds a pointer to the current obstack for output. It is
   initialised from the argument to the javascript tag. */
static struct obstack *javascript_obs;

/* An output function that is put in place of the ordinary stdout
   handler */
int
output_func(void *context, unsigned char *buffer, unsigned int amount)
{
    obstack_grow(javascript_obs, buffer, amount);
    return 0;
}

void
mp4h_init_module(struct obstack *obs)
{
    if ( interp == NULL ) 
        interp = create_interp();
}

void
mp4h_finish_module(void)
{
}

/* The functions for builtins can be static */

/* Execute the body of this tag in the JavaScript interpreter. */
static void
mp4m_javascript_njs (MP4H_BUILTIN_ARGS)
{
    char *script;

    javascript_obs = obs;

    /* make quotes real quotes before handing the script to the
       interpreter... */
    script = xstrdup(ARGBODY);
    remove_special_chars (script, TRUE);

    if ( !js_eval (interp, script) ) {
        MP4HERROR ((warning_status, 0,
            _("Warning:%s:%d: execution of JavaScript failed:\n%s"),
                CURRENT_FILE_LINE, js_error_message (interp)));
    }
    xfree ((voidstar) script);
}


