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

#include "mp4h.h"

#include <sys/stat.h>

/* Size of initial in-memory buffer size for diversions.  Small diversions
   would usually fit in.  */
#define INITIAL_BUFFER_SIZE 512

/* Maximum value for the total of all in-memory buffer sizes for
   diversions.  */
#define MAXIMUM_TOTAL_SIZE (512 * 1024)

/* Size of buffer size to use while copying files.  */
#define COPY_BUFFER_SIZE (32 * 512)
 
#ifdef HAVE_TMPFILE
extern FILE *tmpfile ();
#endif

/* Output functions.  Most of the complexity is for handling cpp like
   sync lines.
  
   This code is fairly entangled with the code in input.c, and maybe it
   belongs there?  */

/* In a struct diversion, only one of file or buffer be may non-NULL,
   depending on the fact output is diverted to a file or in memory
   buffer.  Further, if buffer is NULL, then pointer is NULL, size and
   unused are zero.  */

struct diversion
  {
    FILE *file;                 /* diversion file on disk */
    char *buffer;               /* in-memory diversion buffer */
    int size;                   /* usable size before reallocation */
    int used;                   /* used length in characters */
  };

/* Table of diversions.  */
static struct diversion *diversion_table;

/* Number of entries in diversion table.  */
static int diversions;

/* Total size of all in-memory buffer sizes.  */
static int total_buffer_size;

/* The number of the currently active diversion.  This variable is
   maintained for the `divnum' builtin function.  */
int current_diversion;

/* Current output diversion, NULL if output is being currently discarded.  */
static struct diversion *output_diversion;

/* Values of some output_diversion fields, cached out for speed.  */
static FILE *output_file;       /* current value of (file) */
static char *output_cursor;     /* current value of (buffer + used) */
static int output_unused;       /* current value of (size - used) */

/* Number of input line we are generating output for.  */
int output_current_line;

/* Set to true after a left angle, to remove star before tag name  */
static boolean after_left_angle = FALSE;


/*------------------------.
| Output initialisation.  |
`------------------------*/

void
output_init (void)
{
  diversion_table = (struct diversion *) xmalloc (sizeof (struct diversion));
  diversions = 1;
  diversion_table[0].file = stdout;
  diversion_table[0].buffer = NULL;
  diversion_table[0].size = 0;
  diversion_table[0].used = 0;

  total_buffer_size = 0;
  current_diversion = 0;
  output_diversion = diversion_table;
  output_file = stdout;
  output_cursor = NULL;
  output_unused = 0;
}

/*----------------------.
| Output deallocation.  |
`----------------------*/

void
output_deallocate (void)
{
  xfree (diversion_table);
}

#ifndef HAVE_TMPFILE

#ifndef HAVE_MKSTEMP

/* This implementation of mkstemp(3) does not avoid any races, but its
   there.  */

#include <fcntl.h>

static int
mkstemp (const char *tmpl)
{
  mktemp (tmpl);
  return open (tmpl, O_RDWR | O_TRUNC | O_CREAT, 0600);
}

#endif /* not HAVE_MKSTEMP */

/* Implement tmpfile(3) for non-USG systems.  */

static FILE *
tmpfile (void)
{
  char buf[32];
  int fd;

  strcpy (buf, "/tmp/mp4hXXXXXX");
  fd = mkstemp (buf);
  if (fd < 0)
    return NULL;

  unlink (buf);
  return fdopen (fd, "w+");
}

#endif /* not HAVE_TMPFILE */

/*-----------------------------------------------------------------------.
| Reorganize in-memory diversion buffers so the current diversion can    |
| accomodate LENGTH more characters without further reorganization.  The |
| current diversion buffer is made bigger if possible.  But to make room |
| for a bigger buffer, one of the in-memory diversion buffers might have |
| to be flushed to a newly created temporary file.  This flushed buffer  |
| might well be the current one.                                         |
`-----------------------------------------------------------------------*/

