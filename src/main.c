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
 * @file
 * @brief
 *
 */
#include <argp.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <fcntl.h>

#include "lvdbg.h"
#include "view.h"
#include "pseudo_fork.h"
#include "configuration.h"
#include "input.h"
#include "mi2_interface.h"
#include "misc.h"
#include "debug.h"
#include "lvdbg.h"
#include "win_form.h"

#define LINE_LEN 8192
#define GDB_DBG "gdb --fullname --interpreter=mi2"

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
static error_t parseOpt (int key, char *arg, struct argp_state *state);
static int mainLoop (int fd, view * view, input * input, mi2_interface * mi2,
		     configuration * conf);
static int setupconf (configuration * conf);

/*******************************************************************************
 * Internal structures and enums
 ******************************************************************************/
struct arguments
{
  char *debugger;
  char **options;
  int verbose;
  int verbose_level;
  int quiet;
  char *output_file;
  char *conf_file;
};

/*******************************************************************************
 * Global variables
 ******************************************************************************/
const char *argp_program_version = LVDBG_VERSION;
const char *argp_program_bug_address = LVDBG_BUG_ADDRESS;

static char doc[] =
  _("lvdbg -- a simple front end debugger."
  "\vA simple gui for debuggers.");
static char args_doc[] = "gdb [DEBUGGER ARGS]";

static struct argp_option options[] = {
  {"verbose", 'v', 0, 0, "Produce verbose output"},
  {"quiet", 'q', 0, 0, "Don't produce any output"},
  {"output", 'o', "FILE", 0, "Output to FILE"},
  {"conf", 'c', "FILE", 0, "Use configure FILE"},
  {"verbose_level", 'L', "LEVEL", OPTION_HIDDEN,
   "Verbose level is set to LEVEL"},
  {0},
};

static struct argp argp = { options, parseOpt, args_doc, doc };

static const conf_parameter root_group[] = {
  {"auto frames", PARAM_BOOL, 0, 0, {.bool_value = 0}},
  {NULL},
};

static const conf_parameter output_group[] = {
  {"height", PARAM_UINT, 0, 100, {.uint_value = 5}},
  {"layout", PARAM_STRING, 0, 0, {.string_value = ""}},
  {NULL},
};

static const conf_parameter syntax_group[] = {
  {"enabled", PARAM_BOOL, 0, 0, {.bool_value = 1}},
  {"colors", PARAM_STRING, 0, 0, {.string_value = ""}},
  {"attr", PARAM_STRING, 0, 0, {.string_value = ""}},
  {"groups", PARAM_STRING, 0, 0, {.string_value = ""}},
  {NULL},
};

int VERBOSE_LEVEL = 5;
FILE *OUT_FILE = NULL;

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
static int
mainLoop (int fd, view * view, input * input, mi2_interface * mi2,
	  configuration * conf)
{
  fd_set rfds;
  int retval;
  char line[LINE_LEN];
  FILE *dbg_file = NULL;
  char *p;
  int flags;
  int ret;

  dbg_file = fdopen (fd, "r+");

  flags = fcntl (fd, F_GETFL);
  if (flags == -1)
    {
      LOG_ERR ("Could not get status flags: %m");
      ret = -1;
      goto error;
    }
  retval = fcntl (fd, F_SETFL, flags | O_NONBLOCK);
  if (retval == -1)
    {
      LOG_ERR ("Could not set file to the debugger to non blocking: %m");
      ret = -1;
      goto error;
    }
  while (1)
    {
      FD_ZERO (&rfds);
      FD_SET (0, &rfds);
      FD_SET (fd, &rfds);

      retval = select (fd + 1, &rfds, NULL, NULL, NULL);
      if (retval == -1)
	{
	  perror ("select()");
	}
      else if (retval)
	{
	  if (FD_ISSET (0, &rfds))
	    {
	      ret = input_get_input (input);
	      if (ret == -1)
		{
		  ret = 0;
		  goto error;
		}
	    }
	  if (FD_ISSET (fd, &rfds))
	    {
	      do
		{
		  p = fgets (line, LINE_LEN - 1, dbg_file);
		  if (p == NULL)
		    {
		      break;
		    }
		  if (line[strlen (line) - 1] == '\n')
		    {
		      line[strlen (line) - 1] = '\0';
		    }
		  switch (*line)
		    {
		    case '~':
		      ret = unescape (line + 1, "\r\n\v");
		      if (ret == 0)
			{
			  view_add_line (view, WIN_CONSOLE, line + 1, -1);
			}
		      break;
		    case '@':
		      ret = unescape (line + 1, "\r\n\v");
		      if (ret == 0)
			{
			  view_add_line (view, WIN_TARGET, line + 1, -1);
			}
		      break;
		    case '&':
		      ret = unescape (line + 1, "\r\n\v");
		      if (ret == 0)
			{
			  view_add_line (view, WIN_LOG, line + 1, -1);
			}
		      break;
		    case '^':	/* MI */
		    case '*':	/* Async records. */
		    case '=':	/* Asyc records. */
		      ret = unescape (line + 1, "\r\n\v");
		      if (ret == 0)
			{
			  view_add_line (view, WIN_RESPONSES, line, -1);
			  mi2_parse (mi2, line);
			}
		      break;
		    case '(':
		      if (strncmp (line, "(gdb)", 5) == 0)
			{
			  continue;
			}
		      /* Fall through */
		    default:
		      view_add_line (view, WIN_TARGET, line, -1);
		      LOG_ERR ("Unknown stream record: '%s'", line);
		    }
		}
	      while (1);
	    }
	}
    }
  return 0;
error:
  if (dbg_file != NULL)
    {
      fclose (dbg_file);
    }
  return ret;
}

