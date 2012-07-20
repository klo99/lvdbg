/* A simple front end debugger.
   Copyright (C) 2012 Kenneth Olsson

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file text.c
 *
 * @brief The implementation of the text objects.
 *
 * Implements the functions for the text objects.
 */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "text.h"
#include "debug.h"

#define LINE_INCREASE 100 /**<
                           * The number of lines to increase with, when we
                           * need to allocate more lines.
                           */
#define BUF_LEN 512  /**< Max number of bytes a line has. */

/*******************************************************************************
 * Internal structures and enums
 ******************************************************************************/
/** The structure representing a line of text. */
typedef struct Line_t
{
  char *line; /**< The text string on the line. */
  int len;    /**< The length of the text line. */
} Line;

/** The structure for the whole text. */
struct text_t
{
  Line *lines;	   /**< The lines of the text. */
  int nr_of_lines; /**< The number of lines currently in the text. */
  int max_lines;   /**< The number of lines allocated in the text object. */
  int tab_size;	   /**< The tabsize used when converting tabs '\\t' to spaces.*/
};

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Create a new text object.
 *
 * Create a new text object.
 *
 * @return A pointer to the new text object. If failed to create text we will
 *         exit.
 */
text *
text_create ()
{
  text *new_text = NULL;
  int i;

  DINFO (1, "Created new text structure.");

  new_text = (text *) malloc (sizeof (*new_text));
  LOG_ERR_IF_FATAL (new_text == NULL, ERR_MSG_CREATE ("text"));

  new_text->lines = (Line *) malloc (LINE_INCREASE * sizeof (Line));
  LOG_ERR_IF_FATAL (new_text == NULL, ERR_MSG_CREATE ("lines"));

  new_text->max_lines = LINE_INCREASE;
  new_text->nr_of_lines = 0;
  new_text->tab_size = 2;

  for (i = 0; i < LINE_INCREASE; i++)
    {
      new_text->lines[i].len = -1;
      new_text->lines[i].line = NULL;
    }

  return new_text;
}

/**
 * @brief Free a text object.
 *
 * Free a text object and it's resources.
 *
 * @param text The text to be set free.
 */
void
text_free (text * text)
{
  int i;

  assert (text);

  DINFO (3, "Freeing text.");

  for (i = 0; i < text->max_lines; i++)
    {
      if (text->lines[i].line != NULL)
	{
	  free (text->lines[i].line);
	}
    }
  if (text->lines != NULL)
    {
      free (text->lines);
    }
  free (text);
}

/**
 * @brief Load a text from a file.
 *
 * Create a text object and load the text from a file with the name
 * @a file_name.
 *
 * @param file_name The file name.
 *
 * @return A pointer to the created object. NULL if failed to create the object
 *         or loading failed.
 */
text *
text_load_file (const char *file_name)
{
  text *text = NULL;
  int ret;

  text = text_create ();

  ret = text_update_from_file (text, file_name);
  if (ret != 0)
    {
      goto error;
    }
  return text;

error:
  LOG_ERR ("Could not read file: %s", file_name);
  text_free (text);
  return NULL;
}

/**
 * @brief Load text from a file.
 *
 * Replace the current text with the text from a file.
 *
 * @param text The text obejct.
 * @param file_name The name of the file.
 *
 * @return 0 if the load was successful, otherwise -1.
 */
int
text_update_from_file (text * text, const char *file_name)
{
  FILE *file = NULL;
  int ret = -1;
  char *r;
  char buf[BUF_LEN];
  int lines = 0;

  assert (text);

  DINFO (1, "Start reading from '%s'", file_name);

  file = fopen (file_name, "r");
  if (file == NULL)
    {
      LOG_ERR ("Could not open file '%s': %s", file_name, strerror (errno));
      return -1;
    }
  text_clear (text);
  do
    {
      r = fgets (buf, BUF_LEN, file);
      if (r == NULL)
	{
	  DINFO (1, "Could not read more than %d lines: %s", lines,
		 strerror (errno));
	  break;
	}
      lines++;
      ret = text_add_line (text, buf);
      if (ret < 0)
	{
	  LOG_ERR ("Could not add line");
	  goto error;
	}
    }
  while (1);
  ret = 0;

error:
  fclose (file);
  return ret;
}

