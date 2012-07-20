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
 * @file mi2_parser.c
 *
 * @brief Implements the mi2 parser.
 *
 * Implements the mi2 parsing. Keeps record of the debugger objects and
 * calls view functions when the view needs to be updated.
 *
 * @bug Open a file and set a breakpoint. Breakpoint is not show, even if it
 * inserted.
 *
 * @todo Use defines for more strings.
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "mi2_parser.h"
#include "lvdbg.h"
#include "configuration.h"
#include "mi2_interface.h"
#include "objects.h"
#include "view.h"
#include "misc.h"
#include "debug.h"
#include "win_form.h"

#define PARSE_ERROR "Parse error: '%s'"
#define NOT_A_NUMBER "Not a number: '%s'"

/**
 * @name Breakpoint fields
 *
 * The breakpoint fields in the mi2 messages. Both names and values for the
 * breakpoint parameters.
 */
/*@{*/
#define BKPT_NUMBER      "number"
#define BKPT_TYPE        "type"
#define BKPT_DISP        "disp"
#define BKPT_ENABLED     "enabled"
#define BKPT_ADDR        "addr"
#define BKPT_FUNC        "func"
#define BKPT_FILE        "file"
#define BKPT_FULLNAME    "fullname"
#define BKPT_LINE        "line"
#define BKPT_THREAD      "thread"
#define BKPT_TIMES       "times"
#define BKPT_COND        "cond"
#define BKPT_IGNORE      "ignore"
#define BKPT_ORIGINAL    "original-location"

#define BKPT_BREAKPOINT  "breakpoint"
#define BKPT_WATCHPOINT  "watchpoint"
#define BKPT_KEEP        "keep"
#define BKPT_DEL         "del"
/*@}*/

/**
 * @name Thread fields.
 *
 * The thread fields in the mi2 messages. Both names and values for the
 * thread parameters.
 */
/*@{*/
#define THREAD_GROUP_CREATED "group-created"
#define THREAD_CREATED       "created"
#define THREAD_GROUP_EXITED  "group-exited"
#define THREAD_EXITED        "exited"
#define THREAD_GROUP_ID      "group-id"
#define THREAD_ID            "id"
#define THREAD_RUNNING_ID    "thread-id"
#define THREAD_STOPPED       "stopped-threads"
/*@}*/

/**
 * @name Library fields.
 *
 * The library fields in the mi2 messages. Both names and values for the
 * library parameters.
 */
/*@{*/
#define LIBRARY_LOADED    "loaded"
#define LIBRARY_UNLOADED  "unloaded"
#define LIBRARY_ID        "id"
#define LIBRARY_HOST      "host-name"
#define LIBRARY_TARGET    "target-name"
#define LIBRARY_SYMBOLS   "symbols-loaded"
/*@}*/

/**
 * @name Stopped fields.
 *
 * The stopped fields in the mi2 messages. Both names and values for the
 * stopped parameters.
 */
/*@{*/
#define STOPPED_REASON        "reason"
#define STOPPED_DISP          "disp"
#define STOPPED_BKPT_NO       "bkptno"
#define STOPPED_VALUE         "value"
#define STOPPED_WPT           "wpt"
#define STOPPED_HW_WPT        "wpt"	/* According to gdb breakpoint.c */
#define STOPPED_HW_RWPT       "hw-rwpt"
#define STOPPED_HW_AWPT       "hw-awpt"
#define STOPPED_FRAME         "frame"
#define STOPPED_THREAD_ID     "thread-id"
#define REASON_BREAKPOINT     "breakpoint-hit"
#define REASON_EXIT_NORMAL    "exited-normally"
#define REASON_EXIT_SIGNALLED "exited-signalled"
/*@}*/

/**
 * @name Done fields.
 *
 * The ^done fields in the mi2 records.
 */
/*@{*/
#define DONE_STACK      "stack"
#define DONE_VARIABLES  "variables"
#define DONE_THREADS    "threads"
#define DONE_FILES      "files"
#define DONE_ASM_INSNS  "asm_insns"
#define DONE_REGISTER_NAMES "register-names"
#define DONE_CHANGED_REGISTERS "changed-registers"
#define DONE_REGISTER_VALUES "register-values"
#define DONE_VALUE "value"
/*@}*/

/**
 * @name Frame fields.
 *
 * The frame fields in the mi2 messages. Both names and values for the
 * frame parameters.
 */
/*@{*/
#define FRAME_ADDR        "addr"
#define FRAME_FUNC        "func"
#define FRAME_ARGS        "args"
#define FRAME_FILE        "file"
#define FRAME_FULLNAME    "fullname"
#define FRAME_LINE        "line"
#define FRAME_LEVEL       "level"

#define ARGS_NAME         "name"
#define ARGS_TYPE         "type"
#define ARGS_VALUE        "value"
/*@}*/

/**
 * @name Threads fields.
 *
 * The thread fields from the response from -thread-info command.
 */
/*@{*/
#define THREADS_ID "id"
#define THREADS_TARGET_ID "target-id"
#define THREADS_STATE "state"
#define THREADS_CORE "core"
/*@}*/

/*******************************************************************************
 * Internal structures and enums
 ******************************************************************************/
/** The mi2 parser structure. */
struct mi2_parser_t
{
  view *view;	    /**<
                     * The view object that need to be updated with current
                     * information.
                     */

  int is_running;   /**< Set to 1 when program is running. @todo needed? */
  int is_connected; /**< Set to 1 when debugger is connected to target. */
  int is_exit;	    /**< Set to 1 when debugger exits. */

  breakpoint_table *breakpoint_table; /**< Table of all breakpoints. */
  library *libraries;		      /**< List of loaded libraries. */
  thread_group *thread_groups;	      /**< List of thread groups and threads. */
  stack *stack;			      /**< The stack of the current thread. */
  int frame;			      /**< The current frame. */
  int thread_id;   /**< The current thread id. */

  int auto_frames;  /**<
                     * When 1 and a *stopped happens the frame stack will be
                     * fetched and shown in view. If 0 only the frame supplied
                     * in the frame parameter will be shown.
                     */
  int disassemble;  /**< 1 if we should request disassembler information. */
  assembler *ass_lines;	/**< A set of assembler lines. */

  data_registers *registers; /**< The registers. */
  int changed_regs; /**< Set to 1 if the registers has changed. */
  char *regs;	    /**< Holds which register that has changed. */
  int size_regs;    /**< Size of regs. */

  int pc;
};

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
static void mi2_parser_exit (mi2_parser * parser);
static void mi2_parser_parse_changed_registers (mi2_parser * parser,
						char *regs);
static void mi2_parser_parse_register_values (mi2_parser * parser,
					      char *values);
static void mi2_parser_parse_register_names (mi2_parser * parser,
					     char *names);
static void mi2_parser_parse_asm (mi2_parser * parser, char *asm_value);
static void mi2_parser_parse_files (mi2_parser * parser, char *files);
static int mi2_parser_parse_threads (mi2_parser * parser, char *threads);
static int mi2_parser_parse_variables (mi2_parser * parser, char *vars);
static int mi2_parser_parsestack (mi2_parser * parser, char *stack);
static int mi2_parser_parse_args (frame * frame, char *args_value,
				  int variable);
static int mi2_parser_parse_frame (mi2_parser * parser, char *frame_val,
				   frame * to_use);
static int mi2_parser_parse_watchpoint (mi2_parser * parser, char *wp,
					char *wp_value);
static int mi2_parser_parse_bkpt (mi2_parser * parser, char *line);
static int mi2_parser_parse_done (mi2_parser * parser, char *line);
static int mi2_parser_parse_error (mi2_parser * parser, char *line);
static int mi2_parser_parse_running (mi2_parser * parser, char *line);
static int mi2_parser_parse_stopped (mi2_parser * parser, char *line);
static int mi2_parser_parse_thread (mi2_parser * parser, char *line);
static int mi2_parser_parse_library (mi2_parser * parser, char *line);

/**
 * @brief Do cleanup adter program exit.
 *
 * Clear stack and frame window.
 *
 * @param parser The parser.
 */
static void
mi2_parser_exit (mi2_parser * parser)
{
  assert (parser);

  DINFO (1, "Exiting program");

  stack_clean_frame (parser->stack, -1);
  view_update_stack (parser->view, parser->stack);
  view_update_frame (parser->view, parser->stack, -1);
}

/**
 * @brief Parse change registers.
 *
 * Parse the change-registers, which has the form:
 *
 * @code
 * changed-registers=["0","1",...]
 * @endcode
 *
 * @param parser The parser.
 * @param regs The changed registers.
 */
static void
mi2_parser_parse_changed_registers (mi2_parser * parser, char *regs)
{
  char *s;
  char *e;
  int nr;
  char buf[512];
  char *p = buf;
  char *t;
  int size = 512;
  int len;

  len = 0;
  nr = 0;
  s = regs;
  p[0] = '\0';
  while (*s != '\0')
    {
      while (*s && *s != '\"')
	{
	  s++;
	}
      if (*s == '\0')
	{
	  break;
	}
      s++;
      e = s + 1;
      while (*e && *e != '\"')
	{
	  e++;
	}
      if (*e == '\0')
	{
	  break;
	}
      *e = '\0';
      if (len + strlen (s) + 2 >= size)
	{
	  size = len + strlen (s) + 64;
	  t = (char *) realloc (p, size);
	  LOG_ERR_IF_FATAL (t == NULL, ERR_MSG_CREATE ("regs"));
	  p = t;
	}
      strcat (p, " ");
      strcat (p, s);
      s = e + 1;
    }
  if (strlen (p) != 0)
    {
      LPRINT (parser->regs, -1, parser->size_regs, "%s", p);
      parser->changed_regs = 1;
    }
  else
    {
      parser->changed_regs = 0;
    }
  if (p != buf)
    {
      free (p);
    }
}