static void
make_room_for (int length)
{
  int wanted_size;

  /* Compute needed size for in-memory buffer.  Diversions in-memory
     buffers start at 0 bytes, then 512, then keep doubling until it is
     decided to flush them to disk.  */

  output_diversion->used = output_diversion->size - output_unused;

  for (wanted_size = output_diversion->size;
       wanted_size < output_diversion->used + length;
       wanted_size = wanted_size == 0 ? INITIAL_BUFFER_SIZE : wanted_size * 2)
    ;

  /* Check if we are exceeding the maximum amount of buffer memory.  */

  if (total_buffer_size - output_diversion->size + wanted_size
      > MAXIMUM_TOTAL_SIZE)
    {
      struct diversion *selected_diversion;
      int selected_used;
      struct diversion *diversion;
      int count;

      /* Find out the buffer having most data, in view of flushing it to
         disk.  Fake the current buffer as having already received the
         projected data, while making the selection.  So, if it is
         selected indeed, we will flush it smaller, before it grows.  */

      selected_diversion = output_diversion;
      selected_used = output_diversion->used + length;

      for (diversion = diversion_table + 1;
           diversion < diversion_table + diversions;
           diversion++)
        if (diversion->used > selected_used)
          {
            selected_diversion = diversion;
            selected_used = diversion->used;
          }

      /* Create a temporary file, write the in-memory buffer of the
         diversion to this file, then release the buffer.  */

      selected_diversion->file = tmpfile ();
      if (selected_diversion->file == NULL)
        MP4HERROR ((EXIT_FAILURE, errno,
          _("ERROR: Cannot create temporary file for diversion")));

      if (selected_diversion->used > 0)
        {
          count = fwrite (selected_diversion->buffer,
                          (size_t) selected_diversion->used,
                          1,
                          selected_diversion->file);
          if (count != 1)
            MP4HERROR ((EXIT_FAILURE, errno,
              _("ERROR: Cannot flush diversion to temporary file")));
        }

      /* Reclaim the buffer space for other diversions.  */

      free (selected_diversion->buffer);
      total_buffer_size -= selected_diversion->size;

      selected_diversion->buffer = NULL;
      selected_diversion->size = 0;
      selected_diversion->used = 0;
    }

  /* Reload output_file, just in case the flushed diversion was current.  */

  output_file = output_diversion->file;
  if (output_file)
    {

      /* The flushed diversion was current indeed.  */

      output_cursor = NULL;
      output_unused = 0;
    }
  else
    {

      /* The buffer may be safely reallocated.  */

      output_diversion->buffer
        = xrealloc (output_diversion->buffer, (size_t) wanted_size);

      total_buffer_size += wanted_size - output_diversion->size;
      output_diversion->size = wanted_size;

      output_cursor = output_diversion->buffer + output_diversion->used;
      output_unused = wanted_size - output_diversion->used;
    }
}

/*----------------------------------------------------.
| Remove special characters added in strings.         |
| Replacement is made in place, and this function     |
| returns how many characters are removed.            |
| IMPORTANT WARNING: string must be null-terminated.  |
`----------------------------------------------------*/

int
remove_special_chars (char *s, boolean restore_quotes)
{
  int offset = 0;
  register char *cp;

  /*  For efficiency reason, this loop is divided into 2 parts.
      Until a special character is removed, there is nothing to do.
      After that, string is shifyed.  */
  if (restore_quotes)
    {
      for (cp=s; *cp != '\0' && *cp != CHAR_LQUOTE && *cp != CHAR_RQUOTE
             && *cp != CHAR_BGROUP && *cp != CHAR_EGROUP; cp++)
        {
          if (*cp == CHAR_QUOTE)
            *cp = '"';
        }
    }
  else
    {
      for (cp=s; *cp != '\0' && *cp != CHAR_LQUOTE && *cp != CHAR_RQUOTE
             && *cp != CHAR_BGROUP && *cp != CHAR_EGROUP; cp++)
        ;
    }

  if (*cp == '\0')
    return offset;

  for ( ; *cp != '\0'; cp++)
    {
      if (*cp == CHAR_LQUOTE || *cp == CHAR_RQUOTE
       || *cp == CHAR_BGROUP || *cp == CHAR_EGROUP)
        offset++;
      else if ((*cp == CHAR_QUOTE) && restore_quotes)
        *(cp-offset) = '"';
      else
        *(cp-offset) = *cp;
    }
  *(cp-offset) = '\0';
  return offset;
}

/*----------------------------------------------------.
| Remove leading and trailing stars in tag names.     |
`----------------------------------------------------*/

static void
remove_stars (char *s, int *length_ptr)
{
  int offset, i;
  register char *cp;
  int length = *length_ptr;

  offset = 0;
  cp = s;
  if (*s == '*' && after_left_angle &&
          !(exp_flags & EXP_LEAVE_LEADING_STAR))
    {
      offset++;
      cp++;
    }

  for (i=0; i<length; cp++, i++)
    {
      *(cp-offset) = *cp;
      if (*cp == '<' && i<length-1)
        {
          if (*(cp+1) == '*' && !(exp_flags & EXP_LEAVE_LEADING_STAR))
            {
              offset++;
              cp++, i++;
              continue;
            }
          if (exp_flags & EXP_LEAVE_TRAILING_STAR)
            continue;

          if (*(cp+1) == '/')
            {
              cp++, i++;
              *(cp-offset) = *cp;
            }
          cp++, i++;
          if (i<length && IS_ALPHA (*cp))
            {
              while (i<length && IS_ALNUM(*cp))
                {
                  *(cp-offset) = *cp;
                  cp++, i++;
                }
              if (i<length && *cp == '*' )
                offset++;
              else
                {
                  cp--, i--;
                }
            }
        }
    }
  *length_ptr -= offset;
}

