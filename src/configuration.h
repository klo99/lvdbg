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
 * @file configuration.h
 *
 * @brief Read and writes configuration parameters.
 *
 * Interface for reading and writing configuration parameters from a specified
 * file. First call conf_create for allocating the configuration structure. Set
 * up the desired parameters and sub groups with conf_add_group. Default values
 * are also set by the conf_add_group. Integer and unsigned integer also have
 * min and max limits that should be set. To load the parameters from a file
 * call conf_load. To retrieve the parameters use the confGet* functions.
 *
 * The configuration files has the form:
 *
 * @code
 * # Comments are done by putting a '#' as the first character of the line.
 * # Comments after the parameter are allowed.
 *
 * # Root parameters:
 * width = 80
 *
 * # Subgroups are done by using '[ GROUP NAME ]'
 * [ Sub group 1 ]
 * output_file = /dev/null
 * A long parameter = ......  # Comment... linebreak -> \
 *                    ...value continues.
 * @endcode
 *
 *
 * String and char parameters are interpreted as c-strings, i.e. characters
 * "\n\t\v\b\r\f\a\\\?\'\"" are un-escaped by conf_load. '\\xhh' and '\\ooo'
 * are valid characters. Chars has length 1. Strings could be empty, e.g:
 *
 * @code
 * [ a group ]
 * text_1=
 * text_2=""
 * text_3=''
 * @endcode
 *
 * The float type could be of the same type as the strtof() function uses, e.g.:
 *
 * @code
 * float 1 = 100.5
 * float 2 = 1.005e2
 * @endcode
 *
 * For bool parameter values "yes", "enable", "true" and "1" are all true. For
 * false "no", "disable", "false" and "0" could be used.
 *
 * Parameter values could be encapsuled by either ''' or '"', which are stripped
 * by conf_load.
 *
 * Parameter names are not parsed so any name are valid, e.g. '&#!' is a valid
 * parameter name. The parameter name must not begin with '#'. Spaces are
 * trimmed before and after the name. This holds for group names as well.
 *
 * @code
 * # Group name 'a group'
 * [  a group    ]
 * # Parameter name "text out"
 *    text out   = "Hello world!"
 * @endcode
 */
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

/*******************************************************************************
 * Typedefs
 ******************************************************************************/
typedef struct configuration_t configuration;
typedef struct conf_parameter_t conf_parameter;
typedef union confValue_t confValue;

/*******************************************************************************
 * Enums
 ******************************************************************************/
/** Values for the parameter type. */
enum
{
  PARAM_BOOL,	 /**< Defines the 'boolean' type.*/
  PARAM_STRING,	 /**< Defines the 'string' type.*/
  PARAM_INT,	 /**< Defines the 'integer' type.*/
  PARAM_UINT,	 /**< Defines the 'unsinged integer' type.*/
  PARAM_CHAR,	 /**< Defines the 'char' type.*/
  PARAM_FLOAT,	 /**< Defines the 'float type' type.*/
};
#define PARAM_LAST PARAM_FLOAT

/*******************************************************************************
 * Structures
 ******************************************************************************/
/** Union to hold the real value of the parameters. */
union confValue_t
{
  int bool_value;	   /**< The bool value. */
  char *string_value;	   /**< The string value value. */
  int int_value;	   /**< The integer value. */
  unsigned int uint_value; /**< The unsigned integer value. */
  char char_value;	   /**< The char value. */
  float float_value;	   /**< The float value. */
};

/** Structure for the parameters. */
struct conf_parameter_t
{
  char *name;		   /**< Name of the parameter. */
  int type;		   /**< Type of value for the parameter. */
  int min;		   /**< Min value for int, uint and float values. */
  int max;		   /**< Max value for int, uint and float values. */
  confValue default_value; /**< The default value if not set by a conf file */
  confValue value;	   /**< The value of the parameter. */
};

/*******************************************************************************
 * Public functions
 ******************************************************************************/
configuration *conf_create ();
void conf_free (configuration * conf);
int conf_add_group (configuration * conf, const char *name,
		    const conf_parameter * parameters);
int conf_load (configuration * conf, const char *filename);
unsigned int conf_get_uint (configuration * conf, const char *group_name,
			    const char *name, int *valid);
int conf_get_bool (configuration * conf, const char *group_name,
		   const char *name, int *valid);
const char *conf_get_string (configuration * conf, const char *group_name,
			     const char *name, int *valid);
int conf_get_int (configuration * conf, const char *group_name,
		  const char *name, int *valid);
char conf_get_char (configuration * conf, const char *group_name,
		    const char *name, int *valid);
float conf_get_float (configuration * conf, const char *group_name,
		      const char *name, int *valid);
#endif