/**
 * @brief Parse register values.
 *
 * Parse the register values, which has the form:
 *
 * @code
 * register-values=[{number="0",value="VALUE"}[,...]]
 *
 * value : "0x123" | "{v4_float={0x0,0x0,0x0,0x0}, v2_double= {0x0 0x0}...}"
 * @endcode
 *
 * @param parser The parser.
 * @param values The values.
 */
static void
mi2_parser_parse_register_values (mi2_parser * parser, char *values)
{
  char *next;
  char *name;
  char *value;
  char *inext;
  char *iname;
  char *ivalue;
  char *endptr;
  int nr = -1;
  int ret;
  char *p;
  char *q;

  next = values;
  while (next && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret != '{' || value == NULL)
	{
	  LOG_ERR (PARSE_ERROR, next);
	  return;
	}
      inext = value;
      while (inext && *inext != '\0')
	{
	  ret = get_next_param (inext, &iname, &ivalue, &inext);
	  LOG_ERR_IF_RETURN (ret < 0 || ivalue == NULL || iname == NULL,,
			     PARSE_ERROR, inext);

	  if (strcmp (iname, "number") == 0)
	    {
	      nr = strtol (ivalue, &endptr, 0);
	      LOG_ERR_IF_RETURN (endptr == ivalue,, NOT_A_NUMBER, ivalue);
	    }
	  else if (strcmp (iname, "value") == 0)
	    {
	      if (ivalue[0] == '{')
		{
		  p = ivalue + 1;
		  while (*p && *p != '{')
		    {
		      p++;
		    }
		  LOG_ERR_IF_RETURN (*p == '\0',, PARSE_ERROR, ivalue);
		  p++;
		  q = p;
		  while (*q && *q != '}')
		    {
		      q++;
		    }
		  LOG_ERR_IF_RETURN (*q == '\0',, PARSE_ERROR, ivalue);
		  *q = '\0';
		}
	      else
		{
		  p = ivalue;
		}
	      ret = data_registers_set_str_value (parser->registers, nr, p);
	      LOG_ERR_IF_RETURN (ret < 0,, PARSE_ERROR, ivalue);
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, name);
	      return;
	    }
	}
    }
  parser->changed_regs = 0;
  view_update_registers (parser->view, parser->registers);
}

/**
 * @brief Parse register names.
 *
 * Parse the ^done,register-names which has the form:
 *
 * @code
 * register-names=["Name 1","Name 2",...]
 * @endcode
 *
 * @param parser The parser.
 * @param names The names.
 */
static void
mi2_parser_parse_register_names (mi2_parser * parser, char *names)
{
  char *s;
  char *e;
  int nr;

  nr = 0;
  s = names;
  while (*s != '\0')
    {
      while (*s && *s != '\"')
	{
	  s++;
	}
      if (*s == '\0')
	{
	  break;
	}
      s++;
      e = s + 1;
      while (*e && *e != '\"')
	{
	  e++;
	}
      if (*e == '\0')
	{
	  LOG_ERR (PARSE_ERROR, s);
	  break;
	}
      *e = '\0';
      DINFO (3, "Added reg %d '%s'", nr, s);
      data_registers_add (parser->registers, nr, s);
      nr++;
      s = e + 1;
    }
}

/**
 * @brief Parse asm line.
 *
 * Parse asm line which has the form:
 *
 * @code
 * [{address="ADDR",func-name="NAME",offset="NR", inst="INST"},{addr...}]
 * @endcode
 *
 * @param parser The parser.
 * @param file The file belonging to asm line.
 * @param line_nr The line number.
 * @param line The asm line.
 *
 * @return 1 if the line belongs to a new function.
 */
static int
mi2_parser_parse_asm_line (mi2_parser * parser, const char *file, int line_nr,
			   char *line)
{
  char *next;
  char *name;
  char *value;
  char *inext;
  char *iname;
  char *ivalue;
  int address = -1;
  int offset = -1;
  char *func = NULL;
  char *inst = NULL;
  char *endptr;
  int ret;
  int new_func = 0;


  if (line == NULL || strlen (line) == 0)
    {
      ret = ass_add_line (parser->ass_lines, file, NULL, line_nr, -1,
			  -1, NULL);
      return ret;
    }
  next = line;
  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret != '{' || name != NULL || value == NULL,
			 -1, PARSE_ERROR, next);
      address = -1;
      offset = -1;
      inext = value;
      while (inext && *inext)
	{
	  ret = get_next_param (inext, &iname, &ivalue, &inext);
	  LOG_ERR_IF_RETURN (ret < 0 || iname == NULL || ivalue == NULL,
			     -1, PARSE_ERROR, inext);

	  if (strcmp (iname, "address") == 0)
	    {
	      address = strtol (ivalue, &endptr, 0);
	      LOG_ERR_IF_RETURN (endptr == ivalue, -1, NOT_A_NUMBER, ivalue);
	    }
	  else if (strcmp (iname, "offset") == 0)
	    {
	      offset = strtol (ivalue, &endptr, 0);
	      LOG_ERR_IF_RETURN (endptr == ivalue, -1, NOT_A_NUMBER, ivalue);
	    }
	  else if (strcmp (iname, "func-name") == 0)
	    {
	      func = ivalue;
	    }
	  else if (strcmp (iname, "inst") == 0)
	    {
	      inst = ivalue;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, iname);
	      return -1;
	    }
	}
      LOG_ERR_IF_RETURN (address < 0 || offset < 0, -1, PARSE_ERROR, line);
      ret = ass_add_line (parser->ass_lines, file, func, line_nr, address,
			  offset, inst);
      if (ret == 1)
	{
	  new_func = 1;
	}
    }

  return new_func;
}

/**
 * @brief Parse '^done,asm_insns' field.
 *
 * Parse the '^done,asm_insns' field which has the form:
 *
 * @code
 * asm_insns=[src_and_asm_line={line="NR",file="NAME",
 * line_asm_insn=[{address="ADDR",func-name="NAME",offset="NR", inst="INST"},
 * {addr...}]},src_and_asm_line=...]
 * @endcode
 *
 * @param parser The parser.
 * @param asm_value The instructions and source lines.
 */
static void
mi2_parser_parse_asm (mi2_parser * parser, char *asm_value)
{
  char *next;
  char *name;
  char *value;
  char *endptr;
  char *inext;
  char *iname;
  char *ivalue;
  int address;
  int offset;
  char *func_name;
  char *inst;
  char *file;
  int line_nr;
  int ret;
  int new_function = 0;

  /* Reset the assemble lines */
  ass_reset (parser->ass_lines);

  next = asm_value;
  while (next && *next)
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret != '{' || value == NULL || name == NULL
	  || strcmp (name, "src_and_asm_line") != 0)
	{
	  LOG_ERR (PARSE_ERROR, next);
	  goto error;
	}
      address = -1;
      offset = -1;
      line_nr = -1;
      file = NULL;
      func_name = NULL;
      inst = NULL;
      inext = value;
      while (inext && *inext)
	{
	  ret = get_next_param (inext, &iname, &ivalue, &inext);
	  if (ret < 0 || iname == NULL || ivalue == NULL)
	    {
	      LOG_ERR (PARSE_ERROR, inext);
	      goto error;
	    }
	  if (strcmp (iname, "line") == 0)
	    {
	      line_nr = strtol (ivalue, &endptr, 0);
	      LOG_ERR_IF_RETURN (endptr == ivalue,, NOT_A_NUMBER, ivalue);
	    }
	  else if (strcmp (iname, "file") == 0)
	    {
	      file = ivalue;
	    }
	  else if (strcmp (iname, "line_asm_insn") == 0)
	    {
	      LOG_ERR_IF_RETURN (file == NULL || line_nr < 0,,
				 PARSE_ERROR, value);
	      ret = mi2_parser_parse_asm_line (parser, file, line_nr, ivalue);
	      LOG_ERR_IF_RETURN (ret < 0,, PARSE_ERROR, ivalue);
	      if (ret == 1)
		{
		  new_function = 1;
		}
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, iname);
	      goto error;
	    }
	}
    }
  view_update_ass (parser->view, parser->ass_lines, parser->pc);
error:
  ;
}

/**
 * @brief Parse done files field.
 *
 * Parse the '^done,files' field. The field has the form:
 *
 * @code
 * files=[{file='FILENAME'[,fullname='FULLNAME']},{file=...}]
 * @endcode
 *
 * @param parser The parser.
 * @param files The field containing the files.
 */
