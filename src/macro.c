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

/* This file contains the functions, that performs the basic argument
   parsing and macro expansion.  */

#include "mp4h.h"

static void expand_macro __P ((symbol *, read_type));
static void expand_unknown_macro __P ((char *, read_type));
static void expand_token __P ((struct obstack *, read_type, token_type, token_data *));
static void collect_body __P ((symbol *, boolean, struct obstack *));

/* Current recursion level in expand_macro ().  */
int expansion_level = 0;

/* The number of the current call of expand_macro ().  */
static int macro_call_id = 0;

/*----------------------------------------------------------------------.
| This function read all input, and expands each token, one at a time.  |
`----------------------------------------------------------------------*/

void
expand_input (void)
{
  token_type t;
  token_data td;

  while ((t = next_token (&td, READ_NORMAL)) != TOKEN_EOF)
    expand_token ((struct obstack *) NULL, READ_NORMAL, t, &td);
}


/*------------------------------------------------------------------------.
| Expand one token, according to its type.  Potential macro names         |
| (TOKEN_WORD) are looked up in the symbol table, to see if they have a   |
| macro definition.  If they have, they are expanded as macros, otherwise |
| the text are just copied to the output.                                 |
`------------------------------------------------------------------------*/

static void
expand_token (struct obstack *obs, read_type expansion, token_type t,
        token_data *td)
{
  symbol *sym;
  char *text = TOKEN_DATA_TEXT (td);

  switch (t)
    {                           /* TOKSW */
    case TOKEN_EOF:
    case TOKEN_MACDEF:
      break;

    case TOKEN_SIMPLE:
    case TOKEN_STRING:
    case TOKEN_SPACE:
      shipout_text (obs, text, strlen (text));
      break;

    case TOKEN_BGROUP:
    case TOKEN_EGROUP:
      break;

    case TOKEN_WORD:
      if (IS_ESCAPE(*text))
        text++;

      /* macro names must begin with a letter, an underscore or a %.
         If another character is found, this string is not a
         macro, it could be ePerl delimiters.  */

      if (*text == '/' ||
          (!isalpha((int) *text) && *text != '%' && *text != '_'))
        {
          expand_unknown_macro (text, expansion);
          break;
        }
      sym = lookup_symbol (text, SYMBOL_LOOKUP);
      if (sym == NULL)
        {
          expand_unknown_macro (text, expansion);
        }
      else if (SYMBOL_TYPE (sym) == TOKEN_VOID)
        {
          shipout_text (obs, TOKEN_DATA_TEXT (td),
                                strlen (TOKEN_DATA_TEXT (td)));
        }
      else
        {
          expand_macro (sym, expansion);
        }
      break;

    default:
      MP4HERROR ((warning_status, 0,
               _("INTERNAL ERROR: Bad token type in expand_token ()")));
      abort ();
    }
}



/*-------------------------------------------------------------------------.
| This function parses one argument to a macro call.  It expects the first |
| left parenthesis, or the separating comma to have been read by the       |
| caller.  It skips leading whitespace, and reads and expands tokens,      |
| until it finds a comma or an right parenthesis at the same level of      |
| parentheses.  It returns a flag indicating whether the argument read are |
| the last for the active macro call.  The argument are build on the       |
| obstack OBS, indirectly through expand_token ().                         |
`-------------------------------------------------------------------------*/

