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
* @file view.c
*
* @brief Implements the view obejct.
*
* Implements the view object, which the user is interacting with. The view
* contains a number of windows for presentation of the debbuging
* information.
*
*/
#include <ncurses.h>
#include <panel.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "view.h"
#include "lvdbg.h"
#include "configuration.h"
#include "text.h"
#include "win_handler.h"
#include "debug.h"
#include "objects.h"
#include "misc.h"
#include "lvdbg.h"

#define LAST_WINDOW WIN_REGISTERS

/*******************************************************************************
* Internal structures and enums
******************************************************************************/

/**
* @brief Group
*
* Order each window to a group of windows. It is possible to loop all
* windows within a group, and loop all groups. All windows in a group has the
* same size and position, on top of each other.
*/
typedef struct group_t
{
  int nr_of_groups; /**< Number of groups. */
  int nr_of_wins_in_group[LAST_WINDOW + 1]; /**< Nr of wins in each group. */
  int current_group; /**< The current group. */
  int current_win[LAST_WINDOW + 1]; /**< The current window in each group. */
  int groups[LAST_WINDOW + 1][LAST_WINDOW + 1];	/**< All groups. */
} group;

/** Structure representing the view. */
struct view_t
{
  Win *windows[LAST_WINDOW + 1]; /**< An array of the available windows. */
  Win *current_window; /**< The current window with focus. */
  int current_index;   /**< The current index of the window with focus. */
  int last_stop_mark;  /**<
                        * The last line the gdb stopped at. Kept for unmarking
                        * the line.
                        */
  group groups[LAST_WINDOW + 1]; /**< All groups. */
  unsigned int views; /**< Nr of views. */
  unsigned int current_view; /**< Index of current view. */

  win_attribute win_attr[256]; /**< The defined attributes for highlighting. */
  int color_pairs;  /**< Defined colors for highlighting. */
  int nr_of_attributes; /**< Nr of attributes used. */

  int view_mode; /**<
		  * If set to 0, normal ncurse mode. 1 the ncurse is suspended.
                  */
};

/**
 * @brief Structur of all available windows.
 *
 * A structure used for defining available windows and their properties.
 */
struct view_windows
{
  char *name; /**< The name of the window. */
  win_properties props;	/**< Windows properties. */
};

/*******************************************************************************
 * Global variables
 ******************************************************************************/
/**
 * A variable to represent all available windows. Must be the same order as in
 * #window_type.
 */
static struct view_windows out_windows[] = {
  {_("Main"), {3, WIN_PROP_CURSOR | WIN_PROP_MARKS | WIN_PROP_SYNTAX, NULL, 0,
	       NULL}},
  {_("Messages"), {0, 0, NULL, 0, NULL}},
  {_("Console"), {0, 0, NULL, 0, NULL}},
  {_("Target"), {0, 0, NULL, 0, NULL}},
  {_("Log"), {0, 0, NULL, 0, NULL}},
  {_("Responses"), {0, 0, NULL, 0, NULL}},
  {_("Breakpoints"), {0, WIN_PROP_CURSOR, NULL, 0, NULL}},
  {_("Threads"), {0, WIN_PROP_CURSOR, NULL, 0, NULL}},
  {_("Libraries"), {0, WIN_PROP_CURSOR, NULL, 0, NULL}},
  {_("Stack"), {0, WIN_PROP_CURSOR, NULL, 0, NULL}},
  {_("Frame"), {0, WIN_PROP_CURSOR, NULL, 0, NULL}},
  {_("Disassemble"), {1, WIN_PROP_CURSOR | WIN_PROP_MARKS, NULL, 0, NULL}},
  {_("Registers"), {0, WIN_PROP_CURSOR, NULL, 0, NULL}},
};

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
void view_setup_colors (view * view, configuration * conf);
int viewParseLayout (view * view, char *layout, int starty, int startx,
		     int height, int width, int *used_height,
		     int *used_width);

/** @brief Set up the syntax color.
 *
 * Set up the colors and the syntax highlighting.
 *
 * The highlighting has three parts: color, attr and syntax. The color defines
 * the available colors. The @a attr defines the pair color and an attribute,
 * which is the same attribute from ncurse library. The syntax defines which
 * words or part of a text that should have highlighting and the corresponding
 * @a attr. The syntax is parsed by the win object. The @a attr and @a color
 * is common for all windows.
 *
 * @param view The view.
 * @param conf The configuration holding the syntax and color.
 */