static void
mi2_parser_parse_files (mi2_parser * parser, char *files)
{
  char *next;
  char *name;
  char *value;
  char *inext;
  char *iname;
  char *ivalue;
  char *buf[64];
  char **items = buf;
  int size = 64;
  int len = 0;
  int ret;
  int i;

  next = files;
  while (next && *next)
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret != '{' || value == NULL)
	{
	  LOG_ERR (PARSE_ERROR, next);
	  goto error;
	}
      inext = value;
      while (inext && *inext)
	{
	  ret = get_next_param (inext, &iname, &ivalue, &inext);
	  if (ret < 0 || iname == NULL || ivalue == NULL)
	    {
	      LOG_ERR (PARSE_ERROR, inext);
	      goto error;
	    }
	  if (strcmp (iname, "fullname") == 0)
	    {
	      items[len] = ivalue;
	      len++;
	      if (len >= size - 1)
		{
		  if (items == buf)
		    {
		      items = (char **) malloc ((len + 10) * sizeof (char *));
		      LOG_ERR_IF_FATAL (items == NULL, "Memory");
		      for (i = 0; i < size; i++)
			{
			  items[i] = buf[i];
			}
		      memset (&items[size], 0,
			      (len + 10 - size) * sizeof (char *));
		      size = len + 10;
		    }
		  else
		    {
		      items = (char **) realloc (items,
						 (len +
						  10) * sizeof (char *));
		      LOG_ERR_IF_FATAL (items == NULL, "Memory");
		      memset (&items[size], 0,
			      (len + 10 - size) * sizeof (char *));
		      size = len + 10;
		    }
		}

	    }
	  else if (strcmp (iname, "file") != 0)
	    {
	      LOG_ERR (PARSE_ERROR, iname);
	      goto error;
	    }
	}
    }
  if (len == 0)
    {
      return;
    }
  items[len] = NULL;
  ret = form_selection (items, _("Select a file"));
  if (ret == -2)
    {
      VLOG_ERR (parser->view, _("Could not retrieve file"));
    }
  else if (ret >= 0)
    {
      view_show_file (parser->view, items[ret], 0, 1);
    }

error:
  if (items != buf)
    {
      free (items);
    }
}

/**
 * @brief Parse the threads response.
 *
 * Parse the threads response from a -thread-list command. The response has the
 * form:
 *
 * @code
 * {id="3",target-id="Thread 0xb77e5b70 (LWP 30525)",
 * frame={level="0"...,line="111"},state="stopped",core="1"},{id="2"...
 * @endcode
 *
 * The frame field has the same form as at other frame fields.
 *
 * @param parser The parser.
 * @param threads The thread fiekd value.
 *
 * @return 0 upon success. -1 if failed to parse the threads.
 */
static int
mi2_parser_parse_threads (mi2_parser * parser, char *threads)
{
  int ret;
  char *name;
  char *value;
  char *next;
  char *endptr;
  char *inner_name;
  char *inner_value;
  char *inner_next;
  thread *pt = NULL;
  int thread_id = -1;
  int group_id;
  int addr;
  int core;
  int running;

  next = threads;

  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret < 0 || value == NULL, -1, PARSE_ERROR, threads);

      if (ret == '{' && name == NULL)
	{
	  /* Surrounding the real thread info. */
	  inner_next = value;
	  while (inner_next != NULL && *inner_next != '\0')
	    {
	      ret = get_next_param (inner_next, &inner_name, &inner_value,
				    &inner_next);
	      LOG_ERR_IF_RETURN (ret < 0 || inner_value == NULL
				 || inner_name == NULL, -1, PARSE_ERROR,
				 threads);

	      if (strcmp (inner_name, THREADS_ID) == 0)
		{
		  thread_id = strtol (inner_value, &endptr, 0);
		  LOG_ERR_IF_RETURN (endptr == inner_value, -1, NOT_A_NUMBER,
				     inner_value);
		}
	      else if (strcmp (inner_name, THREADS_TARGET_ID) == 0)
		{
		  ret = sscanf (inner_value, "Thread %X (LWP %d)", &addr,
				&group_id);
		  if (ret != 2)
		    {
		      ret = sscanf (inner_value, "process %d", &group_id);
		      LOG_ERR_IF_RETURN (ret != 1, -1, PARSE_ERROR, value);
		    }
		  pt = thread_group_get_thread (parser->thread_groups, -1,
						thread_id);
		  LOG_ERR_IF_RETURN (pt == NULL, -1,
				     "Could not find thread %d in group %d",
				     thread_id, group_id);
		  thread_clear (pt);
		}
	      else if (strcmp (inner_name, THREADS_STATE) == 0)
		{
		  LOG_ERR_IF_RETURN (pt == NULL, -1, "No frame");
		  if (strcmp (inner_value, "running") == 0)
		    {
		      running = 1;
		    }
		  else if (strcmp (inner_value, "stopped") == 0)
		    {
		      running = 0;
		    }
		  else
		    {
		      LOG_ERR ("Unknown state '%s'", inner_value);
		      return -1;
		    }
		  pt->running = running;
		}
	      else if (strcmp (inner_name, THREADS_CORE) == 0)
		{
		  LOG_ERR_IF_RETURN (pt == NULL, -1, "No frame");
		  core = strtol (inner_value, &endptr, 0);
		  LOG_ERR_IF_RETURN (endptr == inner_value, -1, NOT_A_NUMBER,
				     inner_value);
		  pt->core = core;
		}
	      else if (strcmp (inner_name, "frame") == 0)
		{
		  LOG_ERR_IF_RETURN (pt == NULL, -1, "No frame");
		  ret = mi2_parser_parse_frame (parser, inner_value,
						&pt->frame);
		  LOG_ERR_IF_RETURN (ret < 0, -1, PARSE_ERROR, inner_value);
		}
	      else
		{
		  LOG_ERR (PARSE_ERROR, inner_name);
		  return -1;
		}
	    }			/* inner while */
	}			/* type = '{' */
      else
	{
	  LOG_ERR (PARSE_ERROR, name);
	  return -1;
	}
      pt = NULL;
    }
  return 0;
}

/**
 * @brief Parse the variables response from the debugger.
 *
 * Parse the variable response from the debugger. The response has the form:
 *
 * @code
 * [{name="argc",arg="1",type="int",value="1"},{name="argv",arg="1",
 * type="char **",value="0xbffff444"},{name="p",type="int *",value="0x0"}]
 * @endcode
 *
 * For arguments to the function of the frame, there is the field 'arg="1"',
 * which indicates the variable is an argument. For local variables, 'arg' is
 * absent.
 *
 * @param parser The mi2 parser.
 * @param vars The variables value.
 *
 * @return 0 upon success, otherwise -1.
 */
static int
mi2_parser_parse_variables (mi2_parser * parser, char *vars)
{
  int ret;
  char *name;
  char *value;
  char *next;
  char *inner_name;
  char *inner_value;
  char *inner_next;
  char *vname = NULL;
  char *vtype = NULL;
  char *vval = NULL;
  int arg;

  assert (parser);
  assert (parser->frame >= 0);

  next = vars;
  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret != '{' || value == 0, -1, PARSE_ERROR, vars);
      inner_next = value;
      arg = 0;
      vname = NULL;
      vtype = NULL;
      vval = NULL;
      while (inner_next != NULL && *inner_next != '\0')
	{
	  ret =
	    get_next_param (inner_next, &inner_name, &inner_value,
			    &inner_next);
	  LOG_ERR_IF_RETURN (ret < 0 || inner_name == NULL
			     || inner_value == NULL, -1, PARSE_ERROR, value);
	  if (strcmp (inner_name, "name") == 0)
	    {
	      vname = inner_value;
	    }
	  else if (strcmp (inner_name, "type") == 0)
	    {
	      vtype = inner_value;
	    }
	  else if (strcmp (inner_name, "value") == 0)
	    {
	      vval = inner_value;
	    }
	  else if (strcmp (inner_name, "arg") == 0)
	    {
	      if (strcmp (inner_value, "1") == 0)
		{
		  arg = 1;
		}
	      else
		{
		  arg = 0;
		}
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, inner_name);
	      return -1;
	    }
	}
      ret =
	frame_insert_variable (stack_get_frame (parser->stack, parser->frame),
			       vname, vtype, vval, !arg, 0);
      LOG_ERR_IF_RETURN (ret < 0, -1, "Could not insert variable");
    }

  return 0;
}

/**
 * @brief Parse the stack response.
 *
 * Parse the stack response from the mi2 debugger. The response has the form:
 * @code
 * frame={level="0",addr="0x080484ba",func="bar",file="example_1.c",
 * fullname="/h/example_1.c",line="9"},frame={level="1",addr="0x08048500",
 * func="main",file="example_1.c",fullname="/h/examples/example_1.c",line="27"}
 * @endcode
 *
 * @param parser The mi2 parser.
 * @param stack The stack value.
 *
 * @return 0 upon success, otherwise -1.
 */
static int
mi2_parser_parsestack (mi2_parser * parser, char *stack)
{
  int ret;
  char *name;
  char *value;
  char *next;

  assert (parser);
  assert (stack);

  /* Delete to old stack. */
  if (parser->stack != NULL)
    {
      stack_clean_frame (parser->stack, -1);
    }
  parser->stack->depth = -1;

  next = stack;
  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret < 0 || value == NULL || name == NULL
			 || strcmp (name, STOPPED_FRAME) != 0, -1,
			 PARSE_ERROR, next);
      ret = mi2_parser_parse_frame (parser, value, NULL);
      LOG_ERR_IF_RETURN (ret < 0, -1, PARSE_ERROR, value);
    }

  view_update_stack (parser->view, parser->stack);

  return 0;
}

