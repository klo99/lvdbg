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
 * @file mi2_parser.h
 *
 * @brief The mi2 parser interface.
 *
 * Handles the parsing of the mi2 records. Calling view function when the view
 * needs to be updated.
 */
#ifndef MI2_PARSER_H
#define MI2_PARSER_H

#include "view.h"
#include "configuration.h"

/*******************************************************************************
 * Typedefs
 ******************************************************************************/
typedef struct mi2_parser_t mi2_parser;

/*******************************************************************************
 * Public functions
 ******************************************************************************/
mi2_parser *mi2_parser_create (view * view, configuration * conf);
void mi2_parser_free (mi2_parser * parser);

int mi2_parser_parse (mi2_parser * parser, char *line, int *update,
		      char **regs);
void mi2_parser_set_frame (mi2_parser * parser, int frame);
int mi2_parser_set_thread (mi2_parser * parser, int id);
int mi2_parser_get_thread (mi2_parser * parser);

breakpoint *mi2_parser_get_bp (mi2_parser * parser, const char *file_name,
			       int line_nr);
void mi2_parser_remove_bp (mi2_parser * parser, int number);
int mi2_parser_get_location (mi2_parser * parser, char **file, int *line);
void mi2_parser_toggle_disassemble (mi2_parser * parser);
#endif
