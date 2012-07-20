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
 * @file win_form.h
 *
 * @brief Interface for a simple input window.
 *
 * Setup a simple input form for retrieve different user set values. Draw
 * a window with parameters that the user can alter. Supported types are
 * int, bool, string and enumerations.
 *
 * NOTE: Assumes that curses is already initiated with initscr();
 *
 * Example:
 * @code
 * int ret;\n
 * char* e_def[] = {"Item 1", "Item 2", "Last Item", NULL};\n
 * Field fields[] = {\n
 *   {"Int", "Help", INPUT_TYPE_INT, {.int_value = 0}, 0, 10, NULL},\n
 *   {"Str", "Help", INPUT_TYPE_STRING, {.string_value = NULL}, 0, 0, NULL},\n
 *   {"Bool", "Help", INPUT_TYPE_BOOL, {.bool_value = 1}, 0, 0, NULL},\n
 *   {"Enum", "Help", INPUT_TYPE_ENUM, {.enum_value = 1}, 0, 0, e_def},\n
 * };\n
 * \n
 * ret = form_run(fields, "A test");\n
 * ...
 * @endcode
 */
#ifndef WIN_FORM_H
#define WIN_FORM_H

/**
 * Possible fielt types.
 */
enum inputType_t
{
  INPUT_TYPE_INT,    /**< Field integer type. */
  INPUT_TYPE_STRING, /**< Field string type. */
  INPUT_TYPE_BOOL,   /**< Field bool type. */
  INPUT_TYPE_ENUM,   /**< Field enum type. Simalar to list boxes. */
};

/*******************************************************************************
 * Structures
 ******************************************************************************/
/**
 * @brief Struct for defining an input field.
 *
 * Structure to define and hold an input value.
 */
typedef struct input_field_t
{
  const char *text;	  /**< text label to be shown at the field. */
  const char *help;	  /**< Help for the field. Not in used. */
  int type;		  /**< Type of field, see inputType_t. */
  union
  {
    int int_value;	  /**< Integer value of the field. */
    char *string_value;	  /**< Stringr value of the field. */
    int bool_value;	  /**< Boolean value of the field. */
    int enum_value;	  /**< Enum value of the field. */
  };
  int min;		  /**< Limit for integer value. Not in use. */
  int max;		  /**< Limit for integer value. Not in use. */
  const char **enum_text; /**<
                           * A list of strings defining the different enum
                           * values. Must end with a NULL.
                           */
} input_field;

/*******************************************************************************
 * Public functions
 ******************************************************************************/
int form_run (input_field * fields, const char *header);
int form_selection (char **list, const char *header);
#endif