/*------------------------------------------------------------------------.
| Output one character CHAR, when it is known that it goes to a diversion |
| file or an in-memory diversion buffer.                                  |
`------------------------------------------------------------------------*/

#define OUTPUT_CHARACTER(Char) \
  if (output_file)                                                      \
    putc ((Char), output_file);                                         \
  else if (output_unused == 0)                                          \
    output_character_helper ((Char));                                   \
  else                                                                  \
    (output_unused--, *output_cursor++ = (Char))

static void
output_character_helper (int character)
{
  make_room_for (1);

  if (output_file)
    putc (character, output_file);
  else
    {
      *output_cursor++ = character;
      output_unused--;
    }
}

/*------------------------------------------------------------------------.
| Output one TEXT having LENGTH characters, when it is known that it goes |
| to a diversion file or an in-memory diversion buffer.                   |
`------------------------------------------------------------------------*/

static void
output_text (const char *text, int length)
{
  int count;

  if (!output_file && length > output_unused)
    make_room_for (length);

  if (output_file)
    {
      count = fwrite (text, length, 1, output_file);
      if (count != 1)
        MP4HERROR ((EXIT_FAILURE, errno, _("ERROR: Copying inserted file")));
    }
  else
    {
      memcpy (output_cursor, text, (size_t) length);
      output_cursor += length;
      output_unused -= length;
    }
}

/*-------------------------------------------------------------------------.
| Add some text into an obstack OBS, taken from TEXT, having LENGTH        |
| characters.  If OBS is NULL, rather output the text to an external file  |
| or an in-memory diversion buffer.  If OBS is NULL, and there is no       |
| output file, the text is discarded.                                      |
|                                                                          |
| If we are generating sync lines, the output have to be examined, because |
| we need to know how much output each input line generates.  In general,  |
| sync lines are output whenever a single input lines generates several    |
| output lines, or when several input lines does not generate any output.  |
`-------------------------------------------------------------------------*/

void
shipout_text (struct obstack *obs, char *text, int length)
{
  static boolean start_of_output_line = TRUE;
  char line[20];
  const char *cursor;

  /* If output goes to an obstack, merely add TEXT to it.  */

  if (obs != NULL)
    {
      obstack_grow (obs, text, length);
      return;
    }

  /* Do nothing if TEXT should be discarded.  */

  if (output_diversion == NULL)
    return;

  /* `text' is always a null-terminated string, so we can compute
     its length.  */
  if (strlen (text) > length)
    *(text+length) = '\0';

  /* Restitute some special characters  */
  length -= remove_special_chars (text, TRUE);

  /* Remove leading and trailing stars in tag names  */
  if (!(exp_flags & (EXP_LEAVE_TRAILING_STAR | EXP_LEAVE_TRAILING_STAR)))
    remove_stars (text, &length);

  if (length <= 0)
    {
      after_left_angle = FALSE;
      return;
    }

  /* Output TEXT to a file, or in-memory diversion buffer.  */

  if (!sync_output)
    switch (length)
      {

        /* In-line short texts.  */

      case 8: OUTPUT_CHARACTER (*text); text++;
      case 7: OUTPUT_CHARACTER (*text); text++;
      case 6: OUTPUT_CHARACTER (*text); text++;
      case 5: OUTPUT_CHARACTER (*text); text++;
      case 4: OUTPUT_CHARACTER (*text); text++;
      case 3: OUTPUT_CHARACTER (*text); text++;
      case 2: OUTPUT_CHARACTER (*text); text++;
      case 1: OUTPUT_CHARACTER (*text);
              break;

        /* Optimize longer texts.  */

      default:
        output_text (text, length);
        text += length - 1;
      }
  else
    for (; length-- > 0; text++)
      {
        if (start_of_output_line)
          {
            start_of_output_line = FALSE;
            output_current_line++;

#ifdef DEBUG_OUTPUT
            printf ("DEBUG: cur %d, cur out %d\n",
                    current_line, output_current_line);
#endif

            /* Output a `#line NUM' synchronisation directive if needed.
               If output_current_line was previously given a negative
               value (invalidated), rather output `#line NUM "FILE"'.  */

            if (output_current_line != current_line)
              {
                sprintf (line, "#line %d", current_line);
                for (cursor = line; *cursor; cursor++)
                  OUTPUT_CHARACTER (*cursor);
                if (output_current_line < 1)
                  {
                    OUTPUT_CHARACTER (' ');
                    OUTPUT_CHARACTER ('"');
                    for (cursor = current_file; *cursor; cursor++)
                      OUTPUT_CHARACTER (*cursor);
                    OUTPUT_CHARACTER ('"');
                  }
                OUTPUT_CHARACTER ('\n');
                output_current_line = current_line;
              }
          }
        OUTPUT_CHARACTER (*text);
        if (*text == '\n')
          start_of_output_line = TRUE;
      }

  if (*text == '<')
    after_left_angle = TRUE;
  else
    after_left_angle = FALSE;
}
