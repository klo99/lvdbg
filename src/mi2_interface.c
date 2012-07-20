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
 * @file mi2_interface.c
 *
 * @brief Implements the mi2 interface.
 *
 * Implements the mi2 interface for sending commands to the debugger.
 *
 */
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "mi2_interface.h"
#include "lvdbg.h"
#include "mi2_parser.h"
#include "view.h"
#include "configuration.h"
#include "debug.h"
#include "misc.h"
#include "win_form.h"

#define SEND_ERROR _("Could not send command: '%s'")

/**
 * @name Breakpoint.
 *
 * Breakpoint commands. Watchpoint and breakpoint manipulation commands.
 */
/*@{*/
#define CMD_BREAK_INSERT     "-break-insert %s %s\n"
#define CMD_BREAK_DELETE     "-break-delete %d\n"
#define CMD_BREAK_WATCHPOINT "-break-watch %s%s\n"
/*@}*/

/**
 * @name Execute.
 *
 * Execute commands.
 *
 * Execution commands for controlling the execution.
 */
/*@{*/
#define CMD_EXEC_CONT   "-exec-continue%s\n"
#define CMD_EXEC_FINISH "-exec-finish%s\n"
#define CMD_EXEC_INTR   "-exec-interrupt%s\n"
#define CMD_EXEC_JUMP   "-exec-jump %s\n"
#define CMD_EXEC_NEXT   "-exec-next"
#define CMD_EXEC_NEXTI  "-exec-next-instruction"
#define CMD_EXEC_RETURN "-exec-return\n"
#define CMD_EXEC_RUN    "-exec-run\n"
#define CMD_EXEC_STEP   "-exec-step"
#define CMD_EXEC_STEPI  "-exec-step-instruction"
#define CMD_EXEC_UNTIL  "-exec-until %s\n"
/*@}*/

/**
 * @name Stack commands.
 *
 * Stack action commands.
 */
/*@{*/
#define CMD_STACK_LIST_FRAMES    "-stack-list-frames --thread %d\n"
#define CMD_STACK_LIST_VARIABLES "-stack-list-variables --thread %d " \
                                 "--frame %d %s\n"
/*@}*/

/**
 * @name Thread commands.
 *
 * Thread commands for selecting and get information of threads..
 */
/*@{*/
#define CMD_THREAD_SELECT "-thread-select %d\n"	/* Deprecated. Not used. */
#define CMD_THREAD_INFO   "-thread-info\n"
/*@}*/

/**
 * @name File commands.
 *
 * Command for retrieving information about current source files in the
 * execution file.
 */
/*@{*/
#define CMD_FILE_LIST_EXEC_SOURCE_FILES "-file-list-exec-source-files\n"
/*@}*/

/**
 * @name Data commands.
 *
 * Data commands e.g. disassemble and register commands.
 */
/*@{*/
#define CMD_DATA_DISASSEMBLE "-data-disassemble -f \"%s\" -l %d -n %d -- 1\n"
#define CMD_DATA_DISASSEMBLE2 "-data-disassemble -s $pc -n %d -- 0\n"
#define CMD_DATA_LIST_REGISTER_NAMES "-data-list-register-names\n"
#define CMD_DATA_LIST_CHANGED_REGISTERS "-data-list-changed-registers\n"
#define CMD_DATA_LIST_REGISTERS_VALUES "-data-list-register-values x %s\n"
#define CMD_DATA_PC "-data-evaluate-expression $pc\n"
/*@}*/

/*******************************************************************************
 * Internal structures and enums
 ******************************************************************************/
/** The mi2 interface structure. */
struct mi2_interface_t
{
  pid_t debugger_pid; /**< The pid of the running debugger. */
  int debugger_fd;    /**< The file descriptor for sending commands to the
                           debugger. */
  view *view;	      /**< The view object. */
  mi2_parser *parser;  /**< The parser object. */

  int disassemble; /**< 1 if we should see disassembly. */
};

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
static void mi2_do_data_list_register_val (mi2_interface * mi2,
					   const char *regs);
static void mi2_do_data_disassembly (mi2_interface * mi2);
static int mi2_do_break_delete (mi2_interface * mi2, int nr);
static int mi2_do_break_insert (mi2_interface * mi2, const char *flags,
				const char *location);
static int mi2_do_break_simple (mi2_interface * mi2);
static int mi2_do_break_advanced (mi2_interface * mi2);
static int mi2_do_break_watchpoint (mi2_interface * mi2, char *exp);
static int mi2_do_exec_cont (mi2_interface * mi2, int advanced);
static int mi2_do_exec_finish (mi2_interface * mi2, int finish);
static int mi2_do_exec_interrupt (mi2_interface * mi2, int advanced);
static int mi2_do_exec_jump (mi2_interface * mi2);
static int mi2_do_exec_step_next (mi2_interface * mi2, int step, int reverse);
static int mi2_do_exec_stepi_nexti (mi2_interface * mi2, int step,
				    int reverse);
