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
   Copyright (C) 1989, 90, 91, 92, 93, 94, 98 Free Software Foundation, Inc.
  
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

#include "mp4h.h"
#include "builtin.h"

#ifdef WITH_MODULES

#define MODULES_UNINITIALISED -4444

#include "ltdl.h"

#undef DEBUG_MODULES
#define DEBUG_MODULES

/* 
 * This file implements dynamic modules in GNU m4.  A module is a
 * compiled shared object, that can be linked into GNU m4 at run
 * time. Information about creating modules is in ../modules/README. 
 *
 * The current implementation uses either dlopen(3) (exists on
 * GNU/Linux, OSF, Solaris, SunOS) or shl_load(3) (exists on HPUX).  To
 * enable this experimental feature give configure the `--with-modules'
 * switch.  This implementation is only tested on GNU/Linux, OSF,
 * Solaris, SunOS and HPUX.
 *
 * A m4 module need only define one external symbol, called
 * `m4_macro_table'.  This symbol should point to a table of `struct
 * builtin' exactly as the one in builtin.c.  This table is pushed on a
 * list of builtin tables and each definition therein is added to the
 * symbol table.
 *
 * The code implementing loadable modules is modest.  It is divided
 * between the files builtin.c (user interface and support for multiple
 * builtin tables) and this file (OS dependant routines).
 *
 * To load a module, use `loadmodule(modulename)'.  The function
 * `m4_loadmodule' calls module_load() in this file, which uses
 * module_search() to find the module in the module search path.  This
 * path is initialised from the environment variable MP4HMODPATH, or if
 * not set, initalised to a configuration time default.  Module_search()
 * constructs absolute file names and calls module_try_load().  This
 * function reads the libtool .la file to get the real library name
 * (which can be system dependent) and returns NULL on failure and a
 * non-NULL void* on success.  If succesful module_search() returns the
 * value of this void*, which is a handle for the vm segment mapped.
 * Module_load() checks to see if the module is alreay loaded, and if
 * not, retrives the symbol `m4_macro_table' and returns it's value to
 * m4_loadmodule().  This pointer should be a builtin*, which is
 * installed using install_builtin_table().
 *
 * When a module is loaded, the function "void m4_init_module(struct
 * obstack *obs)" is called, if defined.  Any non NULL return value of
 * this function will be the expansion of "loadmodule".  Before program
 * exit, all modules are unloaded and the function "void
 * m4_finish_module(void)" is called, if defined.
 *
 * There is no way to unload a module unless at program exit.  It is
 * safe to load the same module several times, it has no effect.
 **/

/* This list is used to check for repeated loading of the same modules.  */

typedef struct module_list {
  struct module_list *next;
  char *modname;
  void *handle;
} module_list;

static module_list *modules;

/* 
 * Initialisation.  Currently the module search path in path.c is
 * initialised from MP4HMODPATH.  Only absolute path names are accepted to
 * prevent the path search of the dlopen library from finding wrong
 * files.
 */
static void
module_init (void)
{
  static int errors = MODULES_UNINITIALISED;

  /* Do this only once! */
  if (errors != MODULES_UNINITIALISED)
    return;

  errors = lt_dlinit();

  /* If the user set MP4HMODPATH, then use that as the start of libltdls
   * module search path, else fall back on the default.
   */
  if (errors == 0)
    {
      char *path = getenv("MP4HMODPATH");
      if (path != NULL)
	errors = lt_dladdsearchdir(path);
    }
    
  if (errors == 0)
    errors = lt_dladdsearchdir(PKGLIBDIR);

  if (errors != 0)
    {
      /* Couldn't initialise the module system; diagnose and exit. */
      const char *dlerror = lt_dlerror();
      MP4HERROR ((EXIT_FAILURE, 0,
		_("ERROR: failed to initialise modules: %s"), dlerror));
    }

#ifdef DEBUG_MODULES
  DEBUG_MESSAGE("Module system initialised.");
#endif /* DEBUG_MODULES */
}

/* 
 * Load a dynamic library.
 */

void
library_load (const char *libname, struct obstack *obs)
{
  lt_dlhandle library;
  module_list *list;
  builtin *bp;

  /* Dynamically load the named module. */
  library = lt_dlopenext(libname);

  if (library != NULL)
    {
      for (list = modules; list != NULL; list = list->next)
	if (list->handle == library)
	  {
#ifdef DEBUG_MODULES
	    DEBUG_MESSAGE1("library %s handle already seen", libname);
#endif /* DEBUG_MODULES */
	    lt_dlclose(library); /* close the duplicate copy */
	    return;
	  }
    }
}