static boolean
expand_argument (struct obstack *obs, read_type expansion, token_data *argp)
{
  char *text;
  token_type t;
  token_data td;
  int group_level = 0;
  boolean in_string = FALSE;

  TOKEN_DATA_TYPE (argp) = TOKEN_VOID;

  /* Skip leading white space.  */
  do
    {
      t = next_token (&td, expansion);
    }
  while (t == TOKEN_SPACE);

  while (1)
    {

      switch (t)
        {                       /* TOKSW */
        case TOKEN_SPACE:
        case TOKEN_SIMPLE:
          text = TOKEN_DATA_TEXT (&td);
          if (*text == '"' && group_level == 0)
              in_string = !in_string;
          if ((IS_SPACE(*text) || IS_CLOSE(*text))
               && !in_string && (group_level == 0))
            {

              /* The argument MUST be finished, whether we want it or not.  */
              obstack_1grow (obs, '\0');
              text = obstack_finish (obs);

              if (TOKEN_DATA_TYPE (argp) == TOKEN_VOID)
                {
                  TOKEN_DATA_TYPE (argp) = TOKEN_TEXT;
                  TOKEN_DATA_TEXT (argp) = text;
                }
              return (boolean) (IS_SPACE(*TOKEN_DATA_TEXT (&td)));
            }
          expand_token (obs, expansion, t, &td);
          break;

        case TOKEN_EOF:
          MP4HERROR ((EXIT_FAILURE, 0,
                    _("ERROR: EOF in argument list")));
          break;

        case TOKEN_BGROUP:
          group_level++;
          break;

        case TOKEN_EGROUP:
          group_level--;
          break;

        case TOKEN_QUOTE:
          if (group_level == 0)
              in_string = !in_string;
          break;

        case TOKEN_STRING:
          obstack_grow (obs, TOKEN_DATA_TEXT (&td),
                  strlen (TOKEN_DATA_TEXT (&td)));
          break;

        case TOKEN_WORD:
          if (expansion & READ_VERB_ATTR)
              expand_token (obs, READ_VERBATIM, t, &td);
          else
              expand_token (obs, expansion, t, &td);
          break;

        case TOKEN_MACDEF:
          if (obstack_object_size (obs) == 0)
            {
              TOKEN_DATA_TYPE (argp) = TOKEN_FUNC;
              TOKEN_DATA_FUNC (argp) = TOKEN_DATA_FUNC (&td);
              TOKEN_DATA_FUNC_TRACED (argp) = TOKEN_DATA_FUNC_TRACED (&td);
            }
          break;

        default:
          MP4HERROR ((warning_status, 0, _("\
INTERNAL ERROR: Bad token type in expand_argument ()")));
          abort ();
        }

      t = next_token (&td, expansion);
    }
}

/*-------------------------------------------------------------------------.
| Collect all the arguments to a call of the macro SYM.  The arguments are |
| stored on the obstack ARGUMENTS and a table of pointers to the arguments |
| on the obstack ARGPTR.                                                   |
`-------------------------------------------------------------------------*/

static void
collect_arguments (symbol *sym, read_type expansion, struct obstack *argptr,
                   struct obstack *arguments)
{
  int ch;
  token_data td;
  token_data *tdp;
  char *last_addr;
  boolean more_args;

  TOKEN_DATA_TYPE (&td) = TOKEN_TEXT;
  TOKEN_DATA_TEXT (&td) = SYMBOL_NAME (sym);
  tdp = (token_data *) obstack_copy (arguments, (voidstar) &td, sizeof (td));
  obstack_grow (argptr, (voidstar) &tdp, sizeof (tdp));

  ch = peek_input ();
  if (IS_CLOSE (ch))
    next_token (&td, expansion);
  else if (IS_SPACE(ch) || IS_ESCAPE(ch))
    {
      do
        {
          /*  remember last address in use to remove the last
              argument if it is empty.  */
          last_addr = argptr->next_free;
          more_args = expand_argument (arguments, expansion, &td);
          tdp = (token_data *)
            obstack_copy (arguments, (voidstar) &td, sizeof (td));
          obstack_grow (argptr, (voidstar) &tdp, sizeof (tdp));
        }
      while (more_args);
      
      /*  If the last argument is empty, it is removed.  We need it to
          remove wjite spaces before closing brackets.  */
      if (TOKEN_DATA_TYPE (tdp) == TOKEN_TEXT
          && strlen (TOKEN_DATA_TEXT (tdp)) == 0)
        argptr->next_free = last_addr;
    }
}