static int mi2_do_exec_jump (mi2_interface * mi2);
static int mi2_do_stack_frames (mi2_interface * mi2);
static int mi2_do_stack_variables (mi2_interface * mi2, int frame);
static int mi2_do_simple (mi2_interface * mi2, const char *message);

/**
 * @brief Sends command to retrieve register values.
 *
 * Send command -data-list-register-values.
 *
 * @param mi2 The mi2 interface.
 * @param regs The register that need updated values.
 */
static void
mi2_do_data_list_register_val (mi2_interface * mi2, const char *regs)
{
  char buf[512];
  char *p = buf;
  int size = 512;
  int ret;

  LPRINT (p, p != buf, size, CMD_DATA_LIST_REGISTERS_VALUES, regs);
  LOG_ERR_IF_RETURN (p == NULL,, SEND_ERROR, CMD_DATA_LIST_REGISTERS_VALUES);

  ret = safe_write (mi2->debugger_fd, p);
  LOG_ERR_IF (ret < 0, SEND_ERROR, CMD_DATA_LIST_REGISTERS_VALUES);

  if (p != buf)
    {
      free (p);
    }
}

/**
 * @brief Send disassembly command.
 *
 * Send -data-disassembly command.
 *
 * @param mi2
 */
static void
mi2_do_data_disassembly (mi2_interface * mi2)
{
  int line;
  char *file;
  char buf[512];
  char *p = buf;
  int size = 512;
  int ret;

  ret = mi2_parser_get_location (mi2->parser, &file, &line);
  if (ret < 0 || file == NULL || line < 0)
    {
      VLOG_WARN (mi2->view, _("Failed to retrieve file and line number"));
      return;
    }

  LPRINT (p, p != buf, size, CMD_DATA_DISASSEMBLE, file, line, -1);
  LOG_ERR_IF_FATAL (p == NULL, "Memory");

  ret = safe_write (mi2->debugger_fd, p);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, p);

  if (p != buf)
    {
      free (p);
    }
}

/**
 * @brief Delete specified breakpoint.
 *
 * Send the delete brakpoint command. The command has the for:
 *
 * @code
 * -break-delete ( BREAKPOINT )+
 * @endcode
 *
 * @param mi2 The mi2 interface.
 * @param nr The breakpoint number.
 *
 * @return 0 upon success. -1 if the command could not be sent.
 */
static int
mi2_do_break_delete (mi2_interface * mi2, int nr)
{
  char buf[128];
  char *p = buf;
  int size = 128;
  int ret;

  LPRINT (p, p != buf, size, CMD_BREAK_DELETE, nr);
  ret = safe_write (mi2->debugger_fd, buf);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, buf);

  if (p != buf)
    {
      free (p);
    }
  return ret;
}

/**
 * @brief Do the break insert command.
 *
 * Set up a break insert command. The command has the following structure.
 * -break-insert [ -t ] [ -h ] [ -f ] [ -d ] [ -c CONDITION ]
 *               [ -i IGNORE-COUNT ] [ -p THREAD ] [ LOCATION ]
 *
 * @param mi2 The mi2 interface sending the command.
 * @param flags Flags for the insert command.
 * @param location The location of the breakpoint.
 *
 * @return 0 if the command was sent otherwise -1.
 */
static int
mi2_do_break_insert (mi2_interface * mi2, const char *flags,
		     const char *location)
{
  char buf[128];
  int size = 128;
  char *msg = buf;
  int ret = 0;

  assert (mi2);
  assert (location);

  /* Create message. */
  LPRINT (msg, msg != buf, size, CMD_BREAK_INSERT, flags == NULL ? "" : flags,
	  location);
  if (msg == NULL)
    {
      LOG_ERR ("Could not create break command");
      ret = -1;
      goto error;
    }

  /* Send message. */
  ret = safe_write (mi2->debugger_fd, msg);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, msg);

error:
  if (msg != buf)
    {
      free (msg);
    }
  return ret;
}

/**
 * @brief Set/delete breakpoint.
 *
 * Set or delete breakpoint at the current cursor position in main window.
 * The parser will update the view when the response comes from the debugger,
 * if we insert a breakpoint. When we delete a breakpoint we only get '^done'
 * so we delete the breakpoint here.
 *
 * For valid options see mi2_do_break_insert().
 *
 * @param mi2 The mi2 interface.
 *
 * @return 0 if we sent the breakpoint command. -1 if we didn't.
 */
