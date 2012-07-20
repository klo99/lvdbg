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
 * @file mi2_interface.h
 *
 * @brief Handle the actions sent to the debugger and parsing information
 * from the debugger.
 *
 * The interface for sending the actions to the debugger. The parsing of the
 * information sent by the debugger are handled by the parser object. See
 * mi2_parser.h for more information.
 *
 * If an action needs options a form is set up for user interactions.
 */
#ifndef MI2_INTERFACE_H
#define MI2_INTERFACE_H
#include <unistd.h>

#include "view.h"
#include "configuration.h"

/*******************************************************************************
 * Enums
 ******************************************************************************/

/**
 * Map of available debugger actions.
 */
enum mi2_actions
{
  /**
   * @name Compound actions.
   *
   * Compound action built up by sending multiple commands or using non mi2
   * commands.
   */
  /*@{ */
  ACTION_INT_START, /**< Run the debugger to main (). */
  ACTION_INT_UPDATE, /**< Send commands to debugger for update inforamtion. */
  /*@} */

  /**
   * @name Execution actions.
   *
   * The -exec- family commands.
   */
  /*@{ */
  ACTION_EXEC_CONT,    /**< Continue the execution. */
  ACTION_EXEC_CONT_OPT,/**< Continue the execution with options. */
  ACTION_EXEC_FINISH,  /**< Execute to end of function. */
  ACTION_EXEC_INTR,    /**< Send signal to the debugger. */
  ACTION_EXEC_JUMP,    /**< Jump to a location. */
  ACTION_EXEC_NEXT,    /**< Execut one source line. */
  ACTION_EXEC_NEXTI,   /**< Execute one instruction. */
  ACTION_EXEC_RETURN,  /**< Return from function directly. */
  ACTION_EXEC_RUN,     /**< Execute from beginning. */
  ACTION_EXEC_STEP,    /**< Execute one source line, step into functions. */
  ACTION_EXEC_STEPI,   /**< Execute one instruction, step into functions. */
  ACTION_EXEC_UNTIL,   /**< Execute until given location. */
  /*@} */

  /**
   * @name Stack actions.
   *
   * The -stack- family comands.
   */
  /*@{ */
  ACTION_STACK_LIST_FRAMES,    /**< List frames in stack. */
  ACTION_STACK_LIST_VARIABLES, /**< List variables in the current frame. */
  /*@} */

  /**
   * @name Breakpoint actions.
   *
   * The -break- family commands.
   */
  /*@{ */
  ACTION_BP_SIMPLE,	/**< Set breakpoint at current cursor line. */
  ACTION_BP_ADVANCED,	/**< Set up a breakpoint with options. */
  ACTION_BP_WATCHPOINT,	/**< Set up a watchpoint. */
  /*@} */

  /**
   * @name Thread actions.
   *
   * The thread commands.
   */
  /*@{ */
  ACTION_THREAD_SELECT,	/**< Selects thread. */
  ACTION_THREAD_INFO,	/**< Ask for thread information. */
  /*@} */

  /**
   * @name File actions.
   *
   * The file command.
   */
  /*@{ */
  ACTION_FILE_LIST_EXEC_SORCES,	/**< List source files. */
  /*@} */

  /**
   * @name Data actions.
   *
   * The available data commands.
   */
  /*@{ */
  ACTION_DATA_DISASSEMBLE, /**< Ask for disassemble information. */
  /*@} */
};

/*******************************************************************************
 * Typedefs
 ******************************************************************************/
typedef struct mi2_interface_t mi2_interface;

/*******************************************************************************
 * Public functions
 ******************************************************************************/
mi2_interface *mi2_create (int fd, pid_t pid, view * view,
			   configuration * conf);
void mi2_free (mi2_interface * mi2);
int mi2_parse (mi2_interface * mi2, char *line);
int mi2_do_action (mi2_interface * mi2, int action, int param);
void mi2_toggle_disassemble (mi2_interface * mi2);
#endif
