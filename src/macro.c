/* mp4h -- A macro processor for HTML documents
   Copyright 2000, Denis Barbier
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
static void expand_unknown_tag __P ((char *, read_type));
static void expand_token __P ((struct obstack *, read_type, token_type, token_data *));

/* Current recursion level in expand_macro ().  */
int expansion_level = 0;

/* The number of the current call of expand_macro ().  */
static int macro_call_id = 0;

/* global variable to abort expansion */
static boolean internal_abort = FALSE;

/*----------------------------------------------------------------------.
| This function read all input, and expands each token, one at a time.  |
`----------------------------------------------------------------------*/

void
expand_input (void)
{
  token_type t;
  token_data td;

  while ((t = next_token (&td, READ_NORMAL, FALSE)) != TOKEN_EOF)
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
    case TOKEN_BGROUP:
    case TOKEN_EGROUP:
      break;

    case TOKEN_QUOTED:
    case TOKEN_SIMPLE:
    case TOKEN_SPACE:
    case TOKEN_STRING:
      if (expansion_level > 0 && t == TOKEN_QUOTED)
        obstack_1grow (obs, CHAR_LQUOTE);
      shipout_text (obs, text, strlen (text));
      if (expansion_level > 0 && t == TOKEN_QUOTED)
        obstack_1grow (obs, CHAR_RQUOTE);
      break;

    case TOKEN_WORD:
      if (IS_ESCAPE(*text))
        text++;

      /* macro names must begin with a letter, an underscore or a %.
         If another character is found, this string is not a
         macro.   */

      if (IS_ALPHA (*text) || (*text == '/' && IS_ALPHA (*(text+1))))
        {
          sym = lookup_symbol (text, SYMBOL_LOOKUP);
          if (sym == NULL || SYMBOL_TYPE (sym) == TOKEN_VOID)
            {
              if (exp_flags & EXP_NO_HTMLTAG)
                shipout_text (obs, TOKEN_DATA_TEXT (td),
                        strlen (TOKEN_DATA_TEXT (td)));
              else
                expand_unknown_tag (text, expansion);
            }
          else
            expand_macro (sym, expansion);
        }
      else
        shipout_text (obs, TOKEN_DATA_TEXT (td), strlen (TOKEN_DATA_TEXT (td)));
      break;

    default:
      MP4HERROR ((warning_status, 0,
        _("INTERNAL ERROR: Bad token type in expand_token ()")));
      abort ();
    }
}



/*-------------------------------------------------------------------------.
| This function parses one argument to a macro call.  It skips leading     |
| whitespace and reads and expands tokens until it finds a space outside   |
| of any group, or a right angle bracket.  It returns a flag indicating    |
| whether the argument read are the last for the active macro call.  The   |
| argument read are the last for the active macro call.  The argument are  |
| on the obstack OBS, indirectly through expand_token ().                  |
`-------------------------------------------------------------------------*/

static boolean
expand_argument (struct obstack *obs, read_type expansion, token_data *argp,
                 char *last_char_ptr)
{
  char *text;
  token_type t;
  token_data td;
  int group_level = 0;
  boolean in_string = FALSE;

  *last_char_ptr = ' ';
  TOKEN_DATA_TYPE (argp) = TOKEN_VOID;

  /* Skip leading white space.  */
  do
    {
      t = next_token (&td, expansion, FALSE);
    }
  while (t == TOKEN_SPACE);

  while (1)
    {

      switch (t)
        {                       /* TOKSW */
        case TOKEN_SPACE:
        case TOKEN_SIMPLE:
          text = TOKEN_DATA_TEXT (&td);
          if ((IS_SPACE(*text) || IS_CLOSE(*text))
               && !in_string && (group_level == 0))
            {

              /* The argument MUST be finished, whether we want it or not.  */
              obstack_1grow (obs, '\0');

              if (TOKEN_DATA_TYPE (argp) == TOKEN_VOID)
                {
                  TOKEN_DATA_TYPE (argp) = TOKEN_TEXT;
                  TOKEN_DATA_TEXT (argp) = obstack_finish (obs);
                }
              return (boolean) (IS_SPACE(*TOKEN_DATA_TEXT (&td)));
            }
          expand_token (obs, expansion, t, &td);
          break;

        case TOKEN_EOF:
          internal_abort = TRUE;
          return FALSE;
          break;

        case TOKEN_BGROUP:
          if (expansion == READ_ATTR_ASIS && group_level == 0)
            obstack_1grow (obs, CHAR_BGROUP);
          group_level++;
          break;

        case TOKEN_EGROUP:
          group_level--;
          if (expansion == READ_ATTR_ASIS && group_level == 0)
            obstack_1grow (obs, CHAR_EGROUP);
          break;

        case TOKEN_QUOTE:
          in_string = !in_string;
          obstack_grow (obs, TOKEN_DATA_TEXT (&td),
                  strlen (TOKEN_DATA_TEXT (&td)));
          break;

        case TOKEN_QUOTED:
        case TOKEN_STRING:
          if (expansion_level > 0 && t == TOKEN_QUOTED)
            obstack_1grow (obs, CHAR_LQUOTE);
          obstack_grow (obs, TOKEN_DATA_TEXT (&td),
                  strlen (TOKEN_DATA_TEXT (&td)));
          if (expansion_level > 0 && t == TOKEN_QUOTED)
            obstack_1grow (obs, CHAR_RQUOTE);
          break;

        case TOKEN_WORD:
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
          MP4HERROR ((warning_status, 0,
            _("INTERNAL ERROR: Bad token type in expand_argument ()")));
          abort ();
        }

      *last_char_ptr = LAST_CHAR (TOKEN_DATA_TEXT (&td));
      t = next_token (&td, expansion, in_string);
    }
}

