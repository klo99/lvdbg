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
 * @file vsscanner.h
 *
 * @brief A very simple scanner.
 *
 * The interface for a very simple scanner. 
 *
 */
#ifndef VSSCANNER_H
#define VSSCANNER_H

#define DEF_IDT_LEN 15 /**< The default length of number of tokens per row. */

/*******************************************************************************
 * Typedefs
 ******************************************************************************/
typedef struct vsscanner_t vsscanner;

/*******************************************************************************
 * Structures
 ******************************************************************************/

/**
 * @brief Holds information about tokens found.
 *
 * Hold the information about tokens found.
 */
typedef struct id_entry_t
{
  int id;      /**< The id of the token. */
  int index;   /**< Index of the token, or the column. */
  int len;     /**< Length of token, -1 to end of line. */
} id_entry;

/**
 * @brief The result of scanning a line of text.
 *
 * Holds the result of scanning a text. The first element has always index[0]=0
 * and id[0] = last id from previous line.
 */
typedef struct id_table_t
{
  int len;		    /**< Total number of ids found. */
  id_entry id[DEF_IDT_LEN]; /**< The ids. */

  int size;	     /**< Total size available. */
  id_entry *extra_id;	  /**< Extra ids if it is needed. */
} id_table;

/*******************************************************************************
 * Public functions
 ******************************************************************************/
vsscanner *vsscanner_create (void);
void vsscanner_free (vsscanner * scanner);
void vsscanner_restart (vsscanner * scanner);
int vsscanner_scan (vsscanner * scanner, const char *text, id_table * ids);
int vsscanner_add_rule (vsscanner * scanner, const char *rule, int id,
			int multiline, int word);
#endif