/**
 * @brief Parse the argument field value.
 *
 * Parses the arguments field value from e.g. frame information. Args has the
 * form of
 *
 * @code
 * args=[{name="foo",type="int",value="2"}, {name="bar",type="int",value="0"}]
 * @endcode
 *
 * Depending on the command sent by user, type or value might not be included.
 * The function expect that the "args=" and the '[' and ']' are not included,
 * only the value, e.g. only "{name= ...  value="0"}"
 *
 * @param frame The frame where the arguments belong to.
 * @param args_value The value of the args parameter.
 * @param variable If 1 the args value belongs to variables in the function. If
 *                 0 they belongs to the functions arguments.
 *
 * @return 0 on success. On failure -1 is returned.
 */
static int
mi2_parser_parse_args (frame * frame, char *args_value, int variable)
{
  char *name;
  char *value;
  char *next;
  char *inner_name;
  char *inner_value;
  char *inner_next;
  char *var_name = NULL;
  char *var_type = NULL;
  char *var_value = NULL;
  int ret;

  assert (frame);
  assert (args_value);

  next = args_value;
  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret < 0 || value == NULL, -1, PARSE_ERROR, next);
      if (ret == '{')
	{
	  inner_next = value;
	  while (inner_next != NULL && *inner_next != '\0')
	    {
	      ret = get_next_param (inner_next, &inner_name, &inner_value,
				    &inner_next);
	      LOG_ERR_IF_RETURN (ret < 0 || inner_name == NULL
				 || inner_value == NULL, -1,
				 PARSE_ERROR, inner_next);
	      if (strcmp (inner_name, ARGS_NAME) == 0)
		{
		  var_name = inner_value;
		}
	      else if (strcmp (inner_name, ARGS_TYPE) == 0)
		{
		  var_type = inner_value;
		}
	      else if (strcmp (inner_name, ARGS_VALUE) == 0)
		{
		  var_value = inner_value;
		}
	      else
		{
		  LOG_ERR ("Could not parse '%s'", inner_name);
		  return -1;
		}
	    }

	  LOG_ERR_IF_RETURN (var_name == NULL, -1,
			     "Could not retrieve variable name");
	  ret = frame_insert_variable (frame, var_name, var_type, var_value,
				       variable, 1);
	  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not add variable");

	  var_name = NULL;
	  var_type = NULL;
	  var_value = NULL;
	}
      else
	{
	  LOG_ERR (PARSE_ERROR, name ? name : "[NaN]");
	  return -1;
	}
    }
  return 0;
}

/**
 * @brief Parse a frame field.
 *
 * Parse a frame field. The frame field has the form:
 *
 * @code
 * frame={[level="0",]addr="0x00",func="foo",file="bar.c",fullname="xxx",
 *        line="9"}
 * @endcode
 *
 * Only the value is expected in the @a frame_value string. We get the frame
 * field from *stopped, ^done,threads and ^done,stack. Depending on where
 * the value comes from we know which frame the strings belongs to. In
 * the stack case we actual do not know where the frame belongs to until we
 * read the 'level' field, so in this case we do not supply a value in
 * @a to_use.
 *
 * @param parser The parser object.
 * @param frame_value The value of the frame field.
 * @param to_use When calling from "*stopped" and "*done,threads" the frame is
 *               known and provided. When the caller is from -stack-list-frames
 *               we do not now which frame it belongs to until we see the level
 *               so @a to_use is set to NULL in this case.
 *
 * @return 0 on success, otherwise -1.
 */
static int
mi2_parser_parse_frame (mi2_parser * parser, char *frame_value,
			frame * to_use)
{
  char *name;
  char *value;
  char *next;
  char *endptr;
  int ret;
  int level;
  frame *pframe;

  assert (parser);
  assert (frame_value);

  pframe = to_use;

  /* Parse the frame. */
  next = frame_value;
  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret < 0 || name == NULL || value == NULL)
	{
	  LOG_ERR (PARSE_ERROR, frame_value);
	  goto error;
	}
      if (strcmp (name, FRAME_ADDR) == 0)
	{
	  LOG_ERR_IF_RETURN (pframe == NULL, -1, "No level yet");
	  pframe->addr = strtol (value, &endptr, 0);
	  if (value == endptr)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      goto error;
	    }
	}
      else if (strcmp (name, FRAME_FUNC) == 0)
	{
	  LOG_ERR_IF_RETURN (pframe == NULL, -1, "No level yet");
	  if (pframe->func)
	    {
	      goto duplicate;
	    }
	  pframe->func = strdup (value);
	  LOG_ERR_IF_FATAL (pframe->func == NULL, ERR_MSG_CREATE ("string"));
	}
      else if (strcmp (name, FRAME_ARGS) == 0)
	{
	  LOG_ERR_IF_RETURN (pframe == NULL, -1, "No level yet");
	  ret = mi2_parser_parse_args (pframe, value, 0);
	  if (ret < 0)
	    {
	      goto error;
	    }
	}
      else if (strcmp (name, FRAME_FILE) == 0)
	{
	  LOG_ERR_IF_RETURN (pframe == NULL, -1, "No level yet");
	  if (pframe->file)
	    {
	      goto duplicate;
	    }
	  pframe->file = strdup (value);
	  LOG_ERR_IF_FATAL (pframe->file == NULL, ERR_MSG_CREATE ("string"));
	}
      else if (strcmp (name, FRAME_FULLNAME) == 0)
	{
	  LOG_ERR_IF_RETURN (pframe == NULL, -1, "No level yet");
	  if (pframe->fullname)
	    {
	      goto duplicate;
	    }
	  pframe->fullname = strdup (value);
	  LOG_ERR_IF_FATAL (pframe->fullname == NULL,
			    ERR_MSG_CREATE ("string"));
	}
      else if (strcmp (name, FRAME_LINE) == 0)
	{
	  LOG_ERR_IF_RETURN (pframe == NULL, -1, "No level yet");
	  pframe->line = strtol (value, &endptr, 0);
	  if (value == endptr)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      goto error;
	    }
	}
      else if (strcmp (name, FRAME_LEVEL) == 0)
	{
	  level = strtol (value, &endptr, 0);
	  if (value == endptr)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      goto error;
	    }
	  if (pframe == NULL)
	    {
	      /* Now we know the level so we can pick the right frame to
	       * update.
	       */
	      pframe = stack_get_frame (parser->stack, level);
	    }
	  LOG_ERR_IF_RETURN (pframe == NULL, -1, "Could not get frame of "
			     " level %d", level);
	  if (level + 1 > parser->stack->depth)
	    {
	      /* Update the depth of the stack. */
	      parser->stack->depth = level + 1;
	    }
	}
      else
	{
	  LOG_ERR (PARSE_ERROR, name);
	  goto error;
	}
    }

  return 0;

duplicate:
  LOG_ERR ("Duplicate parameter '%s'", name);
error:
  return -1;
}

static int
mi2_parser_parse_watchpoint (mi2_parser * parser, char *wp, char *wp_value)
{
  int ret;
  char *next;
  char *name;
  char *value;
  char *endptr;
  char *exp = NULL;
  int number = -1;
  char *new_value = NULL;
  char *old_value = NULL;
  breakpoint *bp;

  assert (parser);

  /* Retrieve Watchpoint number & expression. */
  next = wp;
  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret < 0 || name == NULL || value == NULL, -1,
			 PARSE_ERROR, wp);
      if (strcmp (name, "number") == 0)
	{
	  number = strtol (value, &endptr, 0);
	  LOG_ERR_IF_RETURN (endptr == value, -1, NOT_A_NUMBER, value);
	}
      else if (strcmp (name, "exp") == 0)
	{
	  exp = value;
	}
      else
	{
	  LOG_ERR (PARSE_ERROR, name);
	  return -1;
	}
    }

  LOG_ERR_IF_RETURN (exp == NULL || number == -1, -1,
		     "Could not find number and expression '%s'", wp);

  if (wp_value == NULL)
    {
      DINFO (3, "Creating wp nr %d exp '%s'", number, exp);

      /* When setting a wp we only get the 'wpt' part. */
      bp = bp_create ();
      bp->number = number;
      bp->type = BP_TYPE_WATCHPOINT;
      bp->expression = strdup (exp);
      LOG_ERR_IF_FATAL (bp->expression == NULL,
			ERR_MSG_CREATE ("watchpoint"));
      ret = bp_table_insert (parser->breakpoint_table, bp);
      if (ret < 0)
	{
	  LOG_ERR ("Failed to add watchpoint nr %d", bp->number);
	  bp_free (bp);
	}
      return ret;
    }

  /* Restrieve new value */
  next = wp_value;
  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret < 0 || name == NULL || value == NULL, -1,
			 PARSE_ERROR, wp);
      if (strcmp (name, "new") == 0)
	{
	  new_value = value;
	}
      else if (strcmp (name, "old") == 0)
	{
	  old_value = value;
	}
      else
	{
	  LOG_ERR (PARSE_ERROR, name);
	  return -1;
	}
    }

  DINFO (3, "WP %d exp %s old %s new %s", number, exp,
	 new_value ? new_value : "NaN", old_value ? old_value : "NaN");

  LOG_ERR_IF_RETURN (new_value == NULL, -1, "No new value '%s'", wp_value);

  view_add_message (parser->view, 0,
		    _("Watchpoint number %d hit: %s = %s %s%s%s"),
		    number, exp, new_value,
		    old_value ? "[ " : "",
		    old_value ? old_value : "", old_value ? " ]" : "");

  LOG_ERR_IF_RETURN (parser->breakpoint_table == NULL, -1,
		     "No valid watchpoint");
  bp = parser->breakpoint_table->breakpoints[number];
  LOG_ERR_IF_RETURN (bp == NULL || strcmp (bp->expression, exp) != 0, -1,
		     "No valid watchpoint");

  if (bp->value)
    {
      free (bp->value);
    }
  bp->value = strdup (new_value);
  LOG_ERR_IF_FATAL (bp->value == NULL, ERR_MSG_CREATE ("string"));

  return 0;
}

