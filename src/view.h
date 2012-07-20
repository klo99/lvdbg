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
 * @file view.h
 *
 * @brief The view object that the user interacts with.
 *
 * The interface the user are using to interact with the debugger.
 *
 * Defines a set of macros VLOG_* for output messages to lvdbg. The strings
 * are shown to the user and should therefor be encapsuled in '_(...)' for
 * translation.
 */
#ifndef VIEW_H
#define VIEW_H

#include "configuration.h"
#include "objects.h"

#define VLOG_INFO(x, y, z...) view_add_message(x, 0, y, ##z)
#define VLOG_WARN(x, y, z...) view_add_message(x, 1, y, ##z)
#define VLOG_WARN_IF(test,x, y, z...) \
  do { \
    if (test) \
      view_add_message(x, 1, y, ##z); \
  } while (0)

#define VLOG_WARN_IF_RETURN(test, val, x, y, z...) \
  do { \
    if (test) { \
      view_add_message(x, 1, y, ##z); \
      return val; \
    } \
  } while (0)

#define VLOG_ERR(x, y, z...)  view_add_message(x, 2, y, ##z)
#define VLOG_ERR_IF(test,x, y, z...) \
  do { \
    if (test) \
      view_add_message(x, 2, y, ##z); \
  } while (0)

#define VLOG_ERR_IF_RETURN(test, val, x, y, z...) \
  do { \
    if (test) { \
      view_add_message(x, 2, y, ##z); \
      return val; \
    } \
  } while (0)

/*******************************************************************************
 * Typedefs
 ******************************************************************************/
typedef struct view_t view;

/*******************************************************************************
 * Enums
 ******************************************************************************/
/**
 * List of all available windows. Must be the same order as the #out_windows.
 */
enum window_type
{
  WIN_MAIN,
  WIN_MESSAGES,
  WIN_CONSOLE,
  WIN_TARGET,
  WIN_LOG,
  WIN_RESPONSES,
  WIN_BREAKPOINTS,
  WIN_THREADS,
  WIN_LIBRARIES,
  WIN_STACK,
  WIN_FRAME,
  WIN_DISASSAMBLE,
  WIN_REGISTERS,
};

/*******************************************************************************
 * Public functions
 ******************************************************************************/
int view_setup (view ** view, configuration * conf);
void view_cleanup (view * view);

int view_set_status (view * view, int type, const char *status);
int view_set_focus (view * view, int type);
int view_go_to_line (view * view, int type, int line_nr);

int view_add_message (view * view, int level, const char *msg, ...);
int view_add_line (view * view, int type, const char *line, int tag);
void view_update_breakpoints (view * view, breakpoint_table * bpt);
void view_update_threads (view * view, thread_group * thread_groups);
void view_update_libraries (view * view, library * libraries);
void view_update_frame (view * view, stack * stack, int level);
void view_update_stack (view * view, stack * stack);
void view_update_ass (view * view, assembler * ass, int pc);
void view_update_registers (view * view, data_registers * regs);
void view_remove_breakpoint (view * view, const char *file_name, int line_nr);

int view_show_file (view * view, const char *file_name, int line,
		    int mark_stop);

int view_scroll_up (view * view);
int view_scroll_down (view * view);
int view_next_window (view * view, int dir, int type);
int view_move_cursor (view * view, int n);

int view_get_tag (view * view, int *win);
int view_get_cursor (view * view, int *win, int *line_nr,
		     const char **file_name);
void view_toggle_view_mode (view * view);
#endif
