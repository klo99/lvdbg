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
 * @file input.h
 *
 * @brief Interprerate the user input.
 *
 * Interpreter of the user input. Reads the input by using the ncurses library.
 * Two main actions groups are window commands, e.g. scroll, change focus. The
 * other group is action commands to the debugger.
 *
 * @todo Handle configuration for key bindings.
 */
#ifndef INPUT_H
#define INPUT_H
#include "view.h"
#include "mi2_interface.h"
#include "configuration.h"

/*******************************************************************************
 * Typedefs
 ******************************************************************************/
typedef struct input_t input;

/*******************************************************************************
 * Public functions
 ******************************************************************************/
input *input_create (view * view, mi2_interface * mi2, configuration * conf,
		     int fd);
void input_free (input * input);
int input_get_input (input * input);
#endif