static int
mi2_do_break_simple (mi2_interface * mi2)
{
  int ret;
  const char *file_name;
  int line_nr;
  char buf[128];
  int size = 128;
  char *p = buf;
  int win = WIN_MAIN;
  breakpoint *bp;

  ret = view_get_cursor (mi2->view, &win, &line_nr, &file_name);
  VLOG_ERR_IF_RETURN (ret < 0, -1, mi2->view,
		      _("Not a valid cursor in 'Main' window"));

  bp = mi2_parser_get_bp (mi2->parser, file_name, line_nr);
  if (bp == NULL)
    {
      /*
       * No breakpoint so insert one.
       * Create the location and call the insert breakpoint function.
       */
      LPRINT (p, p != buf, size, "%s:%d", file_name, line_nr);
      ret = mi2_do_break_insert (mi2, NULL, p);
    }
  else
    {
      /* Remove it. */
      ret = mi2_do_break_delete (mi2, bp->number);
      mi2_parser_remove_bp (mi2->parser, bp->number);
    }

  if (p != buf)
    {
      free (p);
    }

  return ret;
}

/**
 * @brief Insert breakpoint advanced.
 *
 * Insert or update a breakpoint with options. Ask user which options to send
 * with the insert command. If the breakpoint already exists the old one
 * is removed before inserting a new.
 *
 * @param mi2 The interface.
 *
 * @return 0 upon success, otherwise -1.
 */
static int
mi2_do_break_advanced (mi2_interface * mi2)
{
  int ret;
  const char *file_name;
  int line_nr;
  char loc[128];
  char *pl = loc;
  char opt[128];
  char *po = opt;
  char cond[128];
  char *pc = cond;
  char all[512];
  char *pall = all;
  int size = 128;
  int win = WIN_MAIN;
  input_field fields[] = {
    {_("Temporary"), NULL, INPUT_TYPE_BOOL, {.bool_value = 0}, 0, 0, NULL},
    {_("Hardware"), NULL, INPUT_TYPE_BOOL, {.bool_value = 0}, 0, 0, NULL},
    {_("Condition"), NULL, INPUT_TYPE_STRING, {.string_value = NULL}, 0, 0,
     NULL},
    {_("Ignore"), NULL, INPUT_TYPE_INT, {.int_value = 0}, 0, 0, NULL},
    {_("Thread"), NULL, INPUT_TYPE_INT, {.int_value = -1}, 0, 0, NULL},
    {_("Pending"), NULL, INPUT_TYPE_BOOL, {.bool_value = 0}, 0, 0, NULL},
    {_("Disabled"), NULL, INPUT_TYPE_BOOL, {.bool_value = 0}, 0, 0, NULL},
    {_("*Location"), NULL, INPUT_TYPE_STRING, {.string_value = NULL}, 0, 0,
     NULL},
    {NULL},
  };
  breakpoint *bp = NULL;

  ret = view_get_cursor (mi2->view, &win, &line_nr, &file_name);
  if (ret == 0)
    {
      /* Get the location 'fullname:line_nr'. */
      LPRINT (pl, pl != loc, size, "%s:%d", file_name, line_nr + 1);
      fields[7].string_value = pl;

      bp = mi2_parser_get_bp (mi2->parser, file_name, line_nr);
      if (bp)
	{
	  /* We already got one bp here, so update it. */
	  fields[0].bool_value = !bp->disp;
	  fields[2].string_value = bp->cond;
	  fields[3].int_value = bp->ignore;
	  fields[4].int_value = bp->thread;
	  fields[6].bool_value = !bp->enabled;
	}
    }

  ret = form_run (fields, _("Breakpoint insertion"));
  if (ret < 0)
    {
      /* Canceled */
      ret = 0;
      goto error;
    }
  if (fields[7].string_value == NULL)
    {
      VLOG_ERR (mi2->view, _("Location must have a value"));
      ret = 0;
      goto error;
    }

  /* Get the flag options. */
  po[0] = '\0';
  size = 128;
  if (fields[3].int_value > 0 && fields[4].int_value >= 0)
    {
      LPRINT (po, po != opt, size, "-i %d -p %d", fields[3].int_value,
	      fields[4].int_value);
    }
  else if (fields[3].int_value > 0)
    {
      LPRINT (po, po != opt, size, "-i %d", fields[3].int_value);
    }
  else if (fields[4].int_value >= 0)
    {
      LPRINT (po, po != opt, size, "-p %d", fields[4].int_value);
    }

  /* Get condition. */
  *pc = '\0';
  if (fields[2].string_value)
    {
      size = 128;
      LPRINT (pc, pc != cond, size, " -c \"%s\"", fields[2].string_value);
    }

  /* Get the rest of the single flags. */
  size = 512;
  LPRINT (pall, pall != all, size, "%s%s%s%s%s%s", po, pc,
	  fields[0].bool_value ? " -t" : "",
	  fields[1].bool_value ? " -h" : "",
	  fields[5].bool_value ? " -f" : "",
	  fields[6].bool_value ? " -d" : "");

  if (bp != NULL)
    {
      /* Remove the old one first. */
      ret = mi2_do_break_delete (mi2, bp->number);
      mi2_parser_remove_bp (mi2->parser, bp->number);
      VLOG_ERR_IF (ret < 0, mi2->view, _("Failed to delete breakpoint"));
    }
  DINFO (3, "BP advance : %s", pall);
  ret = mi2_do_break_insert (mi2, pall, fields[7].string_value);
  /* Fall through. */

error:
  if (fields[2].string_value != NULL)
    {
      free (fields[2].string_value);
    }
  if (fields[7].string_value != NULL)
    {
      if (fields[7].string_value != pl)
	{
	  free (fields[7].string_value);
	}
    }
  if (pl != loc)
    {
      free (pl);
    }
  if (po != opt)
    {
      free (po);
    }
  if (pc != cond)
    {
      free (pc);
    }
  if (pall != all)
    {
      free (pall);
    }
  return ret;
}

