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
 * @file pseudo_fork.c
 *
 * @brief Fork and start a program (the debugger) in a shell.
 *
 * Start a child process in a pseudo terminal.
 *
 */
#include <pty.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define FORK_SHELL "/bin/sh"
#define FORK_COMMAND "sh"
#define FORK_OPTION "-c"

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Start a program in a pseudo terminal.
 *
 * Starts a program in a pseudo terminal, so it is possible to both write
 * to the stdin and read the stdout from the started program.
 *
 * @todo Move to misc.
 *
 * @param fd The file descriptor to the started program.
 * @param cpid The pid of the started program.
 * @param debugger The program/debugger that should be started.
 * @param args Arguments to the debugger.
 *
 * @return 0 if the debugger was started, otherwise -1.
 *
 */
int
start_forkpty (int *fd, pid_t * cpid, char *debugger, char **args)
{
  char *argv[] = { FORK_COMMAND, FORK_OPTION, NULL, NULL };
  int i;
  int ret;
  char *debugger_cmd;
  int len;

  *cpid = forkpty (fd, NULL, NULL, NULL);
  if (*cpid == 0)
    {
      i = 0;
      while (args[i] != NULL)
	{
	  i++;
	}
      i = 0;
      len = strlen (debugger);
      while (args[i] != 0)
	{
	  len += strlen (args[i]) + 1;
	  i++;
	}
      debugger_cmd = (char *) malloc (len + 1);
      if (debugger_cmd == NULL)
	{
	  fprintf (stderr, "Could not create command");
	  exit (EXIT_FAILURE);
	}

      sprintf (debugger_cmd, "%s", debugger);
      i = 0;
      while (args[i] != 0)
	{
	  strcat (debugger_cmd, " ");
	  strcat (debugger_cmd, args[i]);
	  i++;
	}
      argv[2] = debugger_cmd;
      ret = execv (FORK_SHELL, argv);
      if (ret < 0)
	{
	  fprintf (stderr, "Could not call execv: %m");
	  exit (EXIT_FAILURE);
	}
      exit (EXIT_SUCCESS);
    }
  else if (*cpid < 0)
    {
      fprintf (stderr, "Could not fork: %m");
      return -1;
    }

  return 0;
}