/**
 * @brief Parse the bkpt field.
 *
 * Parse the bkpt field from ^done. Insert the new break point. Format from
 * gfb info:
 *
 * @code
 * ^done,bkpt={number="NUMBER",type="TYPE",disp="del"|"keep",
 * enabled="y"|"n",addr="HEX",func="FUNCNAME",file="FILENAME",
 * fullname="FULL_FILENAME",line="LINENO",[thread="THREADNO,]
 * times="TIMES"}
 * @endcode
 *
 * Alos added cond="CONDITION" and ignore"IGNORE COUNT" fields.
 *
 * @param parser The parser.
 * @param line The text containing the bkpt.
 *
 * @return 0 on success and -1 if we failed to create a new break point.
 */
static int
mi2_parser_parse_bkpt (mi2_parser * parser, char *line)
{
  breakpoint *bp;
  char *name;
  char *value;
  char *next;
  int ret;
  char *endptr;

  assert (line);

  bp = bp_create ();

  next = line;
  while (next != NULL && *next)
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret < 0)
	{
	  goto error;
	}
      if (name == NULL)
	{
	  LOG_ERR (PARSE_ERROR, "");
	  goto error;
	}
      if (value == NULL)
	{
	  LOG_ERR (PARSE_ERROR, name);
	  goto error;
	}
      if (strcmp (name, BKPT_NUMBER) == 0)
	{
	  bp->number = strtol (value, &endptr, 0);
	  if (endptr == value)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      goto error;
	    }
	}
      else if (strcmp (name, BKPT_TYPE) == 0)
	{
	  if (strcmp (value, BKPT_BREAKPOINT) == 0)
	    {
	      bp->type = BP_TYPE_BREAKPOINT;
	    }
	  else if (strcmp (value, BKPT_WATCHPOINT) == 0)
	    {
	      bp->type = BP_TYPE_WATCHPOINT;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, value);
	      goto error;
	    }
	}
      else if (strcmp (name, BKPT_DISP) == 0)
	{
	  if (strcmp (value, BKPT_KEEP) == 0)
	    {
	      bp->disp = 1;
	    }
	  else if (strcmp (value, BKPT_DEL) == 0)
	    {
	      bp->disp = 0;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, value);
	      goto error;
	    }
	}
      else if (strcmp (name, BKPT_ENABLED) == 0)
	{
	  if (strcmp (value, "y") == 0)
	    {
	      bp->enabled = 1;
	    }
	  else if (strcmp (value, "n") == 0)
	    {
	      bp->enabled = 0;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, value);
	      goto error;
	    }
	}
      else if (strcmp (name, BKPT_ADDR) == 0)
	{
	  bp->addr = strtol (value, &endptr, 0);
	  if (endptr == value)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      goto error;
	    }
	}
      else if (strcmp (name, BKPT_FUNC) == 0)
	{
	  bp->func = strdup (value);
	  LOG_ERR_IF_FATAL (bp->func == NULL, ERR_MSG_CREATE ("string"));
	}
      else if (strcmp (name, BKPT_FILE) == 0)
	{
	  bp->file = strdup (value);
	  LOG_ERR_IF_FATAL (bp->file == NULL, ERR_MSG_CREATE ("string"));
	}
      else if (strcmp (name, BKPT_FULLNAME) == 0)
	{
	  bp->fullname = strdup (value);
	  LOG_ERR_IF_FATAL (bp->fullname == NULL, ERR_MSG_CREATE ("string"));
	}
      else if (strcmp (name, BKPT_LINE) == 0)
	{
	  bp->line = strtol (value, &endptr, 0);
	  if (endptr == value)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      goto error;
	    }
	}
      else if (strcmp (name, BKPT_TIMES) == 0)
	{
	  bp->times = strtol (value, &endptr, 0);
	  if (endptr == value)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      goto error;
	    }
	}
      else if (strcmp (name, BKPT_ORIGINAL) == 0)
	{
	  bp->original_location = strdup (value);
	}
      else if (strcmp (name, BKPT_THREAD) == 0)
	{
	  bp->thread = strtol (value, &endptr, 0);
	  if (endptr == value)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      /**< @todo Something else? goto error;? */
	      bp->thread = -1;
	    }
	}
      else if (strcmp (name, BKPT_IGNORE) == 0)
	{
	  bp->ignore = strtol (value, &endptr, 0);
	  if (endptr == value)
	    {
	      LOG_ERR (NOT_A_NUMBER, value);
	      goto error;
	    }
	}
      else if (strcmp (name, BKPT_COND) == 0)
	{
	  bp->cond = strdup (value);
	}
      else
	{
	  LOG_ERR ("Unknown parameter '%s' = '%s'", name, value);
	  goto error;
	}
    }

  /* Insert new breakpoint in table. */
  ret = bp_table_insert (parser->breakpoint_table, bp);
  if (ret < 0)
    {
      bp_free (bp);
    }

  /* Update the view. */
  view_update_breakpoints (parser->view, parser->breakpoint_table);

  return ret;

error:
  LOG_ERR (PARSE_ERROR, line);
  bp_free (bp);

  return -1;
}

/**
 * @brief Parse the ^done message.
 *
 * Parse the '^done' message. The message has the form:
 *
 * @code
 * ^done[,RESULT]
 * @endcode
 *
 * \a Result depends on the command sent to the debugger. Depending on
 * \a Result approperate functions are called.
 *
 * @param parser The parser.
 * @param line The done line. After the "^done".
 *
 * @return 0 if the message was parsed and -1 on failure.
 */
static int
mi2_parser_parse_done (mi2_parser * parser, char *line)
{
  int ret = 0;
  char *name;
  char *value;
  char *next;

  if (*line == '\0')
    {
      return 0;
    }

  next = line;
  while (next != NULL && *next != '\0' && ret == 0)
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret < 0)
	{
	  goto error;
	}
      DINFO (3, "Name '%s' value '%s'", name ? name : "", value ? value : "");
      if (name && strcmp (name, "bkpt") == 0)
	{
	  ret = mi2_parser_parse_bkpt (parser, value);
	}
      else if (name && strcmp (name, DONE_STACK) == 0)
	{
	  ret = mi2_parser_parsestack (parser, value);
	}
      else if (name && strcmp (name, DONE_VARIABLES) == 0)
	{
	  ret = mi2_parser_parse_variables (parser, value);
	  if (ret == 0)
	    {
	      view_update_frame (parser->view, parser->stack, parser->frame);
	    }
	}
      else if (name && (strcmp (name, STOPPED_WPT) == 0
			|| strcmp (name, STOPPED_HW_WPT) == 0
			|| strcmp (name, STOPPED_HW_RWPT) == 0
			|| strcmp (name, STOPPED_HW_AWPT) == 0))
	{
	  ret = mi2_parser_parse_watchpoint (parser, value, NULL);
	  if (ret == 0)
	    {
	      view_update_breakpoints (parser->view,
				       parser->breakpoint_table);
	    }
	}
      else if (name && strcmp (name, STOPPED_FRAME) == 0)
	{
	  stack_clean_frame (parser->stack, -1);
	  ret = mi2_parser_parse_frame (parser, value, NULL);
	  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not parse the frame");
	  parser->stack->depth = 1;
	  view_update_frame (parser->view, parser->stack, 0);
	  view_show_file (parser->view, parser->stack->stack[0].fullname,
			  parser->stack->stack[0].line, 1);

	}
      else if (name && strcmp (name, DONE_THREADS) == 0)
	{
	  ret = mi2_parser_parse_threads (parser, value);
	  if (ret == 0)
	    {
	      view_update_threads (parser->view, parser->thread_groups);
	    }
	}
      else if (name && strcmp (name, DONE_FILES) == 0)
	{
	  mi2_parser_parse_files (parser, value);
	  ret = 0;
	}
      else if (name && strcmp (name, DONE_ASM_INSNS) == 0)
	{
	  mi2_parser_parse_asm (parser, value);
	  ret = 0;
	}
      else if (name && strcmp (name, DONE_REGISTER_NAMES) == 0)
	{
	  mi2_parser_parse_register_names (parser, value);
	  ret = 0;
	}
      else if (name && strcmp (name, DONE_CHANGED_REGISTERS) == 0)
	{
	  mi2_parser_parse_changed_registers (parser, value);
	  ret = 0;
	}
      else if (name && strcmp (name, DONE_REGISTER_VALUES) == 0)
	{
	  mi2_parser_parse_register_values (parser, value);
	  ret = 0;
	}
      else if (name && strcmp (name, DONE_VALUE) == 0)
	{
	  parser->pc = strtol (value, NULL, 0);
	  ret = 0;
	}
      else
	{
	  LOG_ERR (PARSE_ERROR, name == NULL ? (value ? value : "") : name);
	  goto error;
	}
    }
  if (ret != 0)
    {
      goto error;
    }
  return ret;