/**
 * @brief Add a text line to the text.
 *
 * Add a line of text to the text obejct.
 *
 * @param text The text object.
 * @param line The line of text to be added.
 *
 * @return The number of lines in the text object. -1 if failed to add the
 *         line.
 */
int
text_add_line (text * text, const char *line)
{
  int len;
  const char *r;
  char *w;
  int i;

  assert (text);

  len = 0;
  r = line;
  while (*r != '\0' && strchr ("\r\n", *r) == NULL)
    {
      if (*r == '\t')
	{
	  len += text->tab_size;
	}
      else if (isprint (*r))
	{
	  len++;
	}
      else
	{
	  LOG_ERR ("Not a printable char '0x%02X'", *r);
	  return -1;
	}
      r++;
    }

  /*
   * Check if we already has room for the line, otherwise allocate a new line
   * with enought space.
   */
  if (text->lines[text->nr_of_lines].line != NULL
      && sizeof (text->lines[text->nr_of_lines].line) >= len)
    {
      w = text->lines[text->nr_of_lines].line;
    }
  else
    {
      if (text->lines[text->nr_of_lines].line != NULL)
	{
	  free (text->lines[text->nr_of_lines].line);
	}
      w = (char *) malloc (len + 1);
      LOG_ERR_IF_FATAL (w == NULL, ERR_MSG_CREATE ("line"));
      text->lines[text->nr_of_lines].line = w;
    }
  r = line;
  while (*r != '\0' && strchr ("\r\n", *r) == NULL)
    {
      if (*r == '\t')
	{
	  for (i = 0; i < text->tab_size; i++)
	    {
	      *w = ' ';
	      w++;
	    }
	}
      else if (isprint (*r))
	{
	  *w = *r;
	  w++;
	}
      r++;
    }
  *w = '\0';

  text->lines[text->nr_of_lines].len = len;
  DINFO (4, "Added line nr %d of length %d: '%s",
	 text->nr_of_lines,
	 text->lines[text->nr_of_lines].len,
	 text->lines[text->nr_of_lines].line);

  text->nr_of_lines++;
  if (text->nr_of_lines >= text->max_lines)
    {
      text->max_lines += LINE_INCREASE;
      text->lines =
	(Line *) realloc (text->lines, text->max_lines * sizeof (Line));
      LOG_ERR_IF_FATAL (text->lines == NULL, ERR_MSG_CREATE ("lines"));

      for (i = text->max_lines - LINE_INCREASE; i < text->max_lines; i++)
	{
	  text->lines[i].len = -1;
	  text->lines[i].line = NULL;
	}
    }

  return text->nr_of_lines;
}

/**
 * @brief Retrieve a text line.
 *
 * Retrieves a line from the text.
 *
 * @param text The text obejct.
 * @param nr The line number of the line to be retrieved.
 * @param len The length of the line.
 *
 * @return A pointer to the line. Must not be altered or freed.
 */
const char *
text_get_line (text * text, int nr, int *len)
{
  assert (text);

  DINFO (7, "Retrieving line nr %d of %d", nr, text->nr_of_lines);

  if (nr >= text->nr_of_lines || nr < 0)
    {
      return NULL;
    }

  *len = text->lines[nr].len;
  return text->lines[nr].line;
}

/**
 * @brief Clear the text.
 *
 * Remove all text from the obejct. NOTE: No resources are released.
 *
 * @param text The text object.
 */
void
text_clear (text * text)
{
  assert (text);

  text->nr_of_lines = 0;
}

/**
 * @brief Retrieve number of lines in the text.
 *
 * Retrieve the number of lines the text has.
 *
 * @param text The text object.
 *
 * @return Number of lines.
 */
int
text_nr_of_lines (text * text)
{
  assert (text);

  return text->nr_of_lines;
}

/**
 * @brief Dump text to stdout.
 *
 * @param text The test to be dumped.
 */
void
text_dump (text * text)
{
  int i;

  assert (text);

  for (i = 0; i < text->nr_of_lines; i++)
    {
      fprintf (stdout, "%s\r\n", text->lines[i].line);
    }
}