void
view_setup_colors (view * view, configuration * conf)
{
  int ret;
  char *next;
  char *name;
  char *value;
  char *inext;
  char *iname;
  char *ivalue;
  char *endptr;
  char *attr = NULL;
  char *colors = NULL;
  char *groups = NULL;
  int count = 0;
  int i;
  int bg_color = 0;
  int fg_color = 0;
  const char *conf_colors = conf_get_string (conf, "Syntax", "colors", NULL);

  if (conf_colors == NULL || strlen (conf_colors) == 0)
    {
      return;
    }
  /* Parse syntax' attr group */
  colors = strdup (conf_get_string (conf, "Syntax", "colors", NULL));
  LOG_ERR_IF_RETURN (colors == NULL,, _("Failed to create syntax"));
  count = 1;
  next = colors;
  while (next != NULL && *next)
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret != '{' || name != NULL || value == NULL)
	{
	  LOG_ERR (_("Failed to parse configuration's 'colors'"));
	  goto error;
	}
      inext = value;
      while (inext != NULL && *inext)
	{
	  ret = get_next_param (inext, &iname, &ivalue, &inext);
	  if (ret < 0 || iname == NULL || ivalue == NULL)
	    {
	      LOG_ERR (_("Failed to parse configuration's 'colors' '%s' "
			 "'%s'"), iname, ivalue);
	      goto error;
	    }
	  if (strcmp (iname, "bg_color") == 0)
	    {
	      bg_color = strtol (ivalue, &endptr, 0);
	      if (endptr == ivalue)
		{
		  LOG_ERR (_("Parse error: not a number '%s'"), ivalue);
		  goto error;
		}
	    }
	  else if (strcmp (iname, "fg_color") == 0)
	    {
	      fg_color = strtol (ivalue, &endptr, 0);
	      if (endptr == ivalue)
		{
		  LOG_ERR (_("Parse error: not a number '%s'"), ivalue);
		  goto error;
		}
	    }
	  else
	    {
	      LOG_ERR (_("Parse error: unknown parameter '%s'"), iname);
	      goto error;
	    }
	}
      DINFO (1, "Init pair %d fg: %d bg: %d", count, fg_color, bg_color);
      init_pair (count, fg_color, bg_color);
      count++;
      if (count == COLOR_PAIRS)
	{
	  LOG_ERR (_("Max %d color pairs"), COLOR_PAIRS);
	  break;
	}
    }
  view->color_pairs = count;

  /* Parse syntax' attr group */
  if (conf_get_string (conf, "Syntax", "attr", NULL))
    {
      attr = strdup (conf_get_string (conf, "Syntax", "attr", NULL));
      if (attr == NULL)
	{
	  LOG_ERR ("Failed to create syntax");
	  goto error;
	}
    }
  else
    {
      goto error;
    }
  count = 0;
  next = attr;
  while (next != NULL && *next)
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret != '{' || name != NULL || value == NULL)
	{
	  LOG_ERR (_("Failed to parse configuration's 'attr'"));
	  goto error;
	}
      inext = value;
      while (inext != NULL && *inext)
	{
	  ret = get_next_param (inext, &iname, &ivalue, &inext);
	  if (ret < 0 || iname == NULL || ivalue == NULL)
	    {
	      LOG_ERR (_("Failed to parse configuration's 'colors'"));
	      goto error;
	    }
	  if (strcmp (iname, "color") == 0)
	    {
	      view->win_attr[count].color = strtol (ivalue, &endptr, 0);
	      if (endptr == ivalue)
		{
		  LOG_ERR (_("Parse error: not a number '%s'"), ivalue);
		  goto error;
		}
	      if (view->win_attr[count].color - 1 >= view->color_pairs)
		{
		  LOG_ERR (_("Parse error: invalid color '%s'"), ivalue);
		  goto error;
		}
	    }
	  else if (strcmp (iname, "attr") == 0)
	    {
	      view->win_attr[count].attr = strtol (ivalue, &endptr, 0);
	      if (endptr == ivalue)
		{
		  LOG_ERR (_("Parse error: not a number '%s'"), ivalue);
		  goto error;
		}
	    }
	  else
	    {
	      LOG_ERR (_("Parse error: unknown parameter '%s'"), iname);
	      goto error;
	    }
	}
      DINFO (1, "Added attr %d Color %d attr %d", count,
	     view->win_attr[count].color, view->win_attr[count].attr);
      count++;
      if (count == 256)
	{
	  LOG_ERR (_("Max 256 attributes"));
	  break;
	}
    }
  view->nr_of_attributes = count;

  /* Parse syntax' groups */
  if (conf_get_string (conf, "Syntax", "groups", NULL) == NULL)
    {
      goto error;
    }
  groups = strdup (conf_get_string (conf, "Syntax", "groups", NULL));
  if (groups == NULL)
    {
      LOG_ERR (_("Failed to create syntax"));
      goto error;
    }

  next = groups;
  while (next != NULL && *next)
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret < 0 || name == NULL || value == NULL)
	{
	  LOG_ERR (_("Failed to parse configuration's 'group'"));
	  goto error;
	}
      for (i = 0; i <= LAST_WINDOW; i++)
	{
	  if (strcasecmp (out_windows[i].name, name) == 0)
	    {
	      break;
	    }
	}
      if (i > LAST_WINDOW)
	{
	  LOG_ERR (_("No window called '%s'"), name);
	  goto error;
	}
      out_windows[i].props.scan_definitions = strdup (value);
      out_windows[i].props.attributes = view->win_attr;
      out_windows[i].props.nr_of_attributes = view->nr_of_attributes;
      DINFO (1, "Groups in window '%s' '%s'", name, value);
    }

error:
  if (attr != NULL)
    {
      free (attr);
    }
  if (colors != NULL)
    {
      free (colors);
    }
  if (groups != NULL)
    {
      free (groups);
    }
}

/**
 * @brief Parse and set up the windows in the view.
 *
 * Parses a configuration variable and sets up the windows specified in the
 * variable. The function calls itself recursively.
 *
 * @code
 * {...}, {...}, {...}
 * @endcode
 *
 * Represents different rows in the view.
 *
 * @code
 * [...], [...], [...]
 * @endcode
 * Represents different columns in the view.
 *
 * To set the height of a row its possible to set the \a height in percent of the
 * max height. Or how many \a rows that should split the height.
 *
 * @code
 * [height="75",name="Main"]
 * @endcode
 *
 * @code
 * {rows="4",name="frames"}
 * @endcode
 *
 * It is analogous for the width; \a width and \a cols.
 *
 * It is also possible to set up different views, e.g.
 *
 * @code
 * view={[width=50, name='Main'...]},view={{'height=50,...}}
 * @endcode
 *
 * @todo A more simple way of setting the layout.
 *
 * @param view The view.
 * @param layout The layout parameter.
 * @param starty The start y position of the window.
 * @param startx The start x position of the window.
 * @param height The max height to be used.
 * @param width The max width to be used.
 * @param used_height Returns actual height used.
 * @param used_width Returns actual width used.
 *
 * @return 0 if the view was set up. -1 on failure.
 */