/**
 * @brief Add a watchpoint.
 *
 * Set up a form for the watchpoint options. If the user does not cancel,
 * the add watchpoint command is sent. The form of the watchpoint is:
 *
 * @code
 * -break-watch [-a | -r]
 * @endcode
 *
 * @param mi2 The mi2 interface.
 * @param exp If non null the default expression is set to @a exp.
 *
 * @return 0 if the command was sent or the user canceled the form. -1 if
 *         the command could not be sent or no expression was set.
 */
static int
mi2_do_break_watchpoint (mi2_interface * mi2, char *exp)
{
  const char *opt[] = { "-a ", "-r ", "" };
  const char *wtypes[] = { _("Access"), _("Read"), _("Write"), NULL };
  input_field fields[] = {
    {_("*Expression"), NULL, INPUT_TYPE_STRING, {.string_value = exp},
     0, 0, NULL},
    {_("Type"), NULL, INPUT_TYPE_ENUM, {.enum_value = 2}, 0, 0, wtypes},
    {NULL},
  };
  int ret;
  int size = 64;
  char buf[64];
  char *p = buf;

  /* Run the watchpoint form. */
  ret = form_run (fields, _("Add watchpoint"));
  if (ret < 0)
    {
      /* Cancel. */
      ret = 0;
      goto error;
    }

  if (fields[0].string_value == NULL || strlen (fields[0].string_value) == 0)
    {
      VLOG_ERR (mi2->view, _("Watchpoint must have an expression"));
      ret = -1;
      goto error;
    }

  LPRINT (p, p != buf, size, CMD_BREAK_WATCHPOINT, opt[fields[1].enum_value],
	  fields[0].string_value);

  ret = safe_write (mi2->debugger_fd, p);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, p);
  /* Fall through. */

error:
  if (exp == NULL || fields[0].string_value != exp)
    {
      free (fields[0].string_value);
    }
  if (p != buf)
    {
      free (p);
    }
  return ret;
}

/**
 * @brief Do execute continue command.
 *
 * Creates and send a execute continue command. The command has the form:
 *
 * @code
 * -exec-cont [--reverse] [--all | --thread-group N]
 * @endcode
 *
 * If @a advanced is set a form for the options are set up for user
 * interaction.
 *
 * @param mi2 The mi2 interface.
 * @param advanced If set to 1 a form is set up for the options.
 *
 * @return 0 if the command was sent or user pressed 'Cancel'. -1 if the
 *         command couldn't be sent.
 */
static int
mi2_do_exec_cont (mi2_interface * mi2, int advanced)
{
  input_field fields[] = {
    {_("Reverse"), NULL, INPUT_TYPE_BOOL, {.bool_value = 0}, 0, 0, NULL},
    {_("All"), NULL, INPUT_TYPE_BOOL, {.bool_value = 0}, 0, 0, NULL},
    {_("Thread group"), NULL, INPUT_TYPE_INT, {.int_value = -1}, 0, 0, NULL},
    {NULL},
  };
  int ret;
  char cmd[128];
  char *pc = cmd;
  char options[64];
  char *po = options;
  int size;

  options[0] = '\0';
  if (advanced)
    {
      /* Run form to get continue options. */
      ret = form_run (fields, _("Continue options"));
      if (ret < 0)
	{
	  return 0;
	}

      size = 64;
      if (fields[2].int_value > -1)
	{
	  LPRINT (po, po != options, size, "%s --thread-group %d",
		  fields[0].bool_value ? " --reverse" : "",
		  fields[2].int_value);
	}
      else
	{
	  LPRINT (po, po != options, size, "%s%s",
		  fields[0].bool_value ? " --reverse" : "",
		  fields[1].bool_value ? " --all" : "");
	}
    }

  /* Create command and send it. */
  size = 128;
  LPRINT (pc, pc != cmd, size, CMD_EXEC_CONT, po);
  ret = safe_write (mi2->debugger_fd, pc);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, pc);

  if (po != options)
    {
      free (po);
    }
  if (pc != cmd)
    {
      free (pc);
    }

  return ret;
}