/*-------------------------------------------------------------------------.
| Collect all the arguments to a call of the macro SYM.  The arguments are |
| stored on the obstack ARGUMENTS and a table of pointers to the arguments |
| on the obstack ARGPTR.                                                   |
| If there is a slash character before closing bracket, this function      |
| returns TRUE, otherwise FALSE.                                           |
`-------------------------------------------------------------------------*/

static boolean
collect_arguments (char *symbol_name, read_type expansion,
                   struct obstack *argptr, struct obstack *arguments)
{
  int ch;
  token_data td;
  token_data *tdp;
  char *last_addr;
  boolean more_args;
  char last_char = ' ';

  TOKEN_DATA_TYPE (&td) = TOKEN_TEXT;
  TOKEN_DATA_TEXT (&td) = symbol_name;
  tdp = (token_data *) obstack_copy (arguments, (voidstar) &td, sizeof (td));
  obstack_grow (argptr, (voidstar) &tdp, sizeof (tdp));

  ch = peek_input ();
  if (IS_CLOSE (ch))
    (void) next_token (&td, READ_BODY, FALSE);
  else if (IS_SPACE(ch) || IS_ESCAPE(ch) || ch == '/')
    {
      do
        {
          /*  remember last address in use to remove the last
              argument if it is empty.  */
          last_addr = argptr->next_free;
          more_args = expand_argument (arguments, expansion, &td, &last_char);
          if (internal_abort)
            {
              MP4HERROR ((EXIT_FAILURE, 0,
                _("ERROR:%s:%d: EOF when reading argument of the `%s' tag"),
                     CURRENT_FILE_LINE, symbol_name));
            }
          tdp = (token_data *)
            obstack_copy (arguments, (voidstar) &td, sizeof (td));
          obstack_grow (argptr, (voidstar) &tdp, sizeof (tdp));
        }
      while (more_args);
      
      /*  If the last argument is empty, it is removed.  We need it to
          remove white spaces before closing brackets.  */
      if (TOKEN_DATA_TYPE (tdp) == TOKEN_TEXT
          && strlen (TOKEN_DATA_TEXT (tdp)) == 0)
        argptr->next_free = last_addr;
    }
  else
    {
      MP4HERROR ((warning_status, 0,
        _("INTERNAL ERROR: Bad tag expression in `%s'"),
             CURRENT_FILE_LINE, symbol_name));
    }
  return (last_char == '/');
}

/*-----------------------------------------------------------------.
| Collect the body of a container tag. No expansion is performed,  |
| but when a macro is found its arguments are collected.  This is  |
| necessary to deal with nested expressions.                       |
`-----------------------------------------------------------------*/
static void
collect_body (char *symbol_name, read_type expansion,
              struct obstack *argptr, struct obstack *bodyptr)
{
  token_type t;
  token_data td;
  token_data *tdp;
  char *text;
  symbol *newsym;