error:
  LOG_ERR (PARSE_ERROR, line);
  return -1;
}

/**
 * @brief Parse error message
 *
 * @todo Implement error parsing.
 *
 * Parses the error message, which has the form:
 *
 * @code
 * ^error, C-STRING
 * @endcode
 *
 * The \a C-STRING describes the error.
 *
 * @param parser The parser.
 * @param line The error line to be parsed.
 *
 * @return
 */
static int
mi2_parser_parse_error (mi2_parser * parser, char *line)
{
  return -1;
}

/**
 * @brief Parse the asynchrone running message.
 *
 * Parse the *running message. Note that this is not the same as "^running".
 * The message has the form:
 *
 * @code
 * *running,thread-id="THREAD"
 * @endcode
 *
 * The \a THREAD could be 'all' or the thread id.
 *
 * @todo Fix all cases with thread ids.
 *
 * @param parser The parser.
 * @param line the line after "*running,".
 *
 * @return 0 on success, -1 on failure.
 */
static int
mi2_parser_parse_running (mi2_parser * parser, char *line)
{
  char *next;
  char *name;
  char *value;
  int ret = -1;
  int parsed = -1;

  assert (parser);

  next = line;
  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret < 0 || (name == NULL && value == NULL))
	{
	  LOG_ERR (PARSE_ERROR, line);
	  return -1;
	}
      if (strcmp (name, THREAD_RUNNING_ID) == 0)
	{
	  if (strcmp (value, "all") == 0)
	    {
	      ret = thread_set_running (parser->thread_groups, -1, -1, 1, -1);
	      if (ret < 0)
		{
		  LOG_ERR ("Could not set all threads to running.");
		  return -1;
		}
	      view_update_threads (parser->view, parser->thread_groups);
	      parsed = 0;
	    }
	  else
	    {
	      LOG_ERR ("Todo...");
	      return -1;
	    }
	}
      else
	{
	  LOG_ERR (PARSE_ERROR, name ? name : "");
	}
    }

  if (parsed == -1)
    {
      LOG_ERR (PARSE_ERROR, line);
    }
  return parsed;
}

/**
 * @brief Parse the stopped message.
 *
 * Parse the '*stopped' message, which has the form (according to gdb's info).
 *
 * @code
 * *stopped,reason="REASON",thread-id="ID",stopped-threads="THREADS",core="CORE"
 * @endcode
 *
 * Note that frame is also a field.
 *
 * @todo Not fully implemented.
 *
 * \a REASON could be: "breakpoint-hit", "watchpoint-trigger",
 * "read-watchpoint-trigger", "access-watchpoint-trigger", "function-finished",
 * "location-reached", "watchpoint-scope", "end-stepping-range",
 * "exited-signaled", "exited", "exited-normal" and "signal-received".
 *
 * The view will be updated with the new information.
 *
 * @todo Not fully implemented.
 *
 * @param parser The parser.
 * @param line The line after "*stopped,"
 *
 * @return 0 if line was parsed, or -1 on failure.
 */
static int
mi2_parser_parse_stopped (mi2_parser * parser, char *line)
{
  char *next;
  char *name;
  char *value;
  char *endptr;
  char *thread_str = NULL;
  int thread_id;
  int stopped_threads_all = 0;
  int core = -1;
  int parsed = -1;
  char *reason = NULL;
  int disp = -1;
  int number = -1;
  int ret;
  char *wp = NULL;
  char *wp_value = NULL;

  assert (parser);

  stack_clean_frame (parser->stack, -1);

  next = line;
  while (next != NULL && *next != 0)
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret < 0 || name == NULL || value == NULL, -1,
			 "Could not get next parameter '%s'",
			 next ? next : "");
      if (strcmp (name, THREAD_STOPPED) == 0)
	{
	  if (strcmp (value, "all") == 0)
	    {
	      stopped_threads_all = 1;
	    }
	  else
	    {
	      LOG_ERR ("TODO thread '%s' stopped", value);
	    }
	}
      else if (strcmp (name, STOPPED_THREAD_ID) == 0)
	{
	  thread_id = strtol (value, &endptr, 0);
	  LOG_ERR_IF_RETURN (endptr == value, -1, NOT_A_NUMBER, value);
	  parser->thread_id = thread_id;
	  DINFO (1, "id %d %d", thread_id, parsed);
	}
      else if (strcmp (name, "core") == 0)
	{
	  core = strtol (value, &endptr, 0);
	  LOG_ERR_IF (endptr == value, NOT_A_NUMBER, value);
	}
      else if (strcmp (name, STOPPED_REASON) == 0)
	{
	  reason = value;
	}
      else if (strcmp (name, STOPPED_DISP) == 0)
	{
	  if (strcmp (value, BKPT_DEL) == 0)
	    {
	      disp = 0;
	    }
	  else if (strcmp (value, BKPT_KEEP) == 0)
	    {
	      disp = 1;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, value);
	      return -1;
	    }
	}
      else if (strcmp (name, STOPPED_FRAME) == 0)
	{
	  /* The frame information belongs to level 0 of the stack. */
	  ret = mi2_parser_parse_frame (parser, value,
					stack_get_frame (parser->stack, 0));
	  if (ret == 0)
	    {
	      parsed = 0;
	    }
	  parser->stack->depth = 1;
	  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not parse the frame");
	  view_update_frame (parser->view, parser->stack, 0);
	  view_show_file (parser->view, parser->stack->stack[0].fullname,
			  parser->stack->stack[0].line, 1);
	}
      else if (strcmp (name, STOPPED_BKPT_NO) == 0)
	{
	  number = strtol (value, &endptr, 0);
	  LOG_ERR_IF_RETURN (endptr == value, -1, NOT_A_NUMBER, value);
	}
      else if (strcmp (name, STOPPED_WPT) == 0
	       || strcmp (name, STOPPED_HW_WPT) == 0
	       || strcmp (name, STOPPED_HW_RWPT) == 0
	       || strcmp (name, STOPPED_HW_AWPT) == 0)
	{
	  wp = value;
	}
      else if (strcmp (name, STOPPED_VALUE) == 0)
	{
	  wp_value = value;
	}
      else
	{
	  LOG_ERR ("TODO '%s' = '%s'", name, value);
	}
    }

  /* Watchpoint */
  if (wp != NULL && wp_value != NULL)
    {
      if (mi2_parser_parse_watchpoint (parser, wp, wp_value) == 0)
	{
	  parsed = 0;
	  view_update_breakpoints (parser->view, parser->breakpoint_table);
	}
    }

  if (reason != NULL)
    {
      if (stopped_threads_all)
	{
	  VLOG_INFO (parser->view, _("All thread stopped"));
	}
      else
	{
	  VLOG_INFO (parser->view, _("Thread number %s stopped"), thread_str);
	}

      VLOG_INFO (parser->view, _("Stopped reason: %s"), reason);

      if (strcmp (reason, REASON_BREAKPOINT) == 0)
	{
	  if (disp == 0)
	    {
	      /* The breakpoint should not be kept. */
	      LOG_ERR_IF_RETURN (number >= parser->breakpoint_table->rows
				 || parser->
				 breakpoint_table->breakpoints[number] ==
				 NULL, -1, "Could not delete breakpoint %d",
				 number);
	      view_remove_breakpoint (parser->view,
				      parser->
				      breakpoint_table->breakpoints[number]->
				      fullname,
				      parser->
				      breakpoint_table->breakpoints[number]->
				      line);
	      ret = bp_table_remove (parser->breakpoint_table, number);
	      LOG_ERR_IF_RETURN (ret < 0, -1,
				 "Could not delete breakpoint %d", number);
	      view_update_breakpoints (parser->view,
				       parser->breakpoint_table);
	    }
	  parsed = 0;
	}
      else if (strcmp (reason, REASON_EXIT_NORMAL) == 0
	       || strcmp (reason, REASON_EXIT_SIGNALLED) == 0)
	{
	  mi2_parser_exit (parser);
	  parsed = 0;
	}
      else
	{
	  LOG_ERR ("Unknown reason '%s'", reason);
	}
    }
  if (stopped_threads_all == 1)
    {
      thread_set_running (parser->thread_groups, -1, -1, 0, core);
      view_update_threads (parser->view, parser->thread_groups);
      parsed = 0;
    }

  return parsed;
}

/**
 * @brief Parse the thread information message.
 *
 * Parse the "=thread-" messages which has the forms:
 *
 * @code
 * =thread-group-created,id="ID"
 * =thread-group-exited,id="ID"
 * =thread-created,id="ID"
 * =thread-exited,id="ID"
 * =thread-selected,id="ID"
 * @endcode
 *
 * The view will be updated with new thread values.
 *
 * @todo Implement "thread-selected".
 *
 * @param parser The parser.
 * @param line The line after "=thread-"
 *
 * @return 0 on succes or -1 if failed.
 */