/**
 * @brief Create and send execution finish command.
 *
 * Create and send execution finish command, which has the form:
 *
 * @code
 * -exec-finish [--reverse]
 * @endcode
 *
 * @param mi2 The mi2 interface.
 * @param reverse If set to 1, add option `--reverse'
 *
 * @return 0 if the command was sent. -1 if sending command failed.
 */
static int
mi2_do_exec_finish (mi2_interface * mi2, int reverse)
{
  const char *rev = " --reverse";
  char cmd[64];			/* Enough for finish command + reverse. */
  int ret;

  sprintf (cmd, CMD_EXEC_FINISH, reverse ? rev : "");

  ret = safe_write (mi2->debugger_fd, cmd);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, cmd);

  return ret;
}

/**
 * @brief Do execute interrupt command.
 *
 * Creates and send a execute interrupt command. The command has the form:
 *
 * @code
 * -exec-interrupt [--all | --thread-group N]
 * @endcode
 *
 * If @a advanced is set a form for the options are set up for user
 * interaction.
 *
 * @param mi2 The mi2 interface.
 * @param advanced If set to 1 a form is set up for the options.
 *
 * @return 0 if the command was sent or user pressed 'Cancel'. -1 if the
 *         command couldn't be sent.
 */
static int
mi2_do_exec_interrupt (mi2_interface * mi2, int advanced)
{
  input_field fields[] = {
    {_("All"), NULL, INPUT_TYPE_BOOL, {.bool_value = 0}, 0, 0, NULL},
    {_("Thread group"), NULL, INPUT_TYPE_INT, {.int_value = -1}, 0, 0, NULL},
    {NULL},
  };
  int ret;
  char cmd[128];
  char *pc = cmd;
  char options[64];
  char *po = options;
  int size = 64;

  options[0] = '\0';
  if (advanced)
    {
      /* Run form to get continue options. */
      ret = form_run (fields, _("Interrupt options"));
      if (ret < 0)
	{
	  return 0;
	}

      if (fields[1].int_value > -1)
	{
	  LPRINT (po, po != options, size, " --thread-group %d",
		  fields[1].int_value);
	}
      else
	{
	  /* Enough room in po. */
	  sprintf (po, "%s", fields[0].bool_value ? " --all" : "");
	}
    }

  /* Create command and send it. */
  size = 128;
  LPRINT (pc, pc != cmd, size, CMD_EXEC_INTR, po);
  ret = safe_write (mi2->debugger_fd, cmd);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, cmd);

  if (pc != cmd)
    {
      free (pc);
    }
  if (po != options)
    {
      free (po);
    }
  return ret;
}

/**
 * @brief Do execute jump command.
 *
 * Creates and send a execute jump command. The command has the form:
 *
 * @code
 * -exec-jump LOCATION
 * @endcode
 *
 * If the Main window has a valid file name, the main window's cursor
 * position is used as an default @a location.
 * @param mi2 The mi2 interface.
 *
 * @return 0 if the command was sent or user pressed 'Cancel'. -1 if the
 *         command couldn't be sent.
 */
static int
mi2_do_exec_jump (mi2_interface * mi2)
{
  input_field fields[] = {
    {_("Location"), NULL, INPUT_TYPE_STRING, {.string_value = NULL}, 0, 0,
     NULL},
    {NULL},
  };
  int ret;
  char cmd[512];
  char *pc = cmd;
  char location[512];
  char *pl = location;
  const char *file_name = NULL;
  int line_nr;
  int win = WIN_MAIN;
  int size = 512;

  location[0] = '\0';
  ret = view_get_cursor (mi2->view, &win, &line_nr, &file_name);
  if (ret == 0)
    {
      /* We got a valid cursor pos, so make it a default jump location. */
      LPRINT (pl, pl != location, size, "%s:%d", file_name, line_nr + 1);
    }
  fields[0].string_value = pl;

  /* Run form to get continue options. */
  ret = form_run (fields, _("Jump location"));
  if (ret < 0)
    {
      ret = 0;
      goto error;
    }

  /* Do simple sanity check. */
  if (fields[0].string_value == NULL || strlen (fields[0].string_value) == 0)
    {
      VLOG_ERR (mi2->view, _("Location must be set"));
      ret = -1;
      goto error;
    }

  /* Create command and send it. */
  size = 512;
  LPRINT (pc, pc != cmd, size, CMD_EXEC_JUMP, fields[0].string_value);
  ret = safe_write (mi2->debugger_fd, pc);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, pc);
  /* Fallthrough. */

error:
  if (fields[0].string_value != pl && fields[0].string_value != location)
    {
      free (fields[0].string_value);
    }
  if (pl != location)
    {
      free (pl);
    }
  if (pc != cmd)
    {
      free (pc);
    }

  return ret;
}