  while (1)
    {
      t = next_token (&td, expansion, FALSE);
      text = TOKEN_DATA_TEXT (&td);
      switch (t)
        {                                /* TOKSW */
        case TOKEN_EOF:
        case TOKEN_MACDEF:
          MP4HERROR ((EXIT_FAILURE, 0,
            _("ERROR:%s:%d: EOF when reading body of the `%s' tag"),
                 CURRENT_FILE_LINE, symbol_name));
          break;

        case TOKEN_BGROUP:
        case TOKEN_EGROUP:
          break;

        case TOKEN_QUOTE:
          obstack_1grow (bodyptr, CHAR_QUOTE);
          break;

        case TOKEN_QUOTED:
        case TOKEN_SIMPLE:
        case TOKEN_SPACE:
        case TOKEN_STRING:
          if (expansion_level > 0 && t == TOKEN_QUOTED)
            obstack_1grow (bodyptr, CHAR_LQUOTE);
          shipout_text (bodyptr, text, strlen (text));
          if (expansion_level > 0 && t == TOKEN_QUOTED)
            obstack_1grow (bodyptr, CHAR_RQUOTE);
          break;

        case TOKEN_WORD:
          if (IS_ESCAPE(*text))
            text++;

          if (*text == '/')
            {
              text++;
              if (strcasecmp (text, symbol_name) == 0)
                {
                  /*  gobble closing bracket */
                  do
                    {
                      t = next_token (&td, expansion, FALSE);
                    }
                  while (! IS_CLOSE(*TOKEN_DATA_TEXT (&td)))
                    ;

                  obstack_1grow (bodyptr, '\0');

                  /* Add body to argptr */
                  TOKEN_DATA_TYPE (&td) = TOKEN_TEXT;
                  TOKEN_DATA_TEXT (&td) = obstack_finish (bodyptr);
                  tdp = (token_data *) obstack_copy (bodyptr,
                      (voidstar) &td, sizeof (td));
                  obstack_grow (argptr, (voidstar) &tdp, sizeof (tdp));

                  return;
                }
              else
                {
                  newsym = lookup_symbol (text, SYMBOL_LOOKUP);
                  if (newsym)
                    {
                      if (!(exp_flags & EXP_NOWARN_NEST))
                        MP4HERROR ((warning_status, 0,
                          _("Warning:%s:%d: `</%s>' found when `</%s>' expected"),
                               CURRENT_FILE_LINE, text, symbol_name));
                      if (exp_flags & EXP_UNM_BREAK)
                        {
                          /*  Premature end of body parsing.  */
                          obstack_1grow (bodyptr, '\0');
                          TOKEN_DATA_TYPE (&td) = TOKEN_TEXT;
                          TOKEN_DATA_TEXT (&td) = obstack_finish (bodyptr);
                          tdp = (token_data *) obstack_copy (bodyptr,
                              (voidstar) &td, sizeof (td));
                          obstack_grow (argptr, (voidstar) &tdp, sizeof (tdp));

                          /*  Push read input back on the stack  */
                          unget_string (text-2);

                          return;
                        }
                    }
                  obstack_grow (bodyptr, TOKEN_DATA_TEXT (&td),
                          strlen(TOKEN_DATA_TEXT (&td)) );
                  /*  gobble closing bracket */
                  do
                    {
                      t = next_token (&td, expansion, FALSE);
                      obstack_grow (bodyptr, TOKEN_DATA_TEXT (&td),
                              strlen(TOKEN_DATA_TEXT (&td)) );
                    }
                  while (! IS_CLOSE(*TOKEN_DATA_TEXT (&td)))
                    ;
                }
            }
          else
            {
              newsym = lookup_symbol (text, SYMBOL_LOOKUP);
              if (newsym == NULL || SYMBOL_TYPE (newsym) == TOKEN_VOID)
                {
                  if (exp_flags & EXP_NO_HTMLTAG)
                    shipout_text (bodyptr, TOKEN_DATA_TEXT (&td),
                            strlen (TOKEN_DATA_TEXT (&td)));
                  else
                    expand_unknown_tag (text, expansion);
                }
              else
                expand_macro (newsym, expansion);
            }
          break;

        default:
          MP4HERROR ((warning_status, 0,
            _("INTERNAL ERROR:%d: Bad token type in collect_body ()"), t));
          abort ();
        }
    }
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
  if (SYMBOL_HOOK_BEGIN (sym))
    obstack_grow (obs, SYMBOL_HOOK_BEGIN (sym),
                        strlen (SYMBOL_HOOK_BEGIN (sym)));
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
  if (SYMBOL_HOOK_END (sym))
    obstack_grow (obs, SYMBOL_HOOK_END (sym),
                        strlen (SYMBOL_HOOK_END (sym)));
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
  struct obstack arguments, argptr, body;
  token_data **argv;
  int argc, i;
  struct obstack *obs_expansion;
  const char *expanded;
  char *cp;
  boolean traced, slash;
  int my_call_id;
  read_type attr_expansion;

