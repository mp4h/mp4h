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
   Copyright (C) 1989, 90, 91, 92, 93, 94 Free Software Foundation, Inc.
  
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

#include <getopt.h>

#define _AS_HEADER
#include "version.c"

/* Enable sync output for /lib/cpp (-s).  */
int sync_output = 0;

/* Debug (-d[flags]).  */
int debug_level = 0;

/* Hash table size (should be a prime) (-Hsize).  */
int hash_table_size = HASHMAX;

/* Disable GNU extensions (-G).  */
int no_gnu_extensions = 0;

/* Max length of arguments in trace output (-lsize).  */
int max_debug_argument_length = 0;

/* Suppress warnings about missing arguments.  */
int suppress_warnings = 0;

/* If not zero, then value of exit status for warning diagnostics.  */
int warning_status = 0;

/* Artificial limit for expansion_level in macro.c.  */
int nesting_limit = 250;

/* Name of frozen file to digest after initialization.  */
const char *frozen_file_to_read = NULL;

/* Name of frozen file to produce near completion.  */
const char *frozen_file_to_write = NULL;

/* The name this program was run with. */
const char *program_name;

/* If nonzero, display usage information and exit.  */
static int show_help = 0;

/* If nonzero, print the version on standard output and exit.  */
static int show_version = 0;

/* If nonzero, import the environment as macros.  */
static int import_environment = 0;

/* If nonzero, comments are discarded in the token parser.  */
int discard_comments = 2;

struct macro_definition
{
  struct macro_definition *next;
  int code;                     /* D, U or t */
  const char *macro;
};
typedef struct macro_definition macro_definition;

/* Error handling functions.  */

/*----------------------------------------------------------------------.
| Print program name, source file and line reference on standard        |
| error, as a prefix for error messages.  Flush standard output first.  |
`----------------------------------------------------------------------*/

#include <error.h>

void
print_program_name (void)
{
  fflush (stdout);
  fprintf (stderr, "%s: ", program_name);
  if (current_line != 0)
    fprintf (stderr, "%s: %d: ", current_file, current_line);
}


#ifdef USE_STACKOVF

/*---------------------------------------.
| Tell user stack overflowed and abort.  |
`---------------------------------------*/

static void
stackovf_handler (void)
{
  MP4HERROR ((EXIT_FAILURE, 0,
            _("ERROR: Stack overflow.  (Infinite define recursion?)")));
}

#endif /* USE_STACKOV */

/* Memory allocation.  */

/*------------------------.
| Failsafe free routine.  |
`------------------------*/

void
xfree (voidstar p)
{
  if (p != NULL)
    free (p);
}


/*---------------------------------------------.
| Print a usage message and exit with STATUS.  |
`---------------------------------------------*/

static void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
      fputs (_("\
Mandatory or optional arguments to long options are mandatory or optional\n\
for short options too.\n\
\n\
Operation modes:\n\
      --help                   display this help and exit\n\
      --version                output version information and exit\n\
  -E, --fatal-warnings         stop execution after first warning\n\
  -Q, --quiet, --silent        suppress some warnings for builtins\n"),
             stdout);
      fputs (_("\
\n\
Preprocessor features:\n\
  -I, --include=DIRECTORY      search this directory second for includes\n\
  -D, --define=NAME[=VALUE]    enter NAME has having VALUE, or empty\n\
  -U, --undefine=NAME          delete builtin NAME\n"),
             stdout);
      fputs (_("\
\n\
Limits control:\n\
  -H, --hashsize=PRIME         set symbol lookup hash table size\n\
  -L, --nesting-limit=NUMBER   change artificial nesting limit\n"),
             stdout);
      fputs (_("\
\n\
Frozen state files:\n\
  -F, --freeze-state=FILE      produce a frozen state on FILE at end\n\
  -R, --reload-state=FILE      reload a frozen state from FILE at start\n"),
             stdout);
      fputs (_("\
\n\
Debugging:\n\
  -d, --debug=[FLAGS]          set debug level (no FLAGS implies `ae')\n\
  -t, --trace=NAME             trace NAME when it will be defined\n\
  -l, --arglength=NUM          restrict macro tracing size\n\
  -o, --error-output=FILE      redirect debug and trace output\n"),
             stdout);
      fputs (_("\
\n\
FLAGS is any of:\n\
  t   trace for all macro calls, not only traceon'ed\n\
  a   show actual arguments\n\
  e   show expansion\n\
  c   show before collect, after collect and after call\n\
  x   add a unique macro call id, useful with c flag\n\
  f   say current input file name\n\
  l   say current input line number\n\
  p   show results of path searches\n\
  i   show changes in input files\n\
  V   shorthand for all of the above flags\n"),
             stdout);
      fputs (_("\
\n\
If no FILE or if FILE is `-', standard input is read.\n"),
             stdout);

    }
  exit (status);
}

