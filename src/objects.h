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
 * @file objects.h
 *
 * @brief Common objects to a debugger.
 *
 * Defines structur to common objects for a debugger.
 *
 * Defined objects are:
 *
 * @li Breakpoint table. A table of all breakpoints.
 * @li Breakpoint. Information about a breakpoint or watchpoint.
 * @li Thread group. Information about a thread group.
 * @li Thread. Information about a thread.
 * @li Library. All libraries loaded.
 * @li Stack and frame. Stack and frame information.
 * @li Assembler information. Assembler information.
 * @li Registers.
 *
 * @todo Split in several files?
 */
#ifndef OBJECTS_H
#define OBJECTS_H

#include <inttypes.h>
/*******************************************************************************
 * Defines
 ******************************************************************************/

#define BP_TYPE_BREAKPOINT 0
#define BP_TYPE_WATCHPOINT 1

/*******************************************************************************
 * Structures
 ******************************************************************************/
/** @name Breakpoints
 *
 * A breakpoint or watchpoint are stored in table, which is used by the parser
 * to monitoring the breakpoint actions.
 */
/*@{*/
/**
 * Stucture for breakpoints and watchpoints. The points are stored in a table.
 */
typedef struct breakpoint_t
{
  int number;  /**< Breakpoint number. */

  int type;  /**<
	      * Breakpoint (BP_TYPE_BREAKPOINT) or watchpoint
	      * (BP_TYPE_WATCHPOINT) type.
	      */
  int disp;    /**< Disposition, keep or delete. */
  int enabled; /**< Enabled. */
  int addr;    /**< The address of the breakpoint. */
  char *func;  /**< The function where the breakpoint is located. */
  char *file;  /**< The file where the breakpoint is located. */
  char *fullname; /**< The full filename. */
  int line;	  /**< The line number. */
  int thread;	  /**< The thread id associated with the breakpoint. */
  int times;	  /**< Number of times run but not hit. See ignore. */
  char *cond;	  /**< Condition expression when the breakpoint should hit. */
  int ignore;	  /**< How many times the breakpoint should be ignored. */
  char *original_location; /**< Previous location. */

  char *expression; /**< Watchpoint expression. */
  char *value; /**< The current value of the watchpoint. */
} breakpoint;

/**
 * Where the breakpoints and watchpoints are stored.
 */
typedef struct breakpoint_table_t
{
  int cols; /**< Could be removed? */
  int rows; /**<
	     * Number of rows in table. Note that the rows could be empty
	     * if a breakpoint is deleted.
	     */

  breakpoint **breakpoints; /**< The breakpoints. */
  int in_use;  /**< Number of breakpoints in use. */
} breakpoint_table;
/*@}*/

/**
 * @name Library.
 *
 * Structure to hold the currently loaded libraries. The libraries are stored
 * in a list.
 */
/*@{*/
typedef struct library_t
{
  char *id;	     /**< Id of the library. */
  char *target_name; /**< The target id. */
  char *host_name;   /**< The host id. */
  int symbols_loaded; /**< Set to 1 if the symbols of the library are loaded. */

  struct library_t *next; /**< The next library. */
} library;
/*@}*/

/**
 * @name Stack.
 *
 * The stack is built up by several frames. The stack is associated with a
 * thread.
 */
/*@{*/
/**Holds either an argument or a local variable. */
typedef struct variable_t
{
  char *name;  /**< The variable's name. */
  char *type;  /**< The type of the variable. */
  char *value; /**< The variable's value. */

  struct variable_t *next; /**< Next variable. */
} variable;

/** The information of a frame. */
typedef struct frame_t
{
  int addr;   /**< The address of the frame's function. */
  char *func; /**< The function for the frame. */
  variable *args; /**< List of arguments for the function. */
  variable *variables; /**< List of variables. */
  char *file; /**< The current file of the function. */
  char *fullname; /**< Fullname of the file. */
  int line; /**< Line number of the current position. */
} frame;

/**
 * The stack object. The stack consists of one or more frames and is associated
 * with a thread and a core.
 */
typedef struct stack_t
{
  int depth;	/**< The number of known frames in the stack. */
  int thread_id; /**< The thread id. */
  int core;	 /**< The core where the stack is. */

  int max_depth; /**< The available depth of the stack. */
  frame *stack;	 /**< A set of frames in the stack. */
} stack;
/*@}*/

/**
 * @name Thread.
 *
 * The threads belong to a group. Both the thread and the group of threads
 * are identified by an id and a group id.
 */
/** The thread object. */
typedef struct thread_t
{
  int id; /**< Id of the thread. */
  int running; /**< Is set to 1 if the thread is running. */
  int core; /**< The core where the thread is running on. */
  frame frame; /**< The current frame if the thread is not running. */

  struct thread_t *next; /**< The next thread in the group. */
} thread;

