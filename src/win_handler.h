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
 * @file win_handler.h
 *
 * @brief Handles all actions regarding a window.
 *
 * Interface for the actions on windows.
 *
 */
#ifndef WIN_HANDLER_H
#define WIN_HANDLER_H

#define WIN_PROP_MARKS  0x0001 /**< Window has markers. */
#define WIN_PROP_CURSOR 0x0002 /**< Window has cursor. */
#define WIN_PROP_SYNTAX 0x0004 /**< Window has syntax highlighting. */
#define WIN_PROP_BORDER 0x0008 /**< Window has border. */

/*******************************************************************************
 * Typedef / Structures
 ******************************************************************************/
typedef struct Win_t Win;

typedef struct win_attribute_t
{
  int color;
  int attr;
} win_attribute;

/**
 * @brief Windows properties.
 *
 * Structure to hold properties of a window.
 */
typedef struct win_properties_t
{
  int indent;	  /**< Number of spaces used at beginning of a text line. */
  int properties; /**< Window properties. */
  win_attribute *attributes;	/**< Color attributes used in window. */
  int nr_of_attributes;		/**< Nr of attributes defined. */
  char *scan_definitions; /**< Scanner definitions. */
} win_properties;

/*******************************************************************************
 * Public functions
 ******************************************************************************/
Win *win_create (int starty, int startx, int height, int width,
		 win_properties * props);
void win_free (Win * win);
void win_set_status (Win * win, const char *line);
int win_add_line (Win * win, const char *line, int scroll, int tag);
int win_load_file (Win * win, const char *filename);
int win_scroll (Win * win, int nr_of_lines);
void win_to_top (Win * win);
int win_go_to_line (Win * win, int line_nr);
int win_move_cursor (Win * win, int n);
int win_move (Win * win, int n);
void win_set_focus (Win * win, int focus);
void win_clear (Win * win);
int win_set_mark (Win * win, int line, int nr, char mark);
int win_get_tag (Win * win);
int win_get_cursor (Win * win);
const char *win_get_filename (Win * win);
const char *win_get_line (Win * win, int line_nr);
void win_dump (Win * win);
#endif