/*--------------------------------------.
| Decode options and launch execution.  |
`--------------------------------------*/

static const struct option long_options[] =
{
  {"arglength", required_argument, NULL, 'l'},
  {"debug", optional_argument, NULL, 'd'},
  {"discard-comments", no_argument, NULL, 'c'},
  {"error-output", required_argument, NULL, 'o'},
  {"fatal-warnings", no_argument, NULL, 'E'},
  {"freeze-state", required_argument, NULL, 'F'},
  {"hashsize", required_argument, NULL, 'H'},
  {"include", required_argument, NULL, 'I'},
  {"nesting-limit", required_argument, NULL, 'L'},
  {"quiet", no_argument, NULL, 'Q'},
  {"reload-state", required_argument, NULL, 'R'},
  {"silent", no_argument, NULL, 'Q'},
  {"synclines", no_argument, NULL, 's'},

  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},

  /* These are somewhat troublesome.  */
  { "define", required_argument, NULL, 'D' },
  { "undefine", required_argument, NULL, 'U' },
  { "trace", required_argument, NULL, 't' },

  { 0, 0, 0, 0 },
};

#define OPTSTRING "B:D:EF:H:I:L:N:QR:S:U:cd::ehl:o:st:V"

int
main (int argc, char *const *argv, char *const *envp)
{
  macro_definition *head;       /* head of deferred argument list */
  macro_definition *tail;
  macro_definition *new;
  int optchar;                  /* option character */

  macro_definition *defines;
  FILE *fp;
  char *filename;

  program_name = argv[0];
  error_print_progname = print_program_name;

  /* setlocale (LC_ALL, ""); */
  setlocale (LC_NUMERIC, "C");

  debug_init ();
  include_init ();

#ifdef USE_STACKOVF
  setup_stackovf_trap (argv, envp, stackovf_handler);
#endif

  /* First, we decode the arguments, to size up tables and stuff.  */

  head = tail = NULL;

  while (optchar = getopt_long (argc, argv, OPTSTRING, long_options, NULL),
         optchar != EOF)
    switch (optchar)
      {
      default:
        usage (EXIT_FAILURE);

      case 0:
        break;

      case 'B':                  /* compatibility junk */
      case 'N':
      case 'S':
      case 'T':
        break;

      case 'D':
      case 'U':
      case 't':

        /* Arguments that cannot be handled until later are accumulated.  */

        new = (macro_definition *) xmalloc (sizeof (macro_definition));
        new->code = optchar;
        new->macro = optarg;
        new->next = NULL;

        if (head == NULL)
          head = new;
        else
          tail->next = new;
        tail = new;

        break;

      case 'E':
        warning_status = EXIT_FAILURE;
        break;

      case 'F':
        frozen_file_to_write = optarg;
        break;

      case 'G':
        no_gnu_extensions = 1;
        break;

      case 'H':
        hash_table_size = atoi (optarg);
        if (hash_table_size <= 0)
          hash_table_size = HASHMAX;
        break;

      case 'I':
        add_include_directory (optarg);
        break;
      case 'L':
        nesting_limit = atoi (optarg);
        break;

        suppress_warnings = 1;
        break;

      case 'R':
        frozen_file_to_read = optarg;
        break;

      case 'c':
        discard_comments = 1;
        break;

      case 'd':
        debug_level = debug_decode (optarg);
        if (debug_level < 0)
          {
            error (0, 0, _("Bad debug flags: `%s'"), optarg);
            debug_level = 0;
          }
        break;

      case 'l':
        max_debug_argument_length = atoi (optarg);
        if (max_debug_argument_length <= 0)
          max_debug_argument_length = 0;
        break;

      case 'o':
        if (!debug_set_output (optarg))
          error (0, errno, optarg);
        break;

      case 's':
        sync_output = 1;
        break;

      case 'V':
        show_version = 1;
        break;

      case 'h':
        show_help = 1;
        break;

      }

  if (show_version)
    {
      printf ("%s %s", program_name, MP4H_VersionStr);
      fputs("\n", stdout);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (EXIT_SUCCESS);

  /* Do the basic initialisations.  */

  input_init ();
  output_init ();
  symtab_init ();
  include_env_init ();

  if (frozen_file_to_read)
    reload_frozen_state (frozen_file_to_read);
  else
    builtin_init ();


  /* Import environment variables as macros.  The definition are
     preprended to the macro definition list, so -U can override
     environment variables. */

  if (import_environment)
    {
      char *const *env;

      for (env = envp; *env != NULL; env++)
        {
          new = (macro_definition *) xmalloc (sizeof (macro_definition));
          new->code = 'D';
          new->macro = *env;
          new->next = head;
          head = new;
        }
    }

  /* Handle deferred command line macro definitions.  Must come after
     initialisation of the symbol table.  */

  defines = head;

  while (defines != NULL)
    {
      macro_definition *next;
      char *macro_value;
      symbol *sym;

      switch (defines->code)
        {
        case 'D':
          macro_value = strchr (defines->macro, '=');
          if (macro_value == NULL)
            macro_value = "";
          else
            *macro_value++ = '\0';
          sym = lookup_variable (defines->macro, SYMBOL_INSERT);
          if (SYMBOL_TYPE (sym) == TOKEN_TEXT)
              xfree (SYMBOL_TEXT (sym));
          SYMBOL_TYPE (sym) = TOKEN_TEXT;
          SYMBOL_TEXT (sym) = xstrdup (macro_value);
          break;

        case 'U':
          lookup_variable (defines->macro, SYMBOL_DELETE);
          break;

        case 't':
          sym = lookup_variable (defines->macro, SYMBOL_INSERT);
          SYMBOL_TRACED (sym) = TRUE;
          break;

        default:
          MP4HERROR ((warning_status, 0,
                    _("INTERNAL ERROR: Bad code in deferred arguments")));
          abort ();
        }

      next = defines->next;
      xfree ((voidstar) defines);
      defines = next;
    }

  /* Handle the various input files.  Each file is pushed on the input,
     and the input read.  Wrapup text is handled separately later.  */

  if (optind == argc)
    {
      push_file (stdin, "stdin");
      expand_input ();
    }
  else
    for (; optind < argc; optind++)
      {
        if (strcmp (argv[optind], "-") == 0)
          push_file (stdin, "stdin");
        else
          {
            fp = path_search (argv[optind], &filename);
            if (fp == NULL)
              {
                error (0, errno, argv[optind]);
                continue;
              }
            else
              {
                push_file (fp, filename);
                xfree (filename);
              }
          }
        expand_input ();
      }
#undef NEXTARG

  if (frozen_file_to_write)
    produce_frozen_state (frozen_file_to_write);
  else
    {
      make_diversion (0);
      undivert_all ();
    }

#ifdef WITH_MODULES
  module_unload_all();
#endif /* WITH_MODULES */

  exit (EXIT_SUCCESS);
}
