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
 * @file misc.c
 *
 * @brief Implements utility functions.
 *
 * Implements utility functions.
 *
 * @li unescape Unescapes a c-string to a normal string.
 * @li get_next_param Gets the next parameter in a string.
 * @li safe_write Writes to a file descriptor in a safe manner.
 *
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include "misc.h"
#include "debug.h"

#define ESCAPE_CHAR '\\'		   /**< The escape character. */
#define C_ESCAPES_CHARS "ntvbrfa\\?'\""	   /**< Valid escape chars in a
                                            *   c-string.
                                            */
#define C_ESCAPES "\n\t\v\b\r\f\a\\\?\'\"" /**< The real characters corre-
                                            *   sponding to the escape chars.
                                            */

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Unescape a c-string.
 *
 * Unescapes a c-string to a normal string. If the string is surrounded by
 * either '\'' or '\"' they are removed. If @a skip is non NULL all characters
 * in skip will be excluded in the output.
 *
 * The input text string will be overwritten.
 *
 * @param text The c-string that should be converted.
 * @param skip If non NULL the characters in skip will be excluded in the
 *             output.
 *
 * @return 0 if successful. If an error occurs -1 is returned.
 */
int
unescape (char *text, const char *skip)
{
  char *r;
  char *w;
  char *p;
  char nr[4];
  int in_quote = 0;
  int in_dquote = 0;

  DINFO (7, "unescpe '%s'", text);
  p = &(text[strlen (text) - 1]);
  while (p > text && isspace (*p))
    {
      p--;
    }
  if ((*text == '"' || *text == '\'') && *text == *p)
    {
      *p = '\0';
      r = text + 1;
    }
  else
    {
      r = text;
    }
  w = text;

  while (*r)
    {
      if (*r == ESCAPE_CHAR && in_quote == 0 && in_dquote == 0)
	{
	  if ((p = strchr (C_ESCAPES_CHARS, *(r + 1))) != NULL)
	    {
	      /* Convert the escape character. */
	      *w = C_ESCAPES[p - C_ESCAPES_CHARS];
	      r += 2;
	    }
	  else if (*(r + 1) == 'x')
	    {
	      /* Convert from hexadecimal. */
	      if (isxdigit (r[2]) && isxdigit (r[3]))
		{
		  nr[0] = r[2];
		  nr[1] = r[3];
		  nr[2] = '\0';
		  *w = (char) strtol (nr, NULL, 16);
		  r += 4;
		}
	      else
		{
		  goto error;
		}
	    }
	  else if (r[1] >= '0' && r[1] <= '7')
	    {
	      /* Convert the octal to decimal. */
	      nr[0] = r[1];
	      if (r[2] >= '0' && r[2] <= '7')
		{
		  nr[1] = r[2];
		  if (r[3] >= '0' && r[3] <= '7')
		    {
		      nr[2] = r[3];
		      nr[3] = '\0';
		      r += 4;
		    }
		  else
		    {
		      nr[2] = '\0';
		      r += 3;
		    }
		}
	      else
		{
		  r += 2;
		  nr[1] = '\0';
		}
	      *w = strtol (nr, NULL, 8);
	    }
	  else
	    {
	      /* Not a valid escape sequence. */
	      goto error;
	    }
	}
      else
	{
	  /* 'Normal' characters are copied. */
	  *w = *r;
	  r++;
	}

      if (in_quote)
	{
	  if (*(r - 1) == '\'' && *(r - 2) != '\\')
	    {
	      in_quote = 0;
	    }
	}
      else if (in_dquote)
	{
	  if (*(r - 1) == '\"' && *(r - 2) != '\"')
	    {
	      in_quote = 0;
	    }
	}
      else if (*w == '\'')
	{
	  in_quote = 1;
	}
      else if (*w == '\"')
	{
	  in_quote = 1;
	}
      if (skip == NULL || strchr (skip, *w) == NULL)
	{
	  w++;
	}
    }
  *w = '\0';
  DINFO (7, "After '%s'", text);
  return 0;

error:
  LOG_ERR ("Failed to convert '%s'", text);
  *w = '\0';
  return -1;
}

/**
 * @brief Get the next parameter value in text.
 *
 * Get the next parameter in the input string. The input \a text will be
 * overwritten. The input could have the following formats or a combination.
 *
 * @li var="5".
 * @li var={varB="1",varC="2"}
 * @li var=[varB="1",varC="2"]
 * @li {var="5"}
 * @li [var="6"]
 *
 * The name is pointed to the parameter \a name if it is available. The
 * \a value is pointed to the parameter value. The next is pointed to the next
 * element. E.g. in var="5" the name would be 'var', value would be '"5"' and
 * next would be pointing to the next character in text.
 *
 * @param text The text that should be parsed.
 * @param name A pointer to the parameter name if there is any in text.
 * @param value A pointer to the parameter value.
 * @param next A pointer to the next parameter.
 *
 * @return 0 on success. If the text could not be parsed -1 is returned.
 */