/*-----------------------------------------------------------------.
| Collect the body of a container tag. No expansion is performed.  |
`-----------------------------------------------------------------*/
static void
collect_body (symbol *sym, boolean whitespace, struct obstack *argptr)
{
  token_type t;
  token_data td;
  token_data *tdp;
  struct obstack body;
  char *text;
  symbol *newsym;

  obstack_init (&body);
  while (1)
    {
      t = next_token (&td, READ_VERBATIM);
      text = TOKEN_DATA_TEXT (&td);
      switch (t)
        {                                /* TOKSW */
        case TOKEN_EOF:
        case TOKEN_MACDEF:
          MP4HERROR ((EXIT_FAILURE, 0,
                    _("ERROR: EOF when reading body of the `%s' tag"),
                        SYMBOL_NAME(sym)));
          break;

        case TOKEN_BGROUP:
        case TOKEN_EGROUP:
          break;

        case TOKEN_SIMPLE:
        case TOKEN_STRING:
          shipout_text (&body, text, strlen (text));
          break;

        case TOKEN_SPACE:
          if (!whitespace)
              shipout_text (&body, text, strlen (text));
          break;

        case TOKEN_WORD:
          if (IS_ESCAPE(*text))
            text++;

          if (*text == '/')
            {
              text++;
              if (strcasecmp (text, SYMBOL_NAME(sym)) == 0)
                {
                  /*  gobble closing bracket */
                  do
                    {
                      t = next_token (&td, READ_VERBATIM);
                    }
                  while (! IS_CLOSE(*TOKEN_DATA_TEXT (&td)));
                  obstack_1grow (&body, '\0');

                  /* Add body to argptr */
                  TOKEN_DATA_TYPE (&td) = TOKEN_TEXT;
                  TOKEN_DATA_TEXT (&td) = obstack_finish (&body);
                  tdp = (token_data *) obstack_copy (&body,
                      (voidstar) &td, sizeof (td));
                  obstack_grow (argptr, (voidstar) &tdp, sizeof (tdp));

                  return;
                  break;
                }
              else
                {
                  obstack_grow (&body, TOKEN_DATA_TEXT (&td),
                          strlen(TOKEN_DATA_TEXT (&td)) );
                  /*  gobble closing bracket */
                  do
                    {
                      t = next_token (&td, READ_VERBATIM);
                      obstack_grow (&body, TOKEN_DATA_TEXT (&td),
                              strlen(TOKEN_DATA_TEXT (&td)) );
                    }
                  while (! IS_CLOSE(*TOKEN_DATA_TEXT (&td)));
                }
            }
          else
            {
              newsym = lookup_symbol (text, SYMBOL_LOOKUP);
              if (newsym == NULL || SYMBOL_TYPE (newsym) == TOKEN_VOID)
                {
                  expand_unknown_macro (text, READ_VERBATIM);
                }
              else
                {
                  expand_macro (newsym, READ_VERBATIM);
                }
            }
          break;

        default:
          MP4HERROR ((warning_status, 0,
                    _("INTERNAL ERROR: Bad token type in collect_body ()")));
          abort ();
        }
    }
  obstack_free (&body, NULL);
}


/*------------------------------------------------------------------------.
| The actual call of a macro is handled by call_macro ().  call_macro ()  |
| is passed a symbol SYM, whose type is used to call either a builtin     |
| function, or the user macro expansion function expand_user_macro ()     |
| (lives in builtin.c).  There are ARGC arguments to the call, stored in  |
| the ARGV table.  The expansion is left on the obstack EXPANSION.  Macro |
| tracing is also handled here.                                           |
`------------------------------------------------------------------------*/

void
call_macro (symbol *sym, struct obstack *obs, int argc, token_data **argv,
                 read_type expansion)
{
  switch (SYMBOL_TYPE (sym))
    {
    case TOKEN_FUNC:
      (*SYMBOL_FUNC (sym)) (obs, argc, argv, expansion);
      break;

    case TOKEN_TEXT:
      expand_user_macro (obs, sym, argc, argv, expansion);
      break;

    default:
      MP4HERROR ((warning_status, 0,
                _("INTERNAL ERROR: Bad symbol type in call_macro ()")));
      abort ();
    }
}

/*-------------------------------------------------------------------------.
| The macro expansion is handled by expand_macro ().  It parses the        |
| arguments, using collect_arguments (), and builds a table of pointers to |
| the arguments.  The arguments themselves are stored on a local obstack.  |
| Expand_macro () uses call_macro () to do the call of the macro.          |
|                                                                          |
| Expand_macro () is potentially recursive, since it calls expand_argument |
| (), which might call expand_token (), which might call expand_macro ().  |
`-------------------------------------------------------------------------*/

