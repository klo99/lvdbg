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
 * @file input.c
 *
 * @brief Handler for user input.
 *
 * Handles the user input and take action towards either the debugger
 * or the view.
 *
 */
#include <stdlib.h>
#include <ncurses.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "input.h"
#include "lvdbg.h"
#include "view.h"
#include "mi2_interface.h"
#include "configuration.h"
#include "debug.h"
#include "win_form.h"

#define LINE_SIZE 1024
/******************************************************************************
 * Internal structures and enums
 *****************************************************************************/
/** The input structure for the mi2 interface. */
struct input_t
{
  mi2_interface *mi2;  /**< The mi2 interface to send actions to the debugger.*/
  view *view;	       /**< The view. For commands that alter the view. */
  configuration *conf; /**< The configuration. */
  int fd;	       /**< The fd to the debugger. */
};

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
void input_change_mode (input * input);
void input_load_file (input * input);
int inputParseEnter (input * input);

/**
 * @brief Put view in non ncurse mode.
 *
 * Put view in non ncurse mode and send command or user input direct to gdb.
 *
 * NOTE: lvdbg do not parser the debugger output.
 * NOTE: To stop the mode press Ctrl-q
 *
 * @todo This function should not be in input. Mainloop?
 *
 * @param input The input.
 */
void
input_change_mode (input * input)
{
  int ret;
  char ch;
  struct termios attr;
  struct termios org;
  FILE *f;
  char line[LINE_SIZE];
  fd_set rfds;
  int retval;

  assert (input);

  view_toggle_view_mode (input->view);
  fprintf (stdout, _("Press <C-q> to go back to lvdbg\r\n"));
  fflush (stdout);

  f = fdopen (input->fd, "r+");
  VLOG_WARN_IF_RETURN (f == NULL,, input->view,
		       _("Could set no ncurse mode"));

  tcgetattr (0, &attr);
  memcpy (&org, &attr, sizeof (attr));
  cfmakeraw (&attr);
  tcsetattr (0, TCSANOW, &attr);

  do
    {
      FD_ZERO (&rfds);
      FD_SET (0, &rfds);
      FD_SET (input->fd, &rfds);

      retval = select (input->fd + 1, &rfds, NULL, NULL, NULL);
      if (retval == -1)
	{
	  LOG_ERR ("Select error");
	  break;
	}
      else if (retval)
	{
	  if (FD_ISSET (0, &rfds))
	    {
	      ret = getch ();
	      if (ret == 0x11)
		{
		  break;
		}
	      ch = 0xff & ret;
	      fprintf (f, "%c", ch);
	      fflush (f);
	    }
	  if (FD_ISSET (input->fd, &rfds))
	    {
	      do
		{
		  ret = read (input->fd, line, LINE_SIZE - 1);
		  if (ret > 0)
		    {
		      line[ret] = '\0';
		      fprintf (stdout, "%s\r\n", line);
		    }
		  fflush (stdout);
		}
	      while (ret > 0);
	    }
	}
    }
  while (1);

  tcsetattr (0, TCSANOW, &org);
  view_toggle_view_mode (input->view);
}

/**
 * @brief Set up a form for loading a file.
 *
 * Set up a form for selecting and loading a file to Main window.
 *
 * @todo Should this function be located in view?
 *
 * @param input The input.
 */
void
input_load_file (input * input)
{
  char *file = NULL;
  char *full = NULL;
  char buf[512];
  int size = 512;
  char *path = buf;
  int ret;

  ret = form_selection (&file, _("Select file"));
  if (ret == -2)
    {
      VLOG_ERR (input->view, _("Could not open file"));
      goto error;
    }
  else if (ret == -1)
    {
      goto error;
    }

  do
    {
      if (getcwd (path, size) == path)
	{
	  break;
	}
      else if (errno != ERANGE)
	{
	  VLOG_ERR (input->view, _("Failed to retrieve current directory"));
	  goto error;
	}
      if (path == buf)
	{
	  path = NULL;
	}
      size *= 2;
      path = (char *) realloc (path, size);
      LOG_ERR_IF_FATAL (path == NULL, "Memory");
    }
  while (1);

  size = strlen (file) + strlen (path) + 2;	/* '/' + '\0' */
  full = (char *) malloc (size);
  LOG_ERR_IF_FATAL (full == NULL, "Memory");
  sprintf (full, "%s/%s", path, file);

  view_show_file (input->view, full, 1, 0);

error:
  if (file != NULL)
    {
      free (file);
    }
  if (path != buf)
    {
      free (path);
    }
  if (full != NULL)
    {
      free (full);
    }
}

/**
 * @brief Parse the user input 'carrige return'
 *
 * Handles the different behaviours for the '\\r' command. The outcome
 * depends on which the current view window is.
 *
 * @param input The input object.
 *
 * @return 0 if the action was successful or if no action was taken. -1 if
 * the action failed.
 */