static int
mi2_parser_parse_thread (mi2_parser * parser, char *line)
{
  char *next;
  char *name;
  char *value;
  char *endptr;
  int ret;
  int id;
  int group_id;
  int group_found = 0;
  int thread_found = 0;

  if (strncmp (line, THREAD_GROUP_CREATED, strlen (THREAD_GROUP_CREATED)) ==
      0)
    {
      /* thread group creation. */
      next = line + strlen (THREAD_GROUP_CREATED);
      while (next != NULL && *next != '\0')
	{
	  get_next_param (next, &name, &value, &next);
	  if (name == NULL || value == NULL)
	    {
	      LOG_ERR (PARSE_ERROR, line);
	      goto error;
	    }
	  if (strcmp (name, THREAD_ID) == 0)
	    {
	      group_id = strtol (value, &endptr, 0);
	      if (value == endptr)
		{
		  LOG_ERR (NOT_A_NUMBER, value);
		  goto error;
		}
	      group_found = 1;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, name);
	      goto error;
	    }
	}
      if (!group_found)
	{
	  LOG_ERR ("Could not find group id");
	  goto error;
	}
      ret = thread_group_add (&parser->thread_groups, group_id);
    }
  else if (strncmp (line, THREAD_CREATED, strlen (THREAD_CREATED)) == 0)
    {
      /* thread group creation. */
      next = line + strlen (THREAD_CREATED);
      while (next != NULL && *next != '\0')
	{
	  get_next_param (next, &name, &value, &next);
	  if (name == NULL || value == NULL)
	    {
	      LOG_ERR (PARSE_ERROR, line);
	      goto error;
	    }
	  if (strcmp (name, THREAD_ID) == 0)
	    {
	      id = strtol (value, &endptr, 0);
	      if (value == endptr)
		{
		  LOG_ERR (NOT_A_NUMBER, value);
		  goto error;
		}
	      thread_found = 1;
	      parser->thread_id = id;
	    }
	  else if (strcmp (name, THREAD_GROUP_ID) == 0)
	    {
	      group_id = strtol (value, &endptr, 0);
	      if (value == endptr)
		{
		  LOG_ERR (NOT_A_NUMBER, value);
		  goto error;
		}
	      group_found = 1;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, name);
	      goto error;
	    }
	}
      if (!thread_found || !group_found)
	{
	  LOG_ERR ("Could not find thread or group id");
	  goto error;
	}
      ret = thread_add (parser->thread_groups, group_id, id);
      if (ret < 0)
	{
	  goto error;
	}
    }
  else if (strncmp (line, THREAD_GROUP_EXITED, strlen (THREAD_GROUP_EXITED))
	   == 0)
    {
      /* thread group exited. */
      next = line + strlen (THREAD_GROUP_EXITED);
      while (next != NULL && *next != '\0')
	{
	  get_next_param (next, &name, &value, &next);
	  if (name == NULL || value == NULL)
	    {
	      LOG_ERR (PARSE_ERROR, line);
	      goto error;
	    }
	  if (strcmp (name, THREAD_ID) == 0)
	    {
	      group_id = strtol (value, &endptr, 0);
	      if (value == endptr)
		{
		  LOG_ERR (NOT_A_NUMBER, value);
		  goto error;
		}
	      group_found = 1;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, name);
	      goto error;
	    }
	}
      if (!group_found)
	{
	  LOG_ERR ("Could not find group id");
	  goto error;
	}
      ret = thread_group_remove (&parser->thread_groups, group_id);
      LOG_ERR_IF_RETURN (ret < 0, -1, "Bad group id %d", group_id);
    }
  else if (strncmp (line, THREAD_EXITED, strlen (THREAD_EXITED)) == 0)
    {
      group_id = -2;
      id = -2;
      /* thread exited. */
      next = line + strlen (THREAD_EXITED);
      while (next != NULL && *next != '\0')
	{
	  get_next_param (next, &name, &value, &next);
	  if (name == NULL || value == NULL)
	    {
	      LOG_ERR (PARSE_ERROR, line);
	      goto error;
	    }
	  if (strcmp (name, THREAD_ID) == 0)
	    {
	      id = strtol (value, &endptr, 0);
	      if (value == endptr)
		{
		  LOG_ERR (NOT_A_NUMBER, value);
		  goto error;
		}
	      thread_found = 1;
	    }
	  else if (strcmp (name, THREAD_GROUP_ID) == 0)
	    {
	      group_id = strtol (value, &endptr, 0);
	      if (value == endptr)
		{
		  LOG_ERR (NOT_A_NUMBER, value);
		  goto error;
		}
	      group_found = 1;
	    }
	  else
	    {
	      LOG_ERR (PARSE_ERROR, name);
	      goto error;
	    }
	}
      if (!group_found || !thread_found)
	{
	  LOG_ERR ("Could not find thread or group id");
	  goto error;
	}
      ret = thread_remove (parser->thread_groups, group_id, id);
      LOG_ERR_IF_RETURN (ret < 0, -1, "Bad thread id %d group id %d", id,
			 group_id);
    }
  else
    {
      LOG_ERR (PARSE_ERROR, line);
      return -1;
    }

  view_update_threads (parser->view, parser->thread_groups);
  return 0;

error:
  LOG_ERR (PARSE_ERROR, line);
  return -1;
}

/**
 * @brief Parse the library messages.
 *
 * Parse the "=library" message. The messages has the form:
 *
 * @code
 * =library-loaded,id="..",host-name="..",target-name="..",symbols-loaded="1/0"
 * =library-unloaded,id=".",host-name=".",target-name=".",symbols-loaded="1/0"
 * @endcode
 *
 * The view will be updated with the new library information.
 *
 * @param parser The parser.
 * @param line The line after "=library-"
 *
 * @return 0 if parsed or -1 if parsing failed.
 */
static int
mi2_parser_parse_library (mi2_parser * parser, char *line)
{
  char *next;
  char *name;
  char *value;
  char *id = NULL;
  char *host = NULL;
  char *target = NULL;
  int loaded_symbols = -1;
  int load_library;
  int ret;
  char *endptr;

  if (strncmp (line, LIBRARY_LOADED, strlen (LIBRARY_LOADED)) == 0)
    {
      load_library = 1;
      next = line + strlen (LIBRARY_LOADED);
    }
  else if (strncmp (line, LIBRARY_UNLOADED, strlen (LIBRARY_UNLOADED)) == 0)
    {
      load_library = 0;
      next = line + strlen (LIBRARY_UNLOADED);
    }
  else
    {
      LOG_ERR (PARSE_ERROR, line);
      return -1;
    }

  while (next != NULL && *next != '\0')
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret < 0 || name == NULL || value == NULL, -1,
			 "Failed to get next parameter '%s'", line);
      if (strcmp (name, LIBRARY_ID) == 0)
	{
	  id = value;
	}
      else if (strcmp (name, LIBRARY_HOST) == 0)
	{
	  host = value;
	}
      else if (strcmp (name, LIBRARY_TARGET) == 0)
	{
	  target = value;
	}
      else if (strcmp (name, LIBRARY_SYMBOLS) == 0)
	{
	  loaded_symbols = strtol (value, &endptr, 0);
	  LOG_ERR_IF_RETURN (value == endptr, -1, NOT_A_NUMBER, value);
	  LOG_ERR_IF_RETURN (loaded_symbols != 0 && loaded_symbols != 1, -1,
			     "symbols-loaded should be 1 or 0, but it '%d'",
			     loaded_symbols);
	}
      else
	{
	  LOG_ERR (PARSE_ERROR, name);
	  return -1;
	}
    }
  LOG_ERR_IF_RETURN (id == NULL || host == NULL || target == NULL
		     || (loaded_symbols == -1 && load_library == 1), -1,
		     "Could not get all parameter (s):%s%s%s%s",
		     id == NULL ? " id" : "",
		     host == NULL ? " host" : "",
		     target == NULL ? " target" : "",
		     loaded_symbols == -1 && load_library == 1 ?
		     " symbols-loaded" : "");

  if (load_library == 0)
    {
      library_remove (&parser->libraries, id, target, host);
    }
  else
    {
      ret = library_add (&parser->libraries, id, target, host,
			 loaded_symbols);
      LOG_ERR_IF_RETURN (ret < 0, -1, "Could add library");
    }

  view_update_libraries (parser->view, parser->libraries);

  return 0;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Create a new parser.
 *
 * Create a new parser for the mi2 interface.
 *
 * @param view The view associated with the parser.
 * @param conf The configuration used.
 *
 * @return A pointer to the new parser, or NULL if the parser could not be
 *         created.
 */
