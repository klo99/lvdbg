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
 * @file text.h
 *
 * @brief Interface for the text objects used by the windows.
 *
 * The interface for the text objects, that the view uses for it's windows.
 *
 */
#ifndef TEXT_H
#define TEXT_H

/*******************************************************************************
 * Typedefs
 ******************************************************************************/
typedef struct text_t text;

/*******************************************************************************
 * Public functions
 ******************************************************************************/
void text_free (text * text);
text *text_load_file (const char *file_name);
int text_update_from_file (text * text, const char *file_name);
text *text_create ();
int text_add_line (text * text, const char *line);
const char *text_get_line (text * text, int nr, int *len);
void text_clear (text * text);
int text_nr_of_lines (text * text);
void text_dump (text * text);
#endif