int
inputParseEnter (input * input)
{
  int win_type = -1;
  int tag;
  int ret = 0;

  assert (input);

  /*
   * Get information from view about which window that has focus and the
   * tag of the line under the cursor.
   */
  tag = view_get_tag (input->view, &win_type);

  switch (win_type)
    {
    case WIN_MAIN:
    case WIN_MESSAGES:
    case WIN_CONSOLE:
    case WIN_TARGET:
    case WIN_LOG:
    case WIN_RESPONSES:
    case WIN_BREAKPOINTS:
    case WIN_LIBRARIES:
    case WIN_FRAME:
      break;
    case WIN_STACK:
      ret = mi2_do_action (input->mi2, ACTION_STACK_LIST_VARIABLES, tag);
      break;
    case WIN_THREADS:
      ret = mi2_do_action (input->mi2, ACTION_THREAD_SELECT, tag);
      if (ret == 0)
	{
	  ret = mi2_do_action (input->mi2, ACTION_INT_UPDATE, 0);
	}
      break;
    default:
      LOG_ERR ("Could not retrieve window type");
      return -1;
    }

  return ret;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Create a input object.
 *
 * Creates a input object for handling the user input.
 *
 * @param view The view associated with the input object.
 * @param mi2 The mi2 object handling the communication to the debugger.
 * @param conf The user configuration.
 * @param fd File descripter to the debugger.
 *
 * @return A pointer to the new input object.
 */
input *
input_create (view * view, mi2_interface * mi2, configuration * conf, int fd)
{
  input *new_input;

  new_input = (input *) malloc (sizeof (*new_input));
  LOG_ERR_IF_FATAL (new_input == NULL, ERR_MSG_CREATE ("input"));

  DINFO (1, "Created input");

  new_input->view = view;
  new_input->mi2 = mi2;
  new_input->conf = conf;
  new_input->fd = fd;

  return new_input;
}

/**
 * @brief Free the input object.
 *
 * Free the input object.
 *
 * @param input The input object to be set free.
 */
void
input_free (input * input)
{
  assert (input);

  free (input);
}

/**
 * @brief Get user input.
 *
 * Get the user input and dispatch the input to either a mi2 interface or
 * a view object.
 *
 * The calling function has a select, so there should be at least one key to
 * read.
 *
 * @param input The input obejct to handle the user input.
 *
 * @return -1 if the user has pressed the quit key 'q'. Otherwise 0.
 */
int
input_get_input (input * input)
{
  int key[255];
  int ret = 0;
  int n;
  int i;
  int quit = 0;
  int cright[] = { 0x1b, 0x5b, 0x31, 0x3b, 0x35, 0x44 };
  int cleft[] = { 0x1b, 0x5b, 0x31, 0x3b, 0x35, 0x43 };

  assert (input);

  n = 0;
  do
    {
      key[n] = getch ();
      n++;
    }
  while (key[n - 1] != ERR);
  n--;

  if (n == 1)
    {
      switch (key[0])
	{
	case KEY_DOWN:
	  view_scroll_up (input->view);
	  break;
	case KEY_UP:
	  view_scroll_down (input->view);
	  break;
	case KEY_RIGHT:
	  view_next_window (input->view, 1, 0);
	  break;
	case KEY_LEFT:
	  view_next_window (input->view, -1, 0);
	  break;
	case '\t':
	  view_next_window (input->view, 1, 1);
	  break;
	case KEY_BTAB:
	  view_next_window (input->view, -1, 1);
	  break;
	case '\r':
	  inputParseEnter (input);
	  break;
	case 'b':
	  ret = mi2_do_action (input->mi2, ACTION_BP_SIMPLE, 0);
	  break;
	case 'B':
	  ret = mi2_do_action (input->mi2, ACTION_BP_ADVANCED, 0);
	  break;
	case 'w':
	  ret = mi2_do_action (input->mi2, ACTION_BP_WATCHPOINT, 0);
	  break;
	case KEY_F (3):
	  ret = mi2_do_action (input->mi2, ACTION_INT_START, 0);
	  break;
	case 'r':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_RUN, 0);
	  break;
	case 'c':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_CONT, 0);
	  break;
	case 'C':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_CONT_OPT, 0);
	  break;
	case 's':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_STEP, 0);
	  break;
	case 'S':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_STEPI, 0);
	  break;
	case 'n':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_NEXT, 0);
	  break;
	case 'N':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_NEXTI, 0);
	  break;
	case 'f':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_FINISH, 0);
	  break;
	case 'i':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_INTR, 0);
	  break;
	case 'I':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_INTR, 1);
	  break;
	case 'J':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_JUMP, 0);
	  break;
	case 'R':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_RETURN, 0);
	  break;
	case 'U':
	  ret = mi2_do_action (input->mi2, ACTION_EXEC_UNTIL, 0);
	  break;
	case KEY_F (4):
	  input_load_file (input);
	  break;
	case KEY_F (5):
	  ret = mi2_do_action (input->mi2, ACTION_FILE_LIST_EXEC_SORCES, 0);
	  break;
	case 'd':
	  mi2_toggle_disassemble (input->mi2);
	  mi2_do_action (input->mi2, ACTION_DATA_DISASSEMBLE, 0);
	  break;
	case 'm':
	  input_change_mode (input);
	  break;
	case 'q':
	  /* Quit the program. */
	  quit = -1;
	  break;
	default:
	  DINFO (3, "Unhandled input: %d - %c", key[0], key[0]);
	}
    }
  else
    {
      if (memcmp (cright, key, sizeof (cright)) == 0)
	{
	  view_next_window (input->view, 1, 2);
	}
      else if (memcmp (cleft, key, sizeof (cleft)) == 0)
	{
	  view_next_window (input->view, -1, 2);
	}
      else
	{
	  for (i = 0; i < n; i++)
	    {
	      DINFO (3, "Unhandled input %d/%d 0x%02X", i + 1, n, key[i]);
	    }
	}
    }

  return quit;
}