/**
 * @brief Send -exec-next/step [--reverse] command.
 *
 * Sends the -exec-next/stop command to the debugger.
 *
 * @param mi2 The mi2 interface.
 * @param step If set to 0 -exec-next is sent, otherwise -exec-step is sent.
 * @param reverse If 0 it does not send the option --reverse. Otherwise the
 *                reverse option is sent.
 *
 * @return 0 upon success, otherwise -1.
 */
static int
mi2_do_exec_step_next (mi2_interface * mi2, int step, int reverse)
{
  char msg[64];
  char *p = msg;
  int size = 64;
  int ret;

  LPRINT (p, p != msg, size, "%s --thread %d%s\n",
	  step ? CMD_EXEC_STEP : CMD_EXEC_NEXT,
	  mi2_parser_get_thread (mi2->parser), reverse ? " --reverse" : "");
  ret = safe_write (mi2->debugger_fd, msg);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, msg);

  if (p != msg)
    {
      free (p);
    }
  return ret;
}

/**
 * @brief Send -exec-nexti/stepi [--reverse] command.
 *
 * Sends the -exec-next/stop command to the debugger.
 *
 * @param mi2 The mi2 interface.
 * @param step If set to 0 -exec-nexti is sent, otherwise -exec-stepi is sent.
 * @param reverse If 0 it does not send the option --reverse. Otherwise the
 *                reverse option is sent.
 *
 * @return 0 upon success, otherwise -1.
 */
static int
mi2_do_exec_stepi_nexti (mi2_interface * mi2, int step, int reverse)
{
  char msg[64];
  char *p = msg;
  int size = 64;
  int ret;

  LPRINT (p, p != msg, size, "%s --thread %d%s\n",
	  step ? CMD_EXEC_STEPI : CMD_EXEC_NEXTI,
	  mi2_parser_get_thread (mi2->parser), reverse ? " --reverse" : "");
  ret = safe_write (mi2->debugger_fd, msg);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, msg);

  if (p != msg)
    {
      free (p);
    }

  return ret;
}

/**
 * @brief Do execute until command.
 *
 * Creates and send a execute until command. The command has the form:
 *
 * @code
 * -exec-until [LOCATION]
 * @endcode
 *
 * If the Main window has a valid file name, the main window's cursor
 * position is used as an default @a location.
 *
 * @param mi2 The mi2 interface.
 *
 * @return 0 if the command was sent or user pressed 'Cancel'. -1 if the
 *         command couldn't be sent.
 */
static int
mi2_do_exec_until (mi2_interface * mi2)
{
  input_field fields[] = {
    {_("Location"), NULL, INPUT_TYPE_STRING, {.string_value = NULL}, 0, 0,
     NULL},
    {NULL},
  };
  int ret;
  char cmd[512];
  char *pc = cmd;
  char location[512];
  char *pl = location;
  int size = 512;
  const char *file_name = NULL;
  int line_nr;
  int win = WIN_MAIN;

  location[0] = '\0';
  ret = view_get_cursor (mi2->view, &win, &line_nr, &file_name);
  if (ret == 0)
    {
      /* We got a valid cursor pos, so make it a default jump location. */
      LPRINT (pl, pl != location, size, "%s:%d", file_name, line_nr + 1);
    }
  fields[0].string_value = pl;

  /* Run form to get location. */
  ret = form_run (fields, _("Execute until location"));
  if (ret < 0)
    {
      ret = 0;
      goto error;
    }

  /* Create command and send it. */
  size = 512;
  LPRINT (pc, pc != cmd, size, CMD_EXEC_UNTIL,
	  fields[0].string_value ? fields[0].string_value : "");
  ret = safe_write (mi2->debugger_fd, pc);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, pc);
  /* Fallthrough. */

error:
  if (fields[0].string_value != NULL && fields[0].string_value != pl)
    {
      free (fields[0].string_value);
    }

  if (pl != location)
    {
      free (pl);
    }
  if (pc != cmd)
    {
      free (pc);
    }

  return ret;
}

/**
 * @brief Send list frame command.
 *
 * Send -stack-list-frame command.
 *
 * @param mi2 The mi2.
 *
 * @return 0 upon success.
 */
static int
mi2_do_stack_frames (mi2_interface * mi2)
{
  char buf[64];
  char *p = buf;
  int size = 64;
  int thread;
  int ret;

  assert (mi2);

  thread = mi2_parser_get_thread (mi2->parser);

  /* Get all types. */
  LPRINT (p, p != buf, size, CMD_STACK_LIST_FRAMES, thread);
  ret = safe_write (mi2->debugger_fd, p);
  VLOG_WARN_IF_RETURN (ret < 0, -1, mi2->view, SEND_ERROR, p);

  if (p != buf)
    {
      free (p);
    }
  return ret;
}