mi2_parser *
mi2_parser_create (view * view, configuration * conf)
{
  mi2_parser *new_parser;

  assert (view);
  assert (conf);

  new_parser = (mi2_parser *) malloc (sizeof (*new_parser));
  LOG_ERR_IF_FATAL (new_parser == NULL, ERR_MSG_CREATE ("mi2 parser"));

  DINFO (1, "Created new mi2 parser");
  new_parser->view = view;

  new_parser->breakpoint_table = bp_table_create ();

  new_parser->thread_groups = NULL;
  new_parser->libraries = NULL;
  new_parser->frame = -1;
  new_parser->stack = stack_create (10);
  new_parser->thread_id = -1;
  new_parser->disassemble = 0;
  new_parser->ass_lines = ass_create ();
  LOG_ERR_IF_FATAL (new_parser->ass_lines == NULL,
		    ERR_MSG_CREATE ("assembler"));
  new_parser->changed_regs = 0;
  new_parser->regs = 0;
  new_parser->size_regs = 0;
  new_parser->registers = data_registers_create ();
  LOG_ERR_IF_FATAL (new_parser->registers == NULL,
		    ERR_MSG_CREATE ("registers"));

  new_parser->auto_frames = conf_get_bool (conf, NULL, "auto frames", NULL);

  return new_parser;
}

/**
 * @brief Free a parser.
 *
 * Free the parser and it's resources.
 *
 * @param parser The parser to be set free.
 */
void
mi2_parser_free (mi2_parser * parser)
{
  assert (parser);

  if (parser->breakpoint_table != NULL)
    {
      bp_table_free (parser->breakpoint_table);
    }

  if (parser->stack != NULL)
    {
      stack_free (parser->stack);
    }

  if (parser->libraries != NULL)
    {
      library_remove_all (&parser->libraries);
    }

  if (parser->thread_groups != NULL)
    {
      thread_group_remove_all (&parser->thread_groups);
    }

  if (parser->registers != NULL)
    {
      data_registers_free (parser->registers);
    }

  if (parser->ass_lines != NULL)
    {
      ass_free (parser->ass_lines);
    }

  if (parser->regs)
    {
      free (parser->regs);
    }
  free (parser);
}

/**
 * @brief Parse a line from the debugger.
 *
 * Parse a line from the debugger. The line will be altered by the parsing.
 * The function parses both asynchron messages as well as normal records from
 * the debugger.
 *
 * @param parser The parser.
 * @param line The line to be parsed. The line will be altered.
 * @param update Will be set to 1 if the view needs to be updated. E.g. if the
 *               debugger stopped we need to update threads and stack.
 * @param regs Will be set with the registers that need to be updated. NULL if
 *             no registers need to be updated.
 *
 * @return 0 if the line was parsed. -1 if failed to parse the line.
 */
int
mi2_parser_parse (mi2_parser * parser, char *line, int *update, char **regs)
{
  int ret = 0;

  assert (parser);
  assert (line);
  assert (update);

  DINFO (3, "Parsing '%s'", line);

  *update = 0;

  if (*line != '^' && *line != '*' && *line != '=')
    {
      LOG_ERR (PARSE_ERROR, line);
      return -1;
    }

  if (strncmp (line, "^done", 5) == 0)
    {
      DINFO (3, "Got ^done");
      ret = mi2_parser_parse_done (parser, line + 5);
    }
  else if (strncmp (line, "^running", 8) == 0)
    {
      /* Backward compatibility. Which thread?! See info of gdb. */
      DINFO (1, "Got '^running'");
      parser->is_running = 1;
    }
  else if (strncmp (line, "^connected", 10) == 0)
    {
      parser->is_connected = 1;
    }
  else if (strncmp (line, "^error", 6) == 0)
    {
      ret = mi2_parser_parse_error (parser, line + 6);
    }
  else if (strncmp (line, "^exit", 5) == 0)
    {
      parser->is_exit = 1;
    }
  else if (strncmp (line, "*running", 8) == 0)
    {
      ret = mi2_parser_parse_running (parser, line + 8);
    }
  else if (strncmp (line, "*stopped", 8) == 0)
    {
      ret = mi2_parser_parse_stopped (parser, line + 8);
      if (parser->auto_frames)
	{
	  /* Update frames from stack. */
	  *update = 1;
	}
    }
  else if (strncmp (line, "=thread-", 8) == 0)
    {
      ret = mi2_parser_parse_thread (parser, line + 8);
    }
  else if (strncmp (line, "=library-", 9) == 0)
    {
      ret = mi2_parser_parse_library (parser, line + 9);
    }
  else
    {
      ret = -1;
    }
  if (parser->changed_regs)
    {
      *regs = parser->regs;
    }
  LOG_ERR_IF (ret < 0, PARSE_ERROR, line);
  return ret;
}

/**
 * @brief Set the current frame.
 *
 * Set the current frame, from a user input.
 *
 * @param parser The mi2 parser.
 * @param frame The frame level to be chosen.
 */
void
mi2_parser_set_frame (mi2_parser * parser, int frame)
{
  assert (parser);

  DINFO (1, "Selecting frame %d", frame);
  parser->frame = frame;
}

/**
 * @brief Get the current used thread id.
 *
 * @param parser The parser that holds the information about the current
 *               thread id.
 *
 * @return The thread id.
 */
int
mi2_parser_get_thread (mi2_parser * parser)
{
  assert (parser);

  if (parser->thread_id < 0)
    {
      /* no thread id. Pick one if we got one. */
      if (parser->thread_groups && parser->thread_groups->first)
	{
	  parser->thread_id = parser->thread_groups->first->id;
	}
    }
  return parser->thread_id;
}

/**
 * @brief Set thread id.
 *
 * Set the currently used thread id.
 *
 * @param parser The parser.
 * @param id The id to set.
 *
 * @return 0 If we got the id and it is set. -1 if we do not have the id.
 */
int
mi2_parser_set_thread (mi2_parser * parser, int id)
{
  thread *pt;
  thread_group *pg;

  assert (parser);

  /* See if we really got the id. */
  pg = parser->thread_groups;
  while (pg)
    {
      pt = pg->first;
      while (pt)
	{
	  if (pt->id == id)
	    {
	      DINFO (1, "Setting thread id %d", id);
	      parser->thread_id = id;
	      return 0;
	    }
	  pt = pt->next;
	}
      pg = pg->next;
    }

  /* No one found. */
  LOG_ERR ("Do not have the id %d in any thread groups", id);
  return -1;
}

/**
 * @brief Get breakpoint
 *
 * Get the breakpoint in the given file and line number.
 *
 * @param parser The parser.
 * @param file_name The file name which could have a breakpoint.
 * @param line_nr The line number where the breakpoint is.
 *
 * @return A pointer to the breakpoint if it exists. NULL if there is no
 *         breakpoint at the given location.
 */
breakpoint *
mi2_parser_get_bp (mi2_parser * parser, const char *file_name, int line_nr)
{
  int i;

  assert (parser);

  if (file_name == NULL || *file_name == '\0')
    {
      return NULL;
    }

  for (i = 0; i < parser->breakpoint_table->rows; i++)
    {
      if (parser->breakpoint_table->breakpoints[i] != NULL
	  && parser->breakpoint_table->breakpoints[i]->fullname
	  && strcmp (parser->breakpoint_table->breakpoints[i]->fullname,
		     file_name) == 0
	  && parser->breakpoint_table->breakpoints[i]->line == line_nr)
	{
	  return parser->breakpoint_table->breakpoints[i];
	}
    }
  return NULL;
}


/**
 * @brief Remove breakpoint.
 *
 * Remove breakpoint with number @a number.
 *
 * @param parser The parser.
 * @param number The breakpoint number to be removed.
 */
void
mi2_parser_remove_bp (mi2_parser * parser, int number)
{
  int ret;

  assert (parser);

  if (number >= parser->breakpoint_table->rows)
    {
      LOG_ERR ("breakpoint number out of bounds");
      return;
    }
  view_remove_breakpoint (parser->view,
			  parser->breakpoint_table->
			  breakpoints[number]->fullname,
			  parser->breakpoint_table->
			  breakpoints[number]->line);
  ret = bp_table_remove (parser->breakpoint_table, number);
  LOG_ERR_IF (ret < 0, "Not a valid number");
  view_update_breakpoints (parser->view, parser->breakpoint_table);
}

/**
 * @brief Get the current file name and line number.
 *
 * Get the current location, file name and line number.
 *
 * @param parser The parser.
 * @param file The filename.
 * @param line The line number,
 *
 * @return 0 upon succes. - 1 if no file and line number.
 */
int
mi2_parser_get_location (mi2_parser * parser, char **file, int *line)
{
  assert (parser);
  assert (file);
  assert (line);

  if (parser->frame == -1 && parser->stack > 0)
    {
      parser->frame = 0;
    }
  DINFO (1, "frame %d depth %d - %s %d",
	 parser->frame, parser->stack->depth,
	 parser->stack->stack[parser->frame].fullname,
	 parser->stack->stack[parser->frame].line);
  if (parser->frame >= 0 && parser->stack->depth >= parser->frame
      && parser->stack->stack[parser->frame].fullname != NULL)
    {
      *file = parser->stack->stack[parser->frame].fullname;
      *line = parser->stack->stack[parser->frame].line;
      return 0;
    }
  return -1;
}

/**
 * @brief Toggle disassembly mode.
 *
 * Toggle the disassembly mode.
 *
 * @param parser The parser.
 */
void
mi2_parser_toggle_disassemble (mi2_parser * parser)
{
  assert (parser);

  parser->disassemble = !parser->disassemble;
}