  expansion_level++;
  array_current_line[expansion_level] = current_line;
  if (expansion_level > nesting_limit)
    MP4HERROR ((EXIT_FAILURE, 0,
      _("ERROR: Recursion limit of %d exceeded, use -L<N> to change it"),
           nesting_limit));

  macro_call_id++;
  my_call_id = macro_call_id;

  traced = (boolean) ((debug_level & DEBUG_TRACE_ALL) || SYMBOL_TRACED (sym));

  obstack_init (&arguments);
  obstack_init (&argptr);
  obstack_init (&body);

  if (traced && (debug_level & DEBUG_TRACE_CALL))
    trace_prepre (SYMBOL_NAME (sym), my_call_id);

  if (expansion == READ_ATTR_ASIS || expansion == READ_ATTR_VERB
      || expansion == READ_BODY)
    attr_expansion = READ_ATTR_ASIS;
  else if (! SYMBOL_EXPAND_ARGS (sym))
    attr_expansion = READ_ATTR_VERB;
  else
    attr_expansion = READ_ATTRIBUTE;

  slash = collect_arguments (SYMBOL_NAME (sym), attr_expansion, &argptr,
                             &arguments);
  argc = obstack_object_size (&argptr) / sizeof (token_data *);

  if (SYMBOL_CONTAINER (sym))
    {
      collect_body (SYMBOL_NAME (sym), READ_BODY, &argptr, &body);
      argv = (token_data **) obstack_finish (&argptr);
    }
  else
    {
      argv = (token_data **) obstack_finish (&argptr);
      if (slash)
        {
          cp = TOKEN_DATA_TEXT (argv[argc-1]);
          if (*cp == '/' && *(cp+1) == '\0')
            argc--;
          else if (LAST_CHAR (cp) == '/')
            LAST_CHAR (cp) = '\0';
        }
      else if (!(exp_flags & EXP_NOWARN_SLASH))
        MP4HERROR ((warning_status, 0,
          _("Warning:%s:%d: `<%s>' requires a trailing slash"),
               CURRENT_FILE_LINE, SYMBOL_NAME (sym)));
    }

  if (traced)
    trace_pre (SYMBOL_NAME (sym), my_call_id, argc, argv);

  obs_expansion = push_string_init ();
  if (expansion == READ_NORMAL || expansion == READ_ATTRIBUTE
          || expansion == READ_ATTR_QUOT)
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
      shipout_string (obs_expansion, SYMBOL_NAME (sym), 0);

      for (i = 1; i < argc; i++)
        {
          obstack_1grow (obs_expansion, ' ');
          if (expansion == READ_ATTR_ASIS)
            shipout_string (obs_expansion, TOKEN_DATA_TEXT (argv[i]), 0);
          else
            {
              if (strlen (TOKEN_DATA_TEXT (argv[i])) == 0)
                {
                  obstack_1grow (obs_expansion, CHAR_BGROUP);
                  obstack_1grow (obs_expansion, CHAR_EGROUP);
                }
              else if (strchr (TOKEN_DATA_TEXT (argv[i]), ' '))
                {
                  obstack_1grow (obs_expansion, CHAR_BGROUP);
                  shipout_string (obs_expansion, TOKEN_DATA_TEXT (argv[i]), 0);
                  obstack_1grow (obs_expansion, CHAR_EGROUP);
                }
              else
                shipout_string (obs_expansion, TOKEN_DATA_TEXT (argv[i]), 0);
            }
        }
      if (!SYMBOL_CONTAINER (sym))
        {
          if (argc > 1)
            obstack_1grow (obs_expansion, ' ');
          obstack_1grow (obs_expansion, '/');
        }

