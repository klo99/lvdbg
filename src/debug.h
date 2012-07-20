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
 * @file debug.h
 *
 * @brief Macros for logging information and errors.
 *
 * Definition of macros for logging information and errors. If configured
 * with --enable-debug, the DINFO macro writes to OUT_FILE, otherwise
 * it is not defined.
 *
 * LOG_ERR* is always defined and will log errors if OUT_FILE is non NULL.
 *
 * Programmers must set VERBOSE_LEVEL and OUT_FILE. A higer VERBOSE_LEVEL
 * will produce  more information than a lower value.
 *
 * If OUT_FILE is set to NULL there will not be any output at all, not
 * even LOG_ERR*.
 *
 * DINFO is logging informations.
 * DWARN is logging warnings if compiled with --enable-debug.
 * LOG_ERR writes errors to OUT_FILE.
 * LOG_ERR_IF logs errors if "test" is true.
 * LOG_ERR_IF_RETURN logs errors if "test" is true and will then return
 *                   specified value.
 * LOG_ERR_IF_FATAL Logs error and exit.
 *
 */
#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

extern int VERBOSE_LEVEL;
extern FILE *OUT_FILE;

#define ERR_MSG_CREATE(x)  "Could not create '" x "'"

#define DOUT(level, type, x, y...) \
  do { \
    if (OUT_FILE && level <= VERBOSE_LEVEL) { \
      fprintf(OUT_FILE, "%s:%d %s [ %s ] " x ".\n", __FILE__, __LINE__, \
              __FUNCTION__, type, ##y); \
    } \
  } while (0)

#ifdef DEBUG
#define TRACE DOUT(10, "TRACE", "")
#define D(l, type, x, y...) DOUT(l, type, x, ##y);
#else
#define TRACE
#define D(l, type, x, y...)
#endif

#define LOG_START(x) OUT_FILE = x == NULL ? NULL : fopen(x, "w")
#define LOG_END \
  do { \
    if (OUT_FILE && OUT_FILE != stdout && OUT_FILE != stderr) { \
      fclose(OUT_FILE); \
      OUT_FILE = NULL; \
    } \
  } while (0)

#define LOG_ERR(x, y...) DOUT(0, "ERROR", x, ##y)

#define LOG_ERR_IF(test, x, y...) \
  do { \
    if (test) { \
      LOG_ERR(x, ##y); \
    } \
  } while (0)

#define LOG_ERR_IF_FATAL(test, x, y...) \
  do { \
    if (test) { \
      LOG_ERR(x, ##y); \
      LOG_END; \
      exit(-1); \
    } \
  } while (0)

#define LOG_ERR_IF_RETURN(test, val, x, y...) \
  do { \
    if (test) { \
      LOG_ERR(x, ##y); \
      return val; \
    } \
  } while (0)

#define DWARN(x, y...) D(1, "WARN", x, ##y)
#define DINFO(level, x, y...) D(level, "INFO", x, ##y)

#endif