int
viewParseLayout (view * view, char *layout, int starty, int startx,
		 int height, int width, int *used_height, int *used_width)
{
  char *next;
  char *name;
  char *value;
  int ret;
  char *endptr;
  int h = height;
  int w = width;
  int uh = 0;
  int uw = 0;
  int dh = 0;
  int dw = 0;
  int i;
  int val;
  int new_group = 1;
  int gr = 0;
  int ind = 0;

  assert (view);

  next = layout;
  while (next != NULL && *next != 0)
    {
      ret = get_next_param (next, &name, &value, &next);
      LOG_ERR_IF_RETURN (ret < 0, -1, "Could not parse layout '%s'", layout);
      switch (ret)
	{
	case '{':
	  if (name && strcmp (name, "view") == 0)
	    {
	      ret = viewParseLayout (view, value, starty, startx,
				     height, width, used_height, used_width);
	      DINFO (1, "View %d = %d %dx%d", view->views, ret, *used_height,
		     *used_width);
	      *used_height = 0;
	      *used_width = 0;
	      if (ret < 0)
		{
		  return ret;
		}
	      view->views++;
	      LOG_ERR_IF_RETURN (view->views > LAST_WINDOW, -1,
				 "Too many views");
	      break;
	    }
	  /* A new row. */
	  new_group = 1;
	  ret = viewParseLayout (view, value, starty + uh, startx,
				 height - uh, w, &dh, &dw);
	  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not parse '%s'", value);
	  DINFO (3, "Row used width %d/%d", dw, width);
	  DINFO (3, "Row used height %d/%d", dh, height);
	  uh += dh;
	  if (dw > uw)
	    {
	      uw = dw;
	    }
	  *used_height = uh;
	  *used_width = uw;
	  break;
	case '[':
	  new_group = 1;
	  ret = viewParseLayout (view, value, starty, startx + uw,
				 h, width - uw, &dh, &dw);
	  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not parse '%s'", value);
	  DINFO (3, "Used width %d/%d", dw, width);
	  DINFO (3, "Used height %d/%d", dh, height);
	  uw += dw;
	  if (dh > uh)
	    {
	      uh = dh;
	    }
	  *used_height = uh;
	  *used_width = uw;
	  break;
	case '\'':
	case '"':
	  LOG_ERR_IF_RETURN (name == NULL, -1, "Could not pars.");
	  if (strcmp (name, "name") == 0)
	    {
	      i = 0;
	      while (i <= LAST_WINDOW
		     && strcasecmp (out_windows[i].name, value) != 0)
		{
		  i++;
		}
	      LOG_ERR_IF_RETURN (i > LAST_WINDOW, -1, "Unknown window '%s'",
				 value);
	      if (view->windows[i] == NULL)
		{
		  view->windows[i] =
		    win_create (starty, startx, h, w, &out_windows[i].props);
		}
	      if (new_group)
		{
		  new_group = 0;
		  gr = view->groups[view->views].nr_of_groups++;
		  ind = 0;
		}
	      view->groups[view->views].nr_of_wins_in_group[gr]++;
	      view->groups[view->views].groups[gr][ind++] = i;
	      if (i == WIN_MESSAGES)
		{
		  view->groups[view->views].current_win[gr] = ind - 1;
		}
	      DINFO (1, "Created '%s' as window nr %d in group %d index %d "
		     "in view nr %d",
		     out_windows[i].name, i, gr, ind - 1, view->views);

	      win_set_status (view->windows[i], out_windows[i].name);
	      LOG_ERR_IF_RETURN (view->windows[i] == NULL, -1,
				 "Could not create '%s'-window",
				 out_windows[i].name);
	      *used_height = h;
	      *used_width = w;
	    }
	  else if (strcmp (name, "height") == 0)
	    {
	      val = strtol (value, &endptr, 0);
	      LOG_ERR_IF_RETURN (endptr == value, -1, "Not a value %s",
				 value);
	      h = (1.0f * val * height) / 100.0f + 0.5f;
	    }
	  else if (strcmp (name, "rows") == 0)
	    {
	      val = strtol (value, &endptr, 0);
	      LOG_ERR_IF_RETURN (endptr == value, -1, "Not a value %s",
				 value);
	      h = (1.0f * height) / val + 0.5f;
	    }
	  else if (strcmp (name, "width") == 0)
	    {
	      val = strtol (value, &endptr, 0);
	      LOG_ERR_IF_RETURN (endptr == value, -1, "Not a value %s",
				 value);
	      w = (1.0f * val * width) / 100.0f + 0.5f;
	    }
	  else if (strcmp (name, "cols") == 0)
	    {
	      val = strtol (value, &endptr, 0);
	      LOG_ERR_IF_RETURN (endptr == value, -1, "Not a value %s",
				 value);
	      w = (1.0f * width) / val + 0.5f;
	    }
	  else
	    {
	      LOG_ERR ("Unknown paramter name '%s'", name);
	      return -1;
	    }
	  break;
	default:
	  LOG_ERR ("Strange type '%c'", ret);
	  return -1;
	}
    }
  return 0;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/
/**
 * @brief Setup the view.
*
* Sets up the screen and initiates all output windows. The resulting view is
* strored in @a v.
*
* @param v Structure to hold the new view information.
* @param conf The configuration for the debugger.
*
* @return 0 if successful. -1 if failed to set up the screen.
*/
int
view_setup (view ** v, configuration * conf)
{
  int height;
  int width;
  int starty;
  int startx;
  int max_width;
  int max_height;
  int used_width;
  int used_height;
  int ret;
  const char *layout_conf;
  char *layout;
  const char **p;
  const char *copy[] = {
      "Copyright (C) 2012 Kenneth Olsson.",
      "License GPLv3+: GNU GPL version 3 or later ",
      "<http://gnu.org/licenses/gpl.html>",
      "This is free software: you are free to change and redistribute it.",
      "There is NO WARRANTY, to the extent permitted by law.",
      NULL ,
  };
  char default_layout[] =
    "view={{height='75',[width='50',name='Main'],"
    "[{rows='5',name='threads'},{rows='4',name='breakpoints'},"
    "{rows='3', name='Libraries'},"
    "{rows='2', name='stack'},"
    "{name='frame'}]},"
    "{name='Console',name='Target',name='Log',name='Responses',"
    "name='Messages'}},"
    "view={[width='50',name='Disassemble'],[name='Registers']}";

  assert (v);
  *v = NULL;

  DINFO (1, "Initializing screen");

  if (initscr () == NULL)
    {
      LOG_ERR ("Could not initialize screen");
      return -1;
    }

  /* Set options for the screen. */
  cbreak ();
  raw ();
  noecho ();
  keypad (stdscr, TRUE);
  nonl ();
  nodelay (stdscr, TRUE);
  curs_set (0);
  start_color ();

  getmaxyx (stdscr, max_height, max_width);
  leaveok (stdscr, TRUE);

  layout_conf = conf_get_string (conf, "Output Window", "layout", NULL);
  if (layout_conf && strlen (layout_conf) > 0)
    {
      layout = strdup (layout_conf);
      LOG_ERR_IF_FATAL (layout == NULL, ERR_MSG_CREATE ("layout"));
    }
  else
    {
      layout = default_layout;
    }
  *v = (view *) malloc (sizeof **v);
  LOG_ERR_IF_FATAL (*v == NULL, ERR_MSG_CREATE ("view"));
  memset (*v, 0, sizeof (**v));

  /* Set up colors and syntax. */
  view_setup_colors (*v, conf);

  starty = 0;
  startx = 0;
  height = max_height;
  width = max_width;
  ret = viewParseLayout (*v, layout, starty,
			 startx, max_height - 1, max_width, &used_height,
			 &used_width);
  if (layout != default_layout)
    {
      free (layout);
    }
  if (ret < 0)
    {
      goto error;
    }
  if ((*v)->views == 0)
    {
      (*v)->views = 1;
    }

  (*v)->current_window = (*v)->windows[0];
  (*v)->current_index = 0;
  (*v)->view_mode = 0;
  win_to_top ((*v)->current_window);
  win_set_focus ((*v)->current_window, 1);
  (*v)->last_stop_mark = -1;

  win_to_top ((*v)->windows[WIN_MESSAGES]);
  view_next_window (*v, -1, 2);
  view_next_window (*v, 1, 2);

  update_panels ();
  doupdate ();
  DINFO (1, "Screen init done");

  /* copyright info */
  p = copy;
  while (*p && (*v)->windows[WIN_MESSAGES])
    {
      win_add_line ((*v)->windows[WIN_MESSAGES], *p, 1, 0);
      p++;
    }

  return 0;

error:
  LOG_ERR ("Failed to initialize the screen");
  if (*v != NULL)
    {
      view_cleanup (*v);
    }

  fprintf (stderr, "Failed to initialize the screen.\n");

  return -1;
}

/**
 * @brief Release a view.
 *
 * Free a view and it's allocated resources. Also restores the screen.
 *
 * @param view The view to be set free.
 */
void
view_cleanup (view * view)
{
  int i;

  assert (view);

  DINFO (1, "Freeing view");

  for (i = 0; i <= LAST_WINDOW; i++)
    {
      if (view->windows[i] != NULL)
	{
	  win_free (view->windows[i]);
	}
      if (out_windows[i].props.scan_definitions != NULL)
	{
	  free (out_windows[i].props.scan_definitions);
	  out_windows[i].props.scan_definitions = NULL;
	}
    }
  memset (view, 0, sizeof (*view));
  free (view);
  endwin ();
}

/**
 * @brief Output to message window.
 *
 * Output a message out the message window.
 *
 * @param view The view.
 * @param level The leve. (Currently not used)
 * @param msg The format of the message.
 * @param ... The message arguments.
 *
 * @return 0 upon success otherwise -1.
 */
int
view_add_message (view * view, int level, const char *msg, ...)
{
  int ret;
  va_list ap;
  char buf[512];
  char *p = buf;
  char *t;
  int size = 512;

  assert (view);

  if (view->windows[WIN_MESSAGES] == NULL)
    {
      return 0;
    }
  while (1)
    {
      va_start (ap, msg);
      ret = vsnprintf (p, size, msg, ap);
      va_end (ap);
      if (ret > -1 && ret < size)
	{
	  break;
	}
      if (ret > -1)
	{
	  size = ret + 1;
	}
      else
	{
	  size *= 2;
	}
      if (p == buf)
	{
	  p = NULL;
	}
      t = (char *) realloc (p, size);
      LOG_ERR_IF_FATAL (t == NULL, "Memory");
      p = t;
    }

  ret = win_add_line (view->windows[WIN_MESSAGES], buf, 1, 0);

  update_panels ();
  doupdate ();

  if (p != buf)
    {
      free (p);
    }
  return ret;
}

/**
 * @brief Add a line of text.
 *
 * Adds a line of text to a specified window.
 *
 * @todo Why call the function view_add_line? viewAddLine!?
 *
 * @param view The view object.
 * @param type The type of window that the line of text should be added to.
 *             WIN_MAIN, WIN_CONSTOL,...WIN_FRAME are valid types.
 * @param line The line to be added.
 * @param tag The tag that is associated to the line.
 *
 * @return 0 if the line was added otherwise -1.
 */
int
view_add_line (view * view, int type, const char *line, int tag)
{
  int ret;

  LOG_ERR_IF_RETURN (type < 0
		     || type > LAST_WINDOW, -1, "Wrong type %d", type);
  ret = win_add_line (view->windows[type], line, 1, tag);

  update_panels ();
  doupdate ();

  return ret;
}

/**
 * @brief Update the breakpoint view.
 *
 * Updates the breakpoint view with new information.
 *
 * @param view The view.
 * @param bpt The breakpoint table containing the new set of breakpoints.
 */
void
view_update_breakpoints (view * view, breakpoint_table * bpt)
{
  int i;
  char buf[128];
  char *line = buf;
  int size = 128;
  char mark[3];
  const char *main_file_name;

  assert (view);
  assert (bpt);

  win_clear (view->windows[WIN_BREAKPOINTS]);
  for (i = 0; i < bpt->rows; i++)
    {
      /* Update breakpoints window. */
      if (bpt->breakpoints[i] == NULL)
	{
	  continue;
	}
      if (bpt->breakpoints[i]->type == BP_TYPE_WATCHPOINT)
	{
	  LPRINT (line, line != buf, size, "%2d Watchpoint %s = %s",
		  bpt->breakpoints[i]->number,
		  bpt->breakpoints[i]->expression,
		  bpt->breakpoints[i]->value ? bpt->breakpoints[i]->value :
		  "[ NaN ]");
	  win_add_line (view->windows[WIN_BREAKPOINTS], line, 1,
			bpt->breakpoints[i]->number);
	  continue;
	}
      LPRINT (line, line != buf, size, "%2d %c%c%c 0x%08X %.20s %3d %3d "
	      "%.20s:%-4d %s",
	      bpt->breakpoints[i]->number,
	      bpt->breakpoints[i]->type ? 'w' : 'b',
	      bpt->breakpoints[i]->disp ? 'k' : 'd',
	      bpt->breakpoints[i]->enabled ? 'e' : 'd',
	      bpt->breakpoints[i]->addr,
	      bpt->breakpoints[i]->func,
	      bpt->breakpoints[i]->ignore,
	      bpt->breakpoints[i]->thread,
	      bpt->breakpoints[i]->file,
	      bpt->breakpoints[i]->line,
	      bpt->breakpoints[i]->cond ? bpt->breakpoints[i]->cond : "");
      win_add_line (view->windows[WIN_BREAKPOINTS], line, 1,
		    bpt->breakpoints[i]->number);

      /* Set mark in window. */
      mark[0] = bpt->breakpoints[i]->disp ? 'B' : 'b';
      mark[1] = bpt->breakpoints[i]->enabled ? 'e' : 'd';
      mark[2] = '\0';
      main_file_name = win_get_filename (view->windows[WIN_MAIN]);
      if (main_file_name != NULL && bpt->breakpoints[i]->fullname != NULL
	  && strcmp (main_file_name, bpt->breakpoints[i]->fullname) == 0)
	{
	  win_set_mark (view->windows[WIN_MAIN], bpt->breakpoints[i]->line, 0,
			mark[0]);
	  win_set_mark (view->windows[WIN_MAIN], bpt->breakpoints[i]->line, 1,
			mark[1]);
	}
    }

  if (line != buf)
    {
      free (line);
    }

  update_panels ();
  doupdate ();

  return;
}

/**
 * @brief Update the thread view.
 *
 * Updates the thread view with new information.
 *
 * @param view The view.
 * @param thread_groups The new list of threads.
 */
void
view_update_threads (view * view, thread_group * thread_groups)
{
  char line[80];
  char *pl = line;
  char file[21];
  int size = 80;
  int len;

  thread_group *pg;
  thread *pt;

  assert (view);

  win_clear (view->windows[WIN_THREADS]);

  pg = thread_groups;
  while (pg != NULL)
    {
      LPRINT (pl, pl != line, size, "thread group #%d", pg->id);
      win_add_line (view->windows[WIN_THREADS], line, 1, -pg->id);
      pt = pg->first;
      while (pt != NULL)
	{
	  if (pt->frame.fullname != NULL)
	    {
	      len = strlen (pt->frame.fullname);
	      if (len > 20)
		{
		  snprintf (file, 21, "...%s", pt->frame.fullname + len - 17);
		}
	      else
		{
		  snprintf (file, 21, "%s", pt->frame.fullname);
		}
	    }
	  else if (pt->frame.file != NULL)
	    {
	      len = strlen (pt->frame.file);
	      if (len > 20)
		{
		  snprintf (file, 21, "...%s", pt->frame.file + len - 17);
		}
	      else
		{
		  snprintf (file, 21, "%s", pt->frame.file);
		}

	    }
	  else
	    {
	      file[0] = '\0';
	    }

	  LPRINT (pl, pl != line, size, " #%2d %c %.21s %s", pt->id,
		  pt->running ? 'R' : 'S', file,
		  pt->frame.func ? pt->frame.func : "");
	  win_add_line (view->windows[WIN_THREADS], line, 1, pt->id);
	  pt = pt->next;
	}
      pg = pg->next;
    }

  win_go_to_line (view->windows[WIN_THREADS], 0);

  if (pl != line)
    {
      free (pl);
    }

  update_panels ();
  doupdate ();
}

/**
 * @brief Update the library view.
 *
 * Updates the library view with new information.
 *
 * @param view The view.
 * @param libraries The new list of libraries.
 */
void
view_update_libraries (view * view, library * libraries)
{
  char line[80];
  char *pl = line;
  int size = 80;
  library *l;
  int count = 0;

  assert (view);

  win_clear (view->windows[WIN_LIBRARIES]);

  l = libraries;
  while (l != NULL)
    {
      LPRINT (pl, pl != line, size, "%14s %s",
	      l->symbols_loaded ? "  symb. loaded" : "no symb. loaded",
	      l->id);
      win_add_line (view->windows[WIN_LIBRARIES], line, 1, count);
      l = l->next;
    }
  update_panels ();
  doupdate ();

  if (pl != line)
    {
      free (pl);
    }
}

/**
 * @brief Update the frame view.
 *
 * Updates the frame view with new information.
 *
 * @param view The view.
 * @param stack The stack were the frame belongs to.
 * @param level The level of the frame that should be viewed. If < 0
 *              the frame windows is cleared.
 */
void
view_update_frame (view * view, stack * stack, int level)
{
  char line[80];
  char *pl = line;
  int size = 80;
  frame *f;
  variable *v;

  assert (view);

  DINFO (3, "Updating frame window %d", WIN_FRAME);

  win_clear (view->windows[WIN_FRAME]);

  if (level < 0)
    {
      goto out;
    }

  f = &stack->stack[level];

  /* Show file, func and line number. */
  LPRINT (pl, pl != line, size, "#%-2d %s:%d %s()", level, f->file, f->line,
	  f->func);
  win_add_line (view->windows[WIN_FRAME], line, 1, -1);

  /* Show frame's arguments. */
  v = f->args;
  while (v != NULL)
    {
      LPRINT (pl, pl != line, size, "    %s %s%s%s;", v->type ? v->type : "",
	      v->name, v->value ? " = " : "", v->value ? v->value : "");
      win_add_line (view->windows[WIN_FRAME], line, 1, -1);
      v = v->next;
    }

  /* Show frame's variables. */
  win_add_line (view->windows[WIN_FRAME], "{", 1, -1);
  v = f->variables;
  while (v != NULL)
    {
      LPRINT (pl, pl != line, size, "  %s %s%s%s;", v->type ? v->type : "",
	      v->name, v->value ? " = " : "", v->value ? v->value : "");
      win_add_line (view->windows[WIN_FRAME], line, 1, -1);
      v = v->next;
    }
  win_add_line (view->windows[WIN_FRAME], "}", 1, -1);

  if (f->fullname != NULL)
    {
      view_show_file (view, f->fullname, f->line, 1);
    }
out:
  /* Update the window. */
  update_panels ();
  doupdate ();

  if (pl != line)
    {
      free (pl);
    }
}

/**
 * @brief Update stack window.
 *
 * Update the stack window with the list of frames in \a stack.
 *
 * @param view The view.
 * @param stack The stack that should be viewed.
 */
void
view_update_stack (view * view, stack * stack)
{
  char line[80];
  char *pl = line;
  int size = 80;
  int i;
  frame *f;

  assert (view);

  DINFO (3, "Updating stack window %d", WIN_STACK);

  win_clear (view->windows[WIN_STACK]);

  /* Loop all frames and print the file, func and line number. */
  for (i = 0; i < stack->depth; i++)
    {
      f = &stack->stack[i];
      LPRINT (pl, pl != line, size, "#%-2d %s:%d %s()", i, f->file, f->line,
	      f->func);
      win_add_line (view->windows[WIN_STACK], line, 1, i);
    }

  /* Update the stack window. */
  win_go_to_line (view->windows[WIN_STACK], 0);

  update_panels ();
  doupdate ();

  if (pl != line)
    {
      free (pl);
    }
}

/**
 * @brief Update the assembler view.
 *
 * Update the assembler view.
 *
 * @param view The view.
 * @param ass The assembler lines.
 * @param pc The current pc, used for setting a marker on the current asm line.
 */
void
view_update_ass (view * view, assembler * ass, int pc)
{
  char buf[512];
  char *p = buf;
  char *t;
  int size = 512;
  int ret;
  src_line *sline;
  asm_line *line;
  int show_text;

  assert (view);

  win_clear (view->windows[WIN_DISASSAMBLE]);
  if (win_get_filename (view->windows[WIN_MAIN]) != NULL)
    {
      show_text = strstr (win_get_filename (view->windows[WIN_MAIN]),
			  ass->file) != NULL;
    }
  else
    {
      show_text = 0;
    }

  t = p;
  LPRINT (p, p != buf, size, " 0x%08X - %s ()", ass->address, ass->function);
  if (p != t && t != buf)
    {
      free (t);
    }

  ret = win_add_line (view->windows[WIN_DISASSAMBLE], p, 1, 0);
  if (ret < 0)
    {
      LOG_ERR ("Failed to add asm line");
      goto error;
    }

  sline = ass->lines;
  while (sline)
    {
      LPRINT (p, p != buf, size, "%4d %s", sline->line_nr, show_text ?
	      win_get_line (view->windows[WIN_MAIN],
			    sline->line_nr - 1) : "");
      if (p != t && t != buf)
	{
	  free (t);
	}
      ret = win_add_line (view->windows[WIN_DISASSAMBLE], p, 1, 0);
      if (ret < 0)
	{
	  LOG_ERR ("Failed to add asm line");
	  goto error;
	}
      line = sline->lines;
      while (line)
	{
	  t = p;
	  LPRINT (p, p != buf, size, "+0x%08X - %s", line->offset,
		  line->inst);
	  if (p != t && t != buf)
	    {
	      free (t);
	    }

	  ret = win_add_line (view->windows[WIN_DISASSAMBLE], p, 1, 0);
	  if (ret < 0)
	    {
	      LOG_ERR ("Failed to add asm line");
	      goto error;
	    }
	  win_set_mark (view->windows[WIN_DISASSAMBLE], -1, 0,
			line->address == pc ? 'S' : ' ');
	  line = line->next;
	}
      sline = sline->next;
    }
  update_panels ();
  doupdate ();
error:
  if (p != buf)
    {
      free (p);
    }
}

/**
 * @brief Update the registers view.
 *
 * Update the registers view.
 *
 * @param view The view.
 * @param regs The registers.
 */
void
view_update_registers (view * view, data_registers * regs)
{
  int i;
  char buf[512];
  char *p = buf;
  int size = 512;
  int nlen = 0;
  int vlen = 0;

  assert (view);

  for (i = 0; i < regs->len; i++)
    {
      if (strlen (regs->registers[i].reg_name) > nlen)
	{
	  nlen = strlen (regs->registers[i].reg_name);
	}
      if (regs->registers[i].svalue
	  && strlen (regs->registers[i].svalue) > vlen)
	{
	  vlen = strlen (regs->registers[i].svalue);
	}
    }
  win_clear (view->windows[WIN_REGISTERS]);

  for (i = 0; i < regs->len; i += 2)
    {
      LPRINT (p, p != buf, size, "%*s %*s %*s %*s", nlen,
	      regs->registers[i].reg_name,
	      vlen,
	      regs->registers[i].svalue ? regs->registers[i].svalue : "",
	      nlen, i + 1 < regs->len ? regs->registers[i + 1].reg_name : "",
	      vlen,
	      i + 1 <
	      regs->len ? (regs->registers[i + 1].svalue ? regs->
			   registers[i + 1].svalue : "") : "");
      win_add_line (view->windows[WIN_REGISTERS], p, 1, i);
    }

  update_panels ();
  doupdate ();
  if (p != buf)
    {
      free (p);
    }
}

/**
 * @brief Remove breakpoint from main window.
 *
 * Removes a breakpoint marks from the main window.
 *
 * @param view The view.
 * @param file_name The file name associated with the main window.
 * @param line_nr The line number of the breakpoint.
 */
void
view_remove_breakpoint (view * view, const char *file_name, int line_nr)
{
  const char *f;

  assert (view);

  f = win_get_filename (view->windows[WIN_MAIN]);
  if (f == NULL || file_name == NULL || strcmp (f, file_name) != 0)
    {
      return;
    }
  win_set_mark (view->windows[WIN_MAIN], line_nr, 0, ' ');
  win_set_mark (view->windows[WIN_MAIN], line_nr, 1, ' ');

  update_panels ();
  doupdate ();
}

/**
 * @brief Load a file in a window.
 *
 * Clear the main window and load a text file to the main window. Set \a line
 * in the middle of the window.
 *
 * @param view The view.
 * @param file_name The file name.
 * @param line The line that should be in the middle of the window.
 * @param mark_stop If set to 1 an 'S' will be shown in the main window.
 *
 * @return 0 if successful otherise -1.
 */
int
view_show_file (view * view, const char *file_name, int line, int mark_stop)
{
  int ret;

  assert (view);

  DINFO (3, "Go to line %d in '%s'", line, file_name ? file_name : "");

  if (file_name == NULL || strlen (file_name) == 0)
    {
      win_clear (view->windows[WIN_MAIN]);
      return 0;
    }

  ret = win_load_file (view->windows[WIN_MAIN], file_name);
  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not load file '%s'", file_name);

  if (mark_stop)
    {
      if (view->last_stop_mark >= 0)
	{
	  ret =
	    win_set_mark (view->windows[WIN_MAIN], view->last_stop_mark, 2,
			  ' ');
	  LOG_ERR_IF (ret < 0, "Could not un-set mark");
	}
      ret = win_set_mark (view->windows[WIN_MAIN], line - 1, 2, 'S');
      LOG_ERR_IF_RETURN (ret < 0, -1, "Could not set mark");
      view->last_stop_mark = line - 1;
    }
  ret = win_go_to_line (view->windows[WIN_MAIN], line - 1);
  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not goto '%s':%d", file_name, line);

  update_panels ();
  doupdate ();

  return 0;
}

/**
 * @brief Set the status line of a window.
 *
 * Set a new status line for the specified window.
 *
 * @param view The view.
 * @param type The type of window; WIN_MAIN, ..., LAST_WINDOW.
 * @param status The new status line. Will be copied, so caller should free it.
 *
 * @return 0 if the new status line was set. -1 on failuer.
 */
int
view_set_status (view * view, int type, const char *status)
{
  assert (view);

  LOG_ERR_IF_RETURN (type < 0
		     || type > LAST_WINDOW, -1, "Wrong type %d", type);
  win_set_status (view->windows[type], status);

  update_panels ();
  doupdate ();

  return 0;
}

/**
 * @brief Set focus of the specified window.
 *
 * Set the focus to the specified window.
 *
 * @param view The view.
 * @param type The window type that should have the focus.
 *
 * @return 0 if successful, or -1 if the @a type was out of bounds.
 */
int
view_set_focus (view * view, int type)
{
  assert (view);

  DINFO (3, "Change focus from %d to %d", view->current_index, type);

  LOG_ERR_IF_RETURN (type < 0 || type > LAST_WINDOW, -1,
		     "Windows type %d is out of bounds", type);

  /* Remove old focus. */
  win_set_focus (view->current_window, 0);

  /* Set new focus window. */
  view->current_index = type;
  view->current_window = view->windows[view->current_index];
  win_set_focus (view->current_window, 1);
  win_to_top (view->current_window);

  /* Update the screen. */
  update_panels ();
  doupdate ();

  return 0;
}

/**
 * @brief Go to specified window and line number.
 *
 * Go to specified window and to the specified line number. If the type is not
 * the current, focus will be changed.
 *
 * @param view The view.
 * @param type The type of window to go to.
 * @param line_nr The line number to go to.
 *
 * @return 0 if the go to was successful.
 */
int
view_go_to_line (view * view, int type, int line_nr)
{
  assert (view);

  DINFO (3, "Goto line %d in '%s'", line_nr,
	 type >= 0
	 && type <= LAST_WINDOW ? out_windows[type].name : "Current");

  LOG_ERR_IF_RETURN (type < 0 || type > LAST_WINDOW, -1,
		     "Window type %d out of bounds", type);

  /* Set focus to the specified window. */
  if (type != view->current_index)
    {
      view_set_focus (view, type);
    }

  /* Goto line. */
  return win_go_to_line (view->current_window, line_nr);
}

/**
 * @brief Scroll the window up.
 *
 * Scroll the current window up one line.
 *
 * @param view The view.
 *
 * @return 0 if the current window was scroll. -1 if failed.
 */
int
view_scroll_up (view * view)
{
  int ret;

  assert (view);

  ret = win_move (view->current_window, 1);

  update_panels ();
  doupdate ();

  return ret;
}

/**
 * @brief Scroll the window down.
 *
 * Scroll the current window down one line.
 *
 * @param view The view.
 *
 * @return 0 if the current window was scroll. -1 if failed.
 */
int
view_scroll_down (view * view)
{
  int ret;

  assert (view);

  ret = win_move (view->current_window, -1);

  update_panels ();
  doupdate ();

  return ret;
}

/**
 * @brief Select the next window.
 *
 * Select the next window, and set the focus on the new window.
 *
 * @param view The view.
 * @param dir The direction, 1 for one direction 0 for the other. That is calling
 *        this function first with 1 and then 0, we end up in the first
 *        window.
 * @param type If set to 1 the current group is changed instead of window. If
 *             set to 2 next view is changed.
 *
 * @return 0 on success otherwise -1.
 */
int
view_next_window (view * view, int dir, int type)
{
  assert (view);
  group *cgroup = &view->groups[view->current_view];
  int gr;
  int i;

  DINFO (3, "Dir %d Type %d", dir, type);
  /* Unset focus. */
  win_set_focus (view->current_window, 0);

  if (type == 1)
    {
      cgroup->current_group += (dir > 0) ? 1 : -1;
      if (cgroup->current_group == -1)
	{
	  cgroup->current_group = cgroup->nr_of_groups - 1;
	}
      else if (cgroup->current_group == cgroup->nr_of_groups)
	{
	  cgroup->current_group = 0;
	}
    }
  else if (type == 2)
    {
      view->current_view = (view->current_view + dir) % view->views;
      cgroup = &view->groups[view->current_view];
      for (i = 0; i < cgroup->nr_of_groups; i++)
	{
	  win_to_top (view->
		      windows[cgroup->groups[i][cgroup->current_win[i]]]);
	}
    }
  else
    {
      gr = cgroup->current_group;
      cgroup->current_win[gr] += (dir > 0) ? 1 : -1;
      if (cgroup->current_win[gr] == -1)
	{
	  cgroup->current_win[gr] = cgroup->nr_of_wins_in_group[gr] - 1;
	}
      else if (cgroup->current_win[gr] == cgroup->nr_of_wins_in_group[gr])
	{
	  cgroup->current_win[gr] = 0;
	}
    }
  gr = cgroup->current_group;
  view->current_index = cgroup->groups[gr][cgroup->current_win[gr]];

  view->current_window = view->windows[view->current_index];
  win_set_focus (view->current_window, 1);
  win_to_top (view->current_window);

  update_panels ();
  doupdate ();

  return 0;
}

/**
 * @brief Move the cursor.
 *
 * Move the cursor of the current window to the specified line.
 *
 * @param view The view.
 * @param n The line number to move to.
 *
 * @return 0 upon succes.
 */
int
view_move_cursor (view * view, int n)
{
  int ret;

  assert (view);

  ret = win_move_cursor (view->current_window, n);

  if (ret == 0)
    {
      update_panels ();
      doupdate ();
    }

  return ret;
}

/**
 * @brief Get the tag of the window.
 *
 * Get the tag of the current cursor position. If win is < 0, the current
 * window's tag is returned and win is set to the corresponding window. If
 * win is a valid #window_type the current tag for that window is returned.
 *
 * @param view The view.
 * @param win If it is set to < 0, the current window index will be set in
 *            @a win. The tag of the current window will be returned.
 *            If @a win is set to a valid windows type, the tag for that window
 *            is returned.
 *
 * @return The tag for the selected window is returned.
 */
int
view_get_tag (view * view, int *win)
{
  assert (view);
  assert (win);

  if (*win < 0 || *win > LAST_WINDOW)
    {
      *win = view->current_index;
      return win_get_tag (view->current_window);
    }

  return win_get_tag (view->windows[*win]);
}

/**
 * @brief Get the cursor position from the specified window.
 *
 * Get the filename and the cursor position for the specidied window. If @a win
 * is -1, the information for the current window is returned. If there is no
 * file name associated with the window or no cursor position we return -1.
 *
 * @param view The view.
 * @param win Which window we shall get the information from.
 * @param line_nr The cursors line number.
 * @param file_name The file name of the window.
 *
 * @return 0 if the information is valid, otherwise -1.
 */
int
view_get_cursor (view * view, int *win, int *line_nr, const char **file_name)
{
  assert (view);
  assert (win);
  assert (line_nr);
  assert (file_name);

  if (*win == -1)
    {
      *win = view->current_index;
    }

  *file_name = win_get_filename (view->windows[*win]);
  *line_nr = win_get_cursor (view->windows[*win]);

  return (*file_name == NULL || *line_nr < 0) ? -1 : 0;
}

/**
 * @brief Toggle view mode.
 *
 * Toggles the view mode. Mode 0 is the ncurse mode. Mode 1 is the non ncurse
 * mode.
 *
 * @param view
 */
void
view_toggle_view_mode (view * view)
{
  if (view->view_mode)
    {
      update_panels ();
      refresh ();
      doupdate ();
      view->view_mode = 0;
    }
  else
    {
      endwin ();
      win_dump (view->windows[WIN_TARGET]);
      view->view_mode = 1;
    }
}