      obstack_1grow (obs_expansion, '>');
      if (SYMBOL_CONTAINER (sym))
        {
          shipout_string (obs_expansion, TOKEN_DATA_TEXT (argv[argc]), 0);
          obstack_grow (obs_expansion, "</", 2);
          shipout_string (obs_expansion, SYMBOL_NAME (sym), 0);
          obstack_1grow (obs_expansion, '>');
        }
    }
  expanded = push_string_finish (expansion);

  if (traced)
    trace_post (SYMBOL_NAME (sym), my_call_id, argc, argv, expanded);

  --expansion_level;

  obstack_free (&arguments, NULL);
  obstack_free (&argptr, NULL);
  obstack_free (&body, NULL);

  if (expansion_level == 0)
    clear_tag_attr ();
}

/*-------------------------------------------------------------------------.
| This macro reads attributes without expanding macro.  It is useful to    |
| print unknown tags.                                                      |
`-------------------------------------------------------------------------*/

static void
expand_unknown_tag (char *name, read_type expansion)
{
  struct obstack arguments, argptr, body;
  token_data **argv;
  int argc, i;
  struct obstack *obs_expansion;
  const char *expanded;
  char *symbol_name, *cp;
  read_type attr_expansion;
  boolean slash;

  expansion_level++;

  symbol_name = xstrdup (name);

  obstack_init (&arguments);
  obstack_init (&argptr);
  obstack_init (&body);

  if (expansion == READ_NORMAL || expansion == READ_ATTRIBUTE)
    attr_expansion = READ_ATTR_QUOT;
  else
    attr_expansion = READ_ATTR_ASIS;

  slash = collect_arguments (symbol_name, attr_expansion, &argptr, &arguments);
  argc  = obstack_object_size (&argptr) / sizeof (token_data *);

  /*
     This tag is a simple tag if either
         - tag name begins with a slash (i.e. this is an end tag)
         - tag name last character is a star
         - attributes are ended by a trailing slash
  */
  if (*(symbol_name) == '/')
    slash = TRUE;

  if (!slash && !(exp_flags & EXP_STAR_COMPLEX))
    slash = (LAST_CHAR (symbol_name) == '*');

  if (!slash && !(exp_flags & EXP_DFT_SIMPLE))
    collect_body (symbol_name, expansion, &argptr, &body);

  argv = (token_data **) obstack_finish (&argptr);

  /*  When this tag is no more processed, remove the trailing slash.  */
  if (!(exp_flags & EXP_LEAVE_TRAILING_SLASH) &&
      expansion == READ_NORMAL && slash)
    {
      cp = TOKEN_DATA_TEXT (argv[argc-1]);
      if (*cp == '/' && *(cp+1) == '\0')
        argc--;
      else if (LAST_CHAR (cp) == '/')
        LAST_CHAR (cp) = '\0';
    }

  if (*(symbol_name) == '/' || expansion == READ_ATTR_ASIS
     || expansion == READ_ATTR_VERB || expansion == READ_BODY)
    expansion = READ_BODY;

  obs_expansion = push_string_init ();

  if (expansion != READ_BODY)
    obstack_1grow (obs_expansion, CHAR_LQUOTE);
  obstack_1grow (obs_expansion, '<');
  shipout_string (obs_expansion, symbol_name, 0);

  for (i = 1; i < argc; i++)
    {
      obstack_1grow (obs_expansion, ' ');
      shipout_string (obs_expansion, TOKEN_DATA_TEXT (argv[i]), 0);
    }
  obstack_1grow (obs_expansion, '>');
  if (expansion != READ_BODY)
    obstack_1grow (obs_expansion, CHAR_RQUOTE);
  if (!slash && !(exp_flags & EXP_DFT_SIMPLE))
    {
      shipout_string (obs_expansion, TOKEN_DATA_TEXT (argv[argc]), 0);
      if (expansion != READ_BODY)
        obstack_1grow (obs_expansion, CHAR_LQUOTE);
      obstack_grow (obs_expansion, "</", 2);
      shipout_string (obs_expansion, symbol_name, 0);
      obstack_1grow (obs_expansion, '>');
      if (expansion != READ_BODY)
        obstack_1grow (obs_expansion, CHAR_RQUOTE);
    }

  expanded = push_string_finish (expansion);

  --expansion_level;

  obstack_free (&arguments, NULL);
  obstack_free (&argptr, NULL);
  obstack_free (&body, NULL);

  xfree (symbol_name);
}