/**
 * @brief Send the -stack-list-variables.
 *
 * Sends the -stack-list-variables to the debugger two times. First with the
 * option --simple-values, to retrieve name and type. The second time with
 * the option --all-values to get all values (but not the types).
 *
 * The commands must be sent with the current thread id.
 *
 * @param mi2 The mi2 interface-
 * @param frame The frame level to fetch the variables from.
 *
 * @return 0 upon success, otherwise -1.
 */
static int
mi2_do_stack_variables (mi2_interface * mi2, int frame)
{
  char buf[64];
  char *p = buf;
  int size = 64;
  int thread;
  int ret;

  assert (mi2);

  thread = mi2_parser_get_thread (mi2->parser);

  /* Get all types. */
  LPRINT (p, p != buf, size, CMD_STACK_LIST_VARIABLES, thread, frame,
	  "--simple-values");
  ret = safe_write (mi2->debugger_fd, p);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, p);

  /* Get all values. */
  LPRINT (p, p != buf, size, CMD_STACK_LIST_VARIABLES, thread, frame,
	  "--all-values");
  ret = safe_write (mi2->debugger_fd, p);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, p);

  if (p != buf)
    {
      free (p);
    }
  return 0;
}

/**
 * @brief Send a simple command to the debugger.
 *
 * Send a simple @a command with no options to the debugger.
 *
 * @param mi2 The mi2 interface.
 * @param command The command to be sent to the debugger.
 *
 * @return 0 upon success otherwise -1.
 */
