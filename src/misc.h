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
 * @file misc.h
 *
 * @brief Utility functions.
 *
 * A set of utility functions used by lvdbg.
 *
 * @li unescape Unescapes a c-string to a normal string.
 * @li get_next_param Gets the next parameter in a string.
 * @li safe_write Writes to a file descriptor in a safe manner.
 *
 * Macro:
 * @li LPRINT
 */
#ifndef MISC_H
#define MISC_H

/**
 * Lvdbg's snprint. If the result does not fit @a str more merory is allocated.
 *
 * @code
 * char buf[512];
 * char *p = buf;
 * int size = 512;
 *
 * LPRINT(p, p!=buf, size, "%s", "Bill rules");
 * if (p == NULL)
 *   {
 *     bad...
 *   }
 * ...
 * if (p != buf)
 *   {
 *     free (p);
 *   }
 *
 *  OR:
 *
 * char *p = NULL;
 * int size = 0;
 *
 * LPRINT(p, 1, size, "%s", "Bill rules");
 * ...
 * free (p);
 *
 * @endcode
 */
#define LPRINT(str, test, size, fmt, p...) \
  do \
    { \
      int lvdbg_ret; \
      char *lvdbg_p; \
      do \
        { \
	  lvdbg_ret = snprintf (str, size, fmt, ##p); \
	  if (lvdbg_ret > -1 && lvdbg_ret < size ) \
	    { \
	      break; \
	    } \
	  if (lvdbg_ret > -1) \
	    { \
	      size = lvdbg_ret + 1; \
	    } \
	  else \
	    { \
	      size *= 2; \
	    } \
	  if (test) \
	    { \
	      lvdbg_p = (char*) realloc (str, size); \
	    } \
	  else \
	    { \
	      lvdbg_p = (char*) malloc (size); \
	    } \
	  if (lvdbg_p == NULL) \
	    { \
	      LOG_ERR_IF_FATAL (1, "Memory"); \
	    } \
	  str = lvdbg_p; \
	} \
      while (1); \
    } \
  while (0)

/*******************************************************************************
 * Public functions
 ******************************************************************************/
int unescape (char *text, const char *skip);
int get_next_param (char *text, char **name, char **value, char **nextptr);
int safe_write (int fd, const char *msg);
#endif