void
module_load (const char *modname, struct obstack *obs)
{
  module_init_t *init_func;
  lt_dlhandle module;
  module_list *list;
  builtin *bp;

  module_init();

  /* Dynamically load the named module. */
  module = lt_dlopenext(modname);

  if (module != NULL)
    {
      for (list = modules; list != NULL; list = list->next)
	if (list->handle == module)
	  {
#ifdef DEBUG_MODULES
	    DEBUG_MESSAGE1("module %s handle already seen", modname);
#endif /* DEBUG_MODULES */
	    lt_dlclose(module); /* close the duplicate copy */
	    return;
	  }
    }


  /* Find the initialising table in the loaded module. */
  if (module != NULL)
    {
      bp = (builtin*)lt_dlsym(module, "mp4h_macro_table");
      if (bp == NULL)
	{
#ifdef DEBUG_MODULES
	  DEBUG_MESSAGE1("module %s: no symbol mp4h_macro_table", modname);
#endif /* DEBUG_MODULES */
	  lt_dlclose(module);
	  module = NULL;
	}
    }


  /* Find and run the initialising function in the loaded module
   * (if any).
   */
  if (module != NULL)
    {
      init_func = (module_init_t*)lt_dlsym(module, "mp4h_init_module");
      if (init_func != NULL)
	{
	  (*init_func)(obs);
	}
#ifdef DEBUG_MODULES
      else
	DEBUG_MESSAGE1("module %s: no symbol mp4h_init_module", modname);
#endif /* DEBUG_MODULES */
    }


  /* If the module was correctly loaded and has the necessary
   * symbols, then update our internal tables to remember the
   * new module.
   */
  if (module != NULL)
    {
#ifdef DEBUG_MODULES
      DEBUG_MESSAGE1("module %s: loaded ok", modname);
#endif /* DEBUG_MODULES */

      list = xmalloc (sizeof (struct module_list));
      list->next = modules;
      list->modname = xstrdup(modname);
      list->handle = module;
      modules = list;

      install_builtin_table(bp);
    }
  else
    {
      /* Couldn't load the module; diagnose and exit. */
      const char *dlerror = lt_dlerror();
      if (dlerror == NULL)
	MP4HERROR ((EXIT_FAILURE, 0,
		  _("ERROR: cannot find module: `%s'"), modname));
      else
	MP4HERROR ((EXIT_FAILURE, 0,
		  _("ERROR: cannot find module: `%s': %s"),
		  modname, dlerror));
    }
}

void
module_unload_all(void)
{
  int errors = 0;
  struct module_list *next;
  module_finish_t *finish_func;

  /* Find and run the finishing function for each loaded module. */
  while (modules != NULL)
    {
      finish_func = (module_finish_t*)
	lt_dlsym(modules->handle, "mp4h_finish_module");

      if (finish_func != NULL)
	{
	  (*finish_func)();

#ifdef DEBUG_MODULES
	  DEBUG_MESSAGE1("module %s finish hook called", modules->modname);
#endif /* DEBUG_MODULES */
	}

      errors = lt_dlclose (modules->handle);
      if (errors != 0)
	break;

#ifdef DEBUG_MODULES
      DEBUG_MESSAGE1("module %s unloaded", modules->modname);
#endif /* DEBUG_MODULES */

      next = modules->next;
      xfree ((voidstar) modules);
      modules = next;
    }

  if (errors != 0)
    errors = lt_dlexit();

  if (errors != 0)
    {
      const char *dlerror = lt_dlerror();
      if (modules == NULL)
        {
          if (dlerror == NULL)
	    MP4HERROR ((EXIT_FAILURE, 0,
		      _("ERROR: cannot close modules")));
	  else
	    MP4HERROR ((EXIT_FAILURE, 0,
		      _("ERROR: cannot cannot close modules: %s"),
		      dlerror));
	}
      else
        {
	  if (dlerror == NULL)
	    MP4HERROR ((EXIT_FAILURE, 0,
		      _("ERROR: cannot close module: `%s'"),
		      modules->modname));
	  else
	    MP4HERROR ((EXIT_FAILURE, 0,
		      _("ERROR: cannot cannot close module: `%s': %s"),
		      modules->modname, dlerror));
	}
    }
}

#endif  /* WITH_MODULES */