int
get_next_param (char *text, char **name, char **value, char **next)
{
  char *p;
  int in_quote = 0;
  int in_double = 0;
  int curl = 0;
  int brace = 0;
  int type = -1;

  assert (text);
  assert (name);
  assert (value);
  assert (next);

  DINFO (7, "Parsing: '%s'", text);

  *name = NULL;
  *value = NULL;
  *next = NULL;

  if (*text == '\0' || *text == ']' || *text == '}')
    {
      return 0;
    }

  p = text;
  while (*p)
    {
      if (in_quote)
	{
	  if (*p == '\'' && *(p - 1) != '\\')
	    {
	      in_quote = 0;
	      if (brace == 0 && curl == 0)
		{
		  goto out;
		}
	    }
	}
      else if (in_double)
	{
	  if (*p == '\"' && *(p - 1) != '\\')
	    {
	      in_double = 0;
	      if (brace == 0 && curl == 0)
		{
		  goto out;
		}
	    }
	}
      else if (*p == '[')
	{
	  if (type == -1)
	    {
	      type = '[';
	      *value = p + 1;
	    }
	  brace++;
	}
      else if (*p == '{')
	{
	  if (type == -1)
	    {
	      type = '{';
	      *value = p + 1;
	    }
	  curl++;
	}
      else if (*p == '\"')
	{
	  in_double = 1;
	  if (type == -1)
	    {
	      type = '\"';
	      *value = p + 1;
	    }
	}
      else if (*p == '\'')
	{
	  in_quote = 1;
	  if (type == -1)
	    {
	      type = '\'';
	      *value = p + 1;
	    }
	}
      else if (*p == ']')
	{
	  brace--;
	  if (brace == 0 && curl == 0)
	    {
	      goto out;
	    }
	  if (brace < 0)
	    {
	      goto error;
	    }
	}
      else if (*p == '}')
	{
	  curl--;
	  if (brace == 0 && curl == 0)
	    {
	      goto out;
	    }
	  if (curl < 0)
	    {
	      goto error;
	    }
	}
      else if (isalpha (*p))
	{
	  if (type == -1)
	    {
	      *name = p;
	      while (*p && *p != '=')
		{
		  p++;
		}
	      if (*p != '=')
		{
		  goto error;
		}
	      *p = '\0';
	    }
	}
      else
	{
	  if (type == -1 && *p != ',' && *p != ' ')
	    {
	      LOG_ERR ("Strange char '%c'=0x%02X in '%s'", *p, *p, text);
	      goto error;
	    }
	}
      p++;
    }

error:
  LOG_ERR ("Could not parse '%s' [Might me altered here]", text);
  LOG_ERR ("Type: '%c'", isprint (type) ? type : '.');
  LOG_ERR ("Name: '%s'", *name ? *name : "");
  LOG_ERR ("Value: '%s'", *value ? *value : "");
  LOG_ERR ("Next: '%s'", *next ? *next : "");
  return -1;

out:
  if (type == '\"' && *value != NULL)
    {
      unescape (*value, NULL);
    }

  *next = p + 1;
  *p = '\0';
  DINFO (5, "Type: '%c'", isprint (type) ? type : '.');
  DINFO (5, "Name: '%s'", *name ? *name : "");
  DINFO (5, "Value: '%s'", *value ? *value : "");
  DINFO (5, "Next: '%s'", *next ? *next : "");
  return type;
}

/**
 * @brief Write a message to a file.
 *
 * Insure that the whole message are written if it is possible. If other errors
 * than EAGAIN and EINTR happens, the write will fail.
 *
 * @param fd The file descriptor.
 * @param msg The message to be sent.
 *
 * @return 0 if the whole message was sent. -1 if the message was not sent.
 */
int
safe_write (int fd, const char *msg)
{
  int to_write;
  int written;
  int ret;

  written = 0;
  to_write = strlen (msg);

  while (written < to_write)
    {
      ret = write (fd, msg + written, to_write - written);
      if (ret < 0)
	{
	  if (errno == EAGAIN || errno == EINTR)
	    {
	      DINFO (3, "Write failed, will try again: %m");
	      continue;
	    }
	  LOG_ERR ("Write failed: '%s'", strerror (errno));
	  return -1;
	}
      written += ret;
    }
  return 0;
}