static int
mi2_do_simple (mi2_interface * mi2, const char *command)
{
  int ret;

  assert (mi2);
  assert (command);

  ret = safe_write (mi2->debugger_fd, command);
  VLOG_WARN_IF (ret < 0, mi2->view, SEND_ERROR, command);

  return ret;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Create a mi2 interface object.
 *
 * Create a mi2 interface object, for sending mi2 commands and parsing mi2
 * records.
 *
 * @param fd The file descriptor for sending commands to the debugger.
 * @param pid The pid of the debugger.
 * @param view The view object.
 * @param conf The user configuration.
 *
 * @return A pointer to the interface object, or NULL if failed to create the
 *         object.
 */
mi2_interface *
mi2_create (int fd, pid_t pid, view * view, configuration * conf)
{
  mi2_interface *mi2;

  DINFO (1, "Creating mi2 interface");

  mi2 = (mi2_interface *) malloc (sizeof (*mi2));
  LOG_ERR_IF_FATAL (mi2 == NULL, ERR_MSG_CREATE ("mi2 interface"));

  mi2->disassemble = -1;
  mi2->debugger_fd = fd;
  mi2->debugger_pid = pid;
  mi2->view = view;
  mi2->parser = mi2_parser_create (view, conf);
  if (mi2->parser == NULL)
    {
      free (mi2);
      return NULL;
    }

  return mi2;
}

/**
 * @brief Free a mi2 interface.
 *
 * Free a mi2 interface and it's resources.
 *
 * @param mi2 Interface to be set free.
 */
void
mi2_free (mi2_interface * mi2)
{
  assert (mi2);

  DINFO (1, "Freeing mi2 interface");

  if (mi2->parser != NULL)
    {
      mi2_parser_free (mi2->parser);
    }
  free (mi2);
}

/**
 * @brief Parse information sent by the debugger.
 *
 * Handle information sent from the debugger by dispatching the information
 * to the mi2 parser object.
 *
 * @param mi2 The mi2 interface object.
 * @param line The line with information. NOTE: The line will be altered while
 *             parsing the line.
 *
 * @return 0 if the line was parsed. -1 if the parser failed.
 */
int
mi2_parse (mi2_interface * mi2, char *line)
{
  int ret;
  int update = 0;
  char *regs = NULL;

  DINFO (3, "Parsing '%s'", line);
  ret = mi2_parser_parse (mi2->parser, line, &update, &regs);
  if (ret == 0 && regs != NULL)
    {
      mi2_do_data_list_register_val (mi2, regs);
    }
  if (ret == 0 && update)
    {
      ret = mi2_do_action (mi2, ACTION_STACK_LIST_FRAMES, 0);
      if (ret < 0)
	{
	  return ret;
	}
      ret = mi2_do_action (mi2, ACTION_THREAD_INFO, 0);
      if (ret < 0)
	{
	  return ret;
	}
      if (mi2->disassemble > 0)
	{
	  ret = mi2_do_simple (mi2, CMD_DATA_LIST_CHANGED_REGISTERS);
	  LOG_ERR_IF_RETURN (ret < 0, ret, SEND_ERROR,
			     CMD_DATA_LIST_REGISTERS_VALUES);
	  ret = mi2_do_simple (mi2, CMD_DATA_PC);
	  LOG_ERR_IF_RETURN (ret < 0, ret, SEND_ERROR, CMD_DATA_PC);
	  mi2_do_data_disassembly (mi2);
	}
    }

  return ret;
}

/**
 * @brief Do mi2 actions.
 *
 * Do mi2 actions by create the command and send it to the debugger.
 *
 * @param mi2 The mi2 interface obejct.
 * @param action The action to be sent.
 * @param param Parameters needed to send the action. Not in use.
 *
 * @return 0 if the command was sent. -1 on failure.
 */
int
mi2_do_action (mi2_interface * mi2, int action, int param)
{
  int ret = 0;

  DINFO (1, "Do command %d", action);

  switch (action)
    {
    case ACTION_INT_START:
      ret = mi2_do_break_insert (mi2, "-t", "main");
      if (ret < 0)
	{
	  goto error;
	}
      ret = mi2_do_simple (mi2, CMD_EXEC_RUN);
      break;
    case ACTION_INT_UPDATE:
      ret = mi2_do_action (mi2, ACTION_STACK_LIST_FRAMES, 0);
      if (ret < 0)
	{
	  return ret;
	}
      ret = mi2_do_action (mi2, ACTION_THREAD_INFO, 0);
      if (ret < 0)
	{
	  return ret;
	}
      ret = mi2_do_action (mi2, ACTION_STACK_LIST_VARIABLES, 0);
      break;
    case ACTION_EXEC_CONT:
      ret = mi2_do_exec_cont (mi2, 0);
      break;
    case ACTION_EXEC_CONT_OPT:
      ret = mi2_do_exec_cont (mi2, 1);
      break;
    case ACTION_EXEC_FINISH:
      ret = mi2_do_exec_finish (mi2, 0);
      break;
    case ACTION_EXEC_INTR:
      ret = mi2_do_exec_interrupt (mi2, param);
      break;
    case ACTION_EXEC_JUMP:
      ret = mi2_do_exec_jump (mi2);
      break;
    case ACTION_EXEC_NEXT:
      ret = mi2_do_exec_step_next (mi2, 0, param);
      break;
    case ACTION_EXEC_NEXTI:
      ret = mi2_do_exec_stepi_nexti (mi2, 0, param);
      break;
    case ACTION_EXEC_RETURN:
      ret = mi2_do_simple (mi2, CMD_EXEC_RETURN);
      break;
    case ACTION_EXEC_RUN:
      ret = mi2_do_simple (mi2, CMD_EXEC_RUN);
      break;
    case ACTION_EXEC_STEP:
      ret = mi2_do_exec_step_next (mi2, 1, param);
      break;
    case ACTION_EXEC_STEPI:
      ret = mi2_do_exec_stepi_nexti (mi2, 1, param);
      break;
    case ACTION_EXEC_UNTIL:
      ret = mi2_do_exec_until (mi2);
      break;
    case ACTION_STACK_LIST_FRAMES:
      ret = mi2_do_stack_frames (mi2);
      break;
    case ACTION_STACK_LIST_VARIABLES:
      mi2_parser_set_frame (mi2->parser, param);
      ret = mi2_do_stack_variables (mi2, param);
      break;
    case ACTION_BP_SIMPLE:
      ret = mi2_do_break_simple (mi2);
      break;
    case ACTION_BP_ADVANCED:
      ret = mi2_do_break_advanced (mi2);
      break;
    case ACTION_BP_WATCHPOINT:
      ret = mi2_do_break_watchpoint (mi2, NULL);
      break;
    case ACTION_THREAD_SELECT:
      ret = mi2_parser_set_thread (mi2->parser, param);
      break;
    case ACTION_THREAD_INFO:
      ret = mi2_do_simple (mi2, CMD_THREAD_INFO);
      break;
    case ACTION_FILE_LIST_EXEC_SORCES:
      ret = mi2_do_simple (mi2, CMD_FILE_LIST_EXEC_SOURCE_FILES);
      break;
    case ACTION_DATA_DISASSEMBLE:
      mi2_do_data_disassembly (mi2);
      break;
    default:
      LOG_ERR ("Unknown action type");
      ret = -1;
    }

error:
  if (ret != 0)
    {
      LOG_ERR ("Failed to do command %d %d", action, param);
    }
  return ret;
}

/**
 * @brief Toggle the disassemble mode.
 *
 * Toggle disassemble mode.
 *
 * @param mi2 The mi2 interface.
 */
void
mi2_toggle_disassemble (mi2_interface * mi2)
{
  assert (mi2);

  if (mi2->disassemble < 0)
    {
      /* First time we get the register names. */
      mi2_do_simple (mi2, CMD_DATA_LIST_REGISTER_NAMES);
      mi2->disassemble = 1;
    }
  else
    {
      mi2->disassemble = !mi2->disassemble;
    }
  mi2_parser_toggle_disassemble (mi2->parser);
  if (mi2->disassemble != 0)
    {
      /* Fetch disassemble data. */
      mi2_do_data_disassembly (mi2);
    }
}