/** Structure for the thread group. */
typedef struct thread_group_t
{
  int id; /**< The group id. */
  thread *first; /**< The first thread in the group. */

  struct thread_group_t *next; /**< The next group. */
} thread_group;
/*@}*/

/**
 * @name Assembler.
 *
 * The assembler lines when the front end is in disassembly mode. A set of
 * assembler lines belongs to one source line.
 */
/*@{*/
/**
 * Structure for a single line. The assembler lines are grouped to a source
 * line.
 */
typedef struct asm_line_t
{
  int address; /**< The address of the line. */
  int offset; /**< The offset from the function start. */
  char *inst; /**< The assembler instruction. */
  int size;   /**< Size of the instruction. */

  struct asm_line_t *next; /**<
			    * The next instruction belonging to the same
			    * source line.
			    */
} asm_line;

/**
 * The source line which could have several assembler lines.
 */
typedef struct src_line_t
{
  int line_nr; /**< The source line number. */
  asm_line *lines; /**<
		    * A list of assembler line belonging to the source line.
		    */
  asm_line *last; /**< The last assembler line that we know of. */

  struct src_line_t *next; /**< The next source line. */
} src_line;

/**
 * The object of all source and assembler lines. */
typedef struct assembler_t
{
  char *function; /**< The function where the lines are located. */
  char *file;	  /**< The file for the function. */
  src_line *lines; /**< A list of source lines. */
  int address; /**< The address of the function. */
  src_line *current_line; /**< The current source line. */

  int size; /**< Size of function. */
  int file_size; /**< Size of the file name. */

  asm_line *pool; /**< A pool of unused assembler lines. */
  src_line *src_pool; /**< A pool of unused source lines. */
} assembler;
/*@}*/

/**
 * @name Registers.
 *
 * The registers @a data_reg are stored in a table @a data_registers.
 */
/*@{*/
/**
 * A structure for one register.
 */
typedef struct data_reg_t
{
  char *reg_name; /**< The register's name. */
  int type; /**<
	     * Type of register. Note: Only type 2 string is used for the
	     * moment.
	     */
  union
  {
    uint64_t u64;      /**< The unsigned 64 bit value. */
    uint64_t u128[2];  /**< The 128 bit value. */
    char *svalue;      /**< The corresponding string, e.g. 0x00. */
  };
  int changed;	/**<
		 * Not used, but 1 if the register has changed value and
		 * needs to be updated.
		 */

  int size; /**< Size of the name. */
} data_reg;

/**
 * Table of all the registers.
 */
typedef struct data_registers_t
{
  data_reg *registers; /**< The registers. */
  int len; /**< Number of registers in use. */
  int size; /**< The available size of the table. */

  int pc; /**< Pc register, not in use. */
} data_registers;
/*@}*/

/*******************************************************************************
 * Public functions
 ******************************************************************************/
breakpoint_table *bp_table_create (void);
void bp_table_free (breakpoint_table * bpt);
int bp_table_insert (breakpoint_table * bpt, breakpoint * bp);
int bp_table_remove (breakpoint_table * bpt, int number);

breakpoint *bp_create (void);
void bp_free (breakpoint * bp);

int thread_group_add (thread_group ** head, int group_id);
int thread_group_remove (thread_group ** head, int group_id);
void thread_group_remove_all (thread_group ** head);
int thread_add (thread_group * head, int group_id, int thread_id);
thread *thread_group_get_thread (thread_group * head, int group_id,
				 int thread_id);
int thread_remove (thread_group * head, int group_id, int thread_id);
void thread_remove_all (thread ** head);
int thread_set_running (thread_group * head, int group_id, int thread_id,
			int running, int core);
void thread_clear (thread * pt);

int library_add (library ** head, const char *id, const char *target,
		 const char *host, int loaded);
void library_remove (library ** head, const char *id, const char *target,
		     const char *name);
void library_remove_all (library ** head);

stack *stack_create (int depth);
void stack_free (stack * stack);
frame *stack_get_frame (stack * stack, int level);
void stack_clean_frame (stack * stack, int level);
int frame_insert_variable (frame * frame, char *name, char *type, char *value,
			   int var, int back);
void variable_delete_list (variable * var_list);

assembler *ass_create (void);
void ass_free (assembler * ass);
void ass_reset (assembler * ass);
int ass_add_line (assembler * ass, const char *file, const char *func,
		  int line_nr, int address, int offset, const char *inst);

data_registers *data_registers_create (void);
void data_registers_free (data_registers * registers);
void data_registers_add (data_registers * registers, int nr,
			 const char *name);
int data_registers_set_value (data_registers * registers, int nr,
			      uint64_t value);
int data_registers_set_str_value (data_registers * registers, int nr,
				  const char *value);
unsigned long int data_registers_get_pc (data_registers * registers);
#endif