static error_t
parseOpt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'q':
      arguments->quiet = 1;
      break;
    case 'v':
      arguments->verbose = 1;
      break;
    case 'o':
      arguments->output_file = arg;
      break;
    case 'c':
      arguments->conf_file = arg;
      break;
    case 'L':
      arguments->verbose_level = atoi (arg);
      break;
    case ARGP_KEY_NO_ARGS:
      argp_usage (state);
      break;
    case ARGP_KEY_ARG:
      arguments->debugger = arg;
      arguments->options = &state->argv[state->next];
      state->next = state->argc;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static int
setupconf (configuration * conf)
{
  int ret;

  ret = conf_add_group (conf, NULL, root_group);
  DINFO (1, "Added root %d", ret);
  if (ret < 0)
    {
      return ret;
    }

  ret = conf_add_group (conf, "Output Window", output_group);
  if (ret < 0)
    {
      return ret;
    }

  ret = conf_add_group (conf, "Syntax", syntax_group);
  if (ret < 0)
    {
      return ret;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  struct arguments arguments;
  int i;
  pid_t cpid;
  int fd;
  view *view = NULL;
  mi2_interface *mi2 = NULL;
  int ret;
  configuration *conf;
  input *input;
  char *home;
  char *local_conf;

  arguments.quiet = 0;
  arguments.verbose = 0;
  arguments.verbose_level = 1;

  arguments.output_file = "-";
  arguments.conf_file = NULL;

  argp_parse (&argp, argc, argv, 0, 0, &arguments);

  LOG_START (arguments.output_file);
  if (arguments.quiet)
    {
      VERBOSE_LEVEL = -1;
    }
  else
    {
      if (arguments.verbose && arguments.verbose_level == 1)
	{
	  VERBOSE_LEVEL = 3;
	}
      else
	{
	  VERBOSE_LEVEL = arguments.verbose_level;
	}
    }

  /* Add '--interpreter=mi2' for gdb. */
  if (strcmp (arguments.debugger, "gdb") == 0)
    {
      arguments.debugger = GDB_DBG;
    }

  LOG_ERR ("Silent: %d", arguments.quiet);
  DINFO (1, "Silent: %d", arguments.quiet);
  DINFO (1, "Verbose: %d", arguments.verbose);
  DINFO (1, "Output: %s", arguments.output_file);
  DINFO (1, "Debugger: %s", arguments.debugger);

  for (i = 0; arguments.options[i]; i++)
    {
      DINFO (1, "Option: %s", arguments.options[i]);
    }

  if (start_forkpty (&fd, &cpid, arguments.debugger, arguments.options) < 0)
    {
      LOG_ERR ("Could not start debugger.");
      LOG_END;
      exit (EXIT_FAILURE);
    }

  /* Load configuration. */
  conf = conf_create ();
  ret = setupconf (conf);
  if (ret < 0)
    {
      LOG_ERR ("Could not setup config parameters.");
    }
  ret = conf_load (conf, "/etc/.lvdbg.conf");
  if (ret < 0)
    {
      LOG_ERR ("Could not load config at line %d", -ret);
    }
  home = getenv ("HOME");
  local_conf = (char *) malloc (strlen (home) + strlen ("/.lvdbg.conf") + 1);
  sprintf (local_conf, "%s/.lvdbg.conf", home);
  ret = conf_load (conf, local_conf);
  free (local_conf);
  if (ret < 0)
    {
      LOG_ERR ("Could not load config at line %d", -ret);
    }
  ret = conf_load (conf, arguments.conf_file);
  if (ret < 0)
    {
      LOG_ERR ("Could not load config at line %d", -ret);
    }

  ret = view_setup (&view, conf);
  if (ret < 0)
    {
      LOG_ERR ("Could not set up screen.");
      LOG_END;
      exit (EXIT_FAILURE);
    }

  mi2 = mi2_create (fd, cpid, view, conf);
  if (mi2 == NULL)
    {
      LOG_ERR ("Could not set up mi2 interface");
      exit (EXIT_FAILURE);
    }

  input = input_create (view, mi2, conf, fd);
  if (input == NULL)
    {
      LOG_ERR ("Could not set up input module.");
      LOG_END;
      exit (EXIT_FAILURE);
    }

  ret = mainLoop (fd, view, input, mi2, conf);
  DINFO (1, "Program exits with %d", ret);
  view_cleanup (view);
  mi2_free (mi2);
  input_free (input);
  conf_free (conf);

  LOG_END;
  return 0;
}