static void
expand_macro (symbol *sym, read_type expansion)
{
  struct obstack arguments;
  struct obstack argptr, save_argptr;
  token_data **argv;
  int argc, i;
  struct obstack *obs_expansion;
  const char *expanded;
  boolean traced;
  int my_call_id;
  const char *space_delete;
  read_type attr_expansion;

  expansion_level++;
  if (expansion_level > nesting_limit)
    MP4HERROR ((EXIT_FAILURE, 0, _("\
ERROR: Recursion limit of %d exceeded, use -L<N> to change it"),
              nesting_limit));

  macro_call_id++;
  my_call_id = macro_call_id;

  traced = (boolean) ((debug_level & DEBUG_TRACE_ALL) || SYMBOL_TRACED (sym));

  obstack_init (&argptr);
  obstack_init (&arguments);

  if (traced && (debug_level & DEBUG_TRACE_CALL))
    trace_prepre (SYMBOL_NAME (sym), my_call_id);

  if (expansion & READ_VERBATIM)
    attr_expansion = expansion;
  else if (SYMBOL_EXPAND_ARGS (sym))
      attr_expansion = expansion | READ_ATTRIBUTE;
  else
      attr_expansion = expansion | READ_VERB_ATTR;

  collect_arguments (sym, attr_expansion, &argptr, &arguments);

  save_argptr = argptr;
  argc = obstack_object_size (&argptr) / sizeof (token_data *);
  argv = (token_data **) obstack_finish (&argptr);

  if (SYMBOL_CONTAINER (sym))
    {
      space_delete = predefined_attribute ("whitespace", &argc, argv,
                                           TRUE, FALSE);
      if (space_delete && strcmp (space_delete, "delete") == 0)
          collect_body (sym, TRUE, &save_argptr);
      else
          collect_body (sym, FALSE, &save_argptr);
      
      argv = (token_data **) obstack_finish (&save_argptr);
    }

  if (traced)
    trace_pre (SYMBOL_NAME (sym), my_call_id, argc, argv);

  obs_expansion = push_string_init ();
  if (!(expansion & READ_VERBATIM) && !(expansion & READ_VERB_ATTR))
    {
      if (expansion_level > 1)
        obstack_1grow (obs_expansion, CHAR_BGROUP);
      call_macro (sym, obs_expansion, argc, argv, expansion);
      if (expansion_level > 1)
        obstack_1grow (obs_expansion, CHAR_EGROUP);
    }
  else
    {
      obstack_1grow (obs_expansion, '<');
      obstack_grow (obs_expansion, SYMBOL_NAME (sym),
          strlen (SYMBOL_NAME (sym)));

      for (i = 1; i < argc; i++)
        {
          obstack_1grow (obs_expansion, ' ');
          shipout_string (obs_expansion, TOKEN_DATA_TEXT (argv[i]), 0);
        }
      obstack_1grow (obs_expansion, '>');
      if (SYMBOL_CONTAINER (sym))
        {
          obstack_grow(obs_expansion, TOKEN_DATA_TEXT (argv[argc]),
              strlen(TOKEN_DATA_TEXT (argv[argc])));
          obstack_grow (obs_expansion, "</", 2);
          obstack_grow (obs_expansion, SYMBOL_NAME (sym),
              strlen (SYMBOL_NAME (sym)));
          obstack_1grow (obs_expansion, '>');
        }
    }
  expanded = push_string_finish (expansion);

  if (traced)
    trace_post (SYMBOL_NAME (sym), my_call_id, argc, argv, expanded);

  --expansion_level;

  obstack_free (&arguments, NULL);
  obstack_free (&argptr, NULL);
}

static void
expand_unknown_macro (char *name, read_type expansion)
{
  symbol unknown;
  struct obstack arguments;
  struct obstack argptr;
  token_data **argv;
  int argc, i;
  struct obstack *obs_expansion;
  char *symbol_name;
  const char *expanded;
  boolean traced;
  int my_call_id;

  /* The ``name'' pointer points to a region which is modified
     so we have to copy it now.  */
  symbol_name = xstrdup (name);
  expansion_level++;
  if (expansion_level > nesting_limit)
    MP4HERROR ((EXIT_FAILURE, 0, _("\
ERROR: Recursion limit of %d exceeded, use -L<N> to change it"),
              nesting_limit));

  macro_call_id++;
  my_call_id = macro_call_id;

  /*  define a temporary symbol */
  SYMBOL_NAME (&unknown)        = symbol_name;
  SYMBOL_TRACED (&unknown)      = FALSE;
  SYMBOL_CONTAINER (&unknown)   = FALSE;
  SYMBOL_EXPAND_ARGS (&unknown) = FALSE;

  traced = (boolean) ((debug_level & DEBUG_TRACE_ALL) || SYMBOL_TRACED (&unknown));

  obstack_init (&argptr);
  obstack_init (&arguments);

  if (traced && (debug_level & DEBUG_TRACE_CALL))
    trace_prepre (SYMBOL_NAME (&unknown), my_call_id);


  if (expansion & READ_VERBATIM || expansion & READ_VERB_ATTR)
    expansion = READ_VERBATIM;
  else
    expansion = READ_NORMAL;

  collect_arguments (&unknown, expansion, &argptr, &arguments);

  argc = obstack_object_size (&argptr) / sizeof (token_data *);
  argv = (token_data **) obstack_finish (&argptr);

  if (traced)
    trace_pre (SYMBOL_NAME (&unknown), my_call_id, argc, argv);

  obs_expansion = push_string_init ();
  obstack_1grow (obs_expansion, '<');
  obstack_grow (obs_expansion, symbol_name, strlen (symbol_name));

  for (i = 1; i < argc; i++)
    {
      obstack_1grow (obs_expansion, ' ');
      shipout_string (obs_expansion, TOKEN_DATA_TEXT (argv[i]), 0);
    }
  obstack_1grow (obs_expansion, '>');
  expanded = push_string_finish (READ_VERBATIM);

  if (traced)
    trace_post (SYMBOL_NAME (&unknown), my_call_id, argc, argv, expanded);

  --expansion_level;

  obstack_free (&arguments, NULL);
  obstack_free (&argptr, NULL);
  xfree (symbol_name);
}
