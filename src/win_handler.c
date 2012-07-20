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
 * @file win_handler.c
 *
 * @brief Implements the functions used for a window.
 *
 * Implements the functions for manipulate the windows.
 *
 */
#include <ncurses.h>
#include <panel.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "win_handler.h"
#include "text.h"
#include "debug.h"
#include "vsscanner.h"
#include "misc.h"

#define MARKS_LEN 10 /**< Max number of markers. */
#define TLI_INCREASE 100 /**<
                          * The increase amount when allocating more
                          * text line info.
                          */

#define NEEDS_TEXT_LINE_INFO(x) ((x) & (WIN_PROP_MARKS | WIN_PROP_CURSOR))

/*******************************************************************************
 * Internal structures and enums
 ******************************************************************************/
/** Information about each line of the text. */
typedef struct text_line_info_t
{
  int marked;		  /**< 1 if the line has a marker. */
  int cur_pos;		  /**< 1 if the cursor is on this line. */
  char marks[MARKS_LEN];  /**< The mark (s) to be used if mark is set. */
  int tag;		  /**<
                           * A tag number that are set by win_add_line and
                           * the tag of then current cursor position can be
                           * retrieved by win_get_tag.
                           */
  id_table ids;		  /**< Information about the scanned lines. */
} text_line_info;

/** Structure for information on windows lines. */
typedef struct line_info_t
{
  int n;       /**< Denotes the text line that is on a window line. */
  int part;    /**<
                * The part of the text line that is represented on the
                * windown line. Total number of parts are
                * len/(window width - indent).
                */
  int len;     /**<
                * The length of the whole text line, which could take up
                * more than one line on the screen.
                */
  const char *text; /**< The line of text. */
} line_info;

/**
 * @brief Structure for window objects.
 *
 * Structure representing a window. Holds information about the actual curse
 * window, text object and properties of the window.
 *
 * @todo Hold more than only one text object, so each text does bot have to be
 *       loaded all the time.
 * */
struct Win_t
{
  WINDOW *window;      /**< The actual curses window. */
  PANEL *panel;	       /**< The panel assiciated with the window. */

  int startx;	       /**< Start position of the window. */
  int starty;	       /**< Start position of the window. */
  int width;	       /**< Width of the window. */
  int height;	       /**< Height of the window. */

  win_properties props;	/**< The window's properties. */

  text *text;	       /**< The text object in this window. */
  line_info *line_info;	/**< The line information of a screen line. */
  char *status_line;   /**< The status text. */
  int focus;	       /**< 1 if the windows has focus. 0 if not in focus. */

  text_line_info *text_line_info; /**< Info about each line in the text. */
  int tli_len;			/**< Length of the \a text_line_info array. */

  int cursor_pos; /**< The current cursor pos. */

  char *file_name; /**<
                    * The current file name if the text was loaded from a
                    * file.
                    */
  vsscanner *scanner; /**< Scanner used for finding parts to highlight. */
};

/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
int win_setup_scanner (Win * win);
int win_draw_text_line (Win * win, int n);
int win_draw_line (Win * win, int y, int width);
int win_update_cursor (Win * win, int pos);
void win_add_text_line_info (Win * win, int tli_len);
void win_redraw_status (Win * win);
int win_redraw_show_top (Win * win, int line);
int win_redraw_show_bottom (Win * win, int line);

/**
 * @brief Set up scanner.
 *
 * Set up the scanner for highlighting.
 *
 * @param win The windows.
 *
 * @return 0 upon success. -1 if failed.
 */
int
win_setup_scanner (Win * win)
{
  char *text = NULL;
  char *next;
  char *name;
  char *value;
  char *inext;
  char *iname;
  char *ivalue;
  char *endptr;
  int id;
  int type;
  int ret;

  assert (win);

  /* Any definitions? */
  if (win->props.scan_definitions == NULL || win->props.nr_of_attributes <= 0)
    {
      LOG_ERR ("No syntax definitions %p", win->props.scan_definitions);
      goto error;
    }

  /* Set up scanner. */
  win->scanner = vsscanner_create ();
  if (win->scanner == NULL)
    {
      LOG_ERR ("Failed to create definitions");
      goto error;
    }

  text = strdup (win->props.scan_definitions);
  if (text == NULL)
    {
      LOG_ERR ("Could not create definitions");
      goto error;
    }

  next = text;
  while (next && *next)
    {
      ret = get_next_param (next, &name, &value, &next);
      if (ret < 0)
	{
	  LOG_ERR ("Could not parse syntax id '%s'", next);
	  goto error;
	}
      if (ret != '{')
	{
	  LOG_ERR ("Could not parse syntax, expected '{' found '%c'", ret);
	  goto error;
	}
      id = -1;
      type = 1;
      inext = value;
      while (inext && *inext)
	{
	  ret = get_next_param (inext, &iname, &ivalue, &inext);
	  if (ret < 0 || iname == NULL || ivalue == NULL)
	    {
	      LOG_ERR ("Could not parse syntax id '%s'", inext);
	      goto error;
	    }
	  if (strcmp (iname, "id") == 0)
	    {
	      id = strtol (ivalue, &endptr, 0);
	      if (endptr == ivalue)
		{
		  LOG_ERR ("Could not parse syntax '%s' is not a value",
			   ivalue);
		}
	    }
	  else if (strcmp (iname, "type") == 0)
	    {
	      type = strtol (ivalue, &endptr, 0);
	      if (endptr == ivalue)
		{
		  LOG_ERR ("Could not parse syntax '%s' is not a value",
			   ivalue);
		}
	    }
	  else if (strcmp (iname, "match") == 0)
	    {
	      DINFO (3, "Syntax match id %d word %d '%s'", id, type, ivalue);
	      if (id < 1 || id > win->props.nr_of_attributes)
		{
		  LOG_ERR ("Syntax id '%d' invalid", id);
		  goto error;
		}
	      ret =
		vsscanner_add_rule (win->scanner, ivalue, id - 1, type == 2,
				    type == 1);
	      if (ret < 0)
		{
		  LOG_ERR ("Failed to add syntax rule '%s' id %d", ivalue,
			   id);
		  goto error;
		}
	    }
	  else
	    {
	      LOG_ERR ("Could not parse syntax '%s'", iname);
	      goto error;
	    }

	}
    }

  if (text)
    {
      free (text);
    }
  return 0;

error:
  if (win->scanner)
    {
      vsscanner_free (win->scanner);
      win->scanner = NULL;
    }

  if (text)
    {
      free (text);
    }
  win->props.properties &= ~WIN_PROP_SYNTAX;
  return -1;
}

/**
 * @brief Draw a line of text.
 *
 * Draw the specified line on text, if it is in the window.
 *
 * @param win The win.
 * @param n The text line to draw. If it is not currently in the window it is
 *          not drawm.
 *
 * @return 0 upon success.
 */
int
win_draw_text_line (Win * win, int n)
{
  int width;
  int y;
  int ret;

  assert (win);

  width = win->width - win->props.indent;

  for (y = 0; y < win->height - 1; y++)
    {
      if (win->line_info[y].n == n)
	{
	  ret = win_draw_line (win, y, width);
	  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not draw line");
	}
    }
  return 0;
}

/**
 * @brief Draw a line in the window.
 *
 * Draw the specified window line.
 *
 * @param win The window.
 * @param y The window line to draw.
 * @param width The window body text width. The window width - indent.
 *
 * @return 0 upon success.
 */
int
win_draw_line (Win * win, int y, int width)
{
  int n;
  int part;
  int ret = 0;
  int i;
  int xstart;
  int xstop;
  id_table *pid;
  int x;
  int border = win->props.properties & WIN_PROP_BORDER ? 1 : 0;

  assert (win);
  assert (y >= 0 && y < win->height - 1);

  n = win->line_info[y].n;
  part = win->line_info[y].part;

  if ((win->props.properties & WIN_PROP_MARKS)
      && part == 0 && win->text_line_info[n].marks[0])
    {
      ret = mvwaddstr (win->window, y + border, border,
		       win->text_line_info[n].marks);
    }
  if ((win->props.properties & WIN_PROP_CURSOR)
      && win->text_line_info[n].cur_pos)
    {
      wattron (win->window, A_REVERSE);
    }

  if ((win->props.properties & WIN_PROP_SYNTAX) == 0)
    {
      ret = mvwaddnstr (win->window, y + border, win->props.indent + border,
			win->line_info[y].text +
			win->line_info[y].part * width, width);
      goto out;
    }
  i = 0;
  pid = &win->text_line_info[n].ids;
  xstart = win->line_info[y].part * width;
  xstop = xstart + (win->line_info[y].part < win->line_info[y].len / width ?
		    width : win->line_info[y].len % width);
  x = xstart;
  while (x < xstop)
    {
      while (i < pid->len && (pid->id[i].index + pid->id[i].len <= x))
	{
	  i++;
	}
      if (i < pid->len
	  && x >= pid->id[i].index && x < pid->id[i].index + pid->id[i].len)
	{
	  DINFO (1, "Attr %d %d", win->props.attributes[pid->id[i].id].color,
		 win->props.attributes[pid->id[i].id].attr);
	  wattron (win->window,
		   COLOR_PAIR (win->props.attributes[pid->id[i].id].color));
	  wattron (win->window, win->props.attributes[pid->id[i].id].attr);
	  ret = mvwaddnstr (win->window, y, win->props.indent + x - xstart,
			    win->line_info[y].text + x,
			    pid->id[i].index + pid->id[i].len - x);
	  wattroff (win->window, win->props.attributes[pid->id[i].id].attr);
	  wattroff (win->window,
		    COLOR_PAIR (win->props.attributes[pid->id[i].id].color));
	  DINFO (10, "Draw attr %d %d %d %d %d '%.*s'",
		 i, n, x, pid->id[i].index, pid->id[i].len,
		 pid->id[i].index + pid->id[i].len - x,
		 win->line_info[y].text + x);
	  x = pid->id[i].index + pid->id[i].len;
	}
      else if (i < pid->len && pid->id[i].len > 0)
	{
	  ret = mvwaddnstr (win->window, y, win->props.indent + x - xstart,
			    win->line_info[y].text + x, pid->id[i].index - x);
	  DINFO (10, "Draw to attr %d %d %d %d %d '%.*s'",
		 i, n, x, pid->id[i].index, pid->id[i].len,
		 pid->id[i].index - x, win->line_info[y].text + x);
	  x = pid->id[i].index;
	}
      else
	{
	  ret = mvwaddnstr (win->window, y, win->props.indent + x - xstart,
			    win->line_info[y].text + x, xstop - x);
	  DINFO (10, "Draw %d %d %d %d %d '%.*s'",
		 i, n, x, pid->id[i].index, pid->id[i].len,
		 xstop - x, win->line_info[y].text + x);
	  x = xstop;
	}
    }

out:
  if ((win->props.properties & WIN_PROP_CURSOR)
      && win->text_line_info[n].cur_pos)
    {
      wattroff (win->window, A_REVERSE);
    }

  if (win->props.properties & WIN_PROP_BORDER)
    {
      box (win->window, 0, 0);
    }
  return ret;
}

/**
 * @brief Set cursor position.
 *
 * Updates the cursor position to new value. The function assumes that there
 * is a text_line_info structure for that line number.
 *
 * @param win The window.
 * @param pos The new position.
 *
 * @return 0 if the cursor was moved. -1 if failed to move the cursor.
 */
int
win_update_cursor (Win * win, int pos)
{
  int ret;

  assert (win);

  if (!NEEDS_TEXT_LINE_INFO (win->props.properties))
    {
      return 0;
    }

  if (pos < 0)
    {
      pos = 0;
    }
  else if (pos > text_nr_of_lines (win->text))
    {
      pos = text_nr_of_lines (win->text);
    }

  if (win->cursor_pos >= 0)
    {
      win->text_line_info[win->cursor_pos].cur_pos = 0;
      ret = win_draw_text_line (win, win->cursor_pos);
      LOG_ERR_IF_RETURN (ret < 0, -1, "Could not draw line");
    }
  win->text_line_info[pos].cur_pos = 1;
  win->cursor_pos = pos;
  ret = win_draw_text_line (win, pos);

  return ret;
}

/**
 * @brief Allocate more text line information.
 *
 * Allocate more memory for text line infromation. The function will allocate
 * more memory so that \a nr + TLI_INCREASE text_line_info structures could be
 * stored.
 *
 * @param win The window that needs more text line infos.
 * @param nr The amount needed. A bit more than \a nr will be allocated.
 */
void
win_add_text_line_info (Win * win, int nr)
{
  int i;
  int j;

  assert (win);
  assert (nr >= 0);

  if (nr <= win->tli_len)
    {
      return;
    }

  nr += TLI_INCREASE;
  win->text_line_info = (text_line_info *) realloc (win->text_line_info,
						    nr *
						    sizeof (text_line_info));
  LOG_ERR_IF_FATAL (win->text_line_info == NULL,
		    ERR_MSG_CREATE ("text line info"));

  for (i = win->tli_len; i < nr; i++)
    {
      for (j = 0; j < win->props.indent; j++)
	{
	  win->text_line_info[i].marks[j] = ' ';
	}
      win->text_line_info[i].marks[j] = '\0';
      win->text_line_info[i].marked = 0;
      win->text_line_info[i].cur_pos = 0;
      win->text_line_info[i].ids.len = 0;
      win->text_line_info[i].ids.size = DEF_IDT_LEN;
    }

  win->tli_len = nr;
}

/**
 * @brief Redraws the status line.
 *
 * Redraws the status line of the window.
 *
 * @param win The window.
 */
void
win_redraw_status (Win * win)
{
  int ret;
  int border = win->props.properties & WIN_PROP_BORDER ? 1 : 0;
  if (win->focus)
    {
      wattron (win->window, A_UNDERLINE);
    }
  else
    {
      wattron (win->window, A_REVERSE);
    }
  ret = mvwaddnstr (win->window, win->height - 1 + border, border,
		    win->status_line, win->width);
  wchgat (win->window, -1, win->focus ? A_UNDERLINE : A_REVERSE, 1, NULL);
  if (win->focus)
    {
      wattroff (win->window, A_UNDERLINE);
    }
  else
    {
      wattroff (win->window, A_REVERSE);
    }
}

/**
 * @brief Shows the speciefied line on screen line 0.
 *
 * Redraws the window with the text line @a line as the first line in the
 * window.
 *
 * @param win The window.
 * @param line The line number that should be the first on the window.
 *
 * @return 0 on success. -1 on failure.
 */
int
win_redraw_show_top (Win * win, int line)
{
  int y;
  int n;
  int part;
  int len;
  int width = win->width - win->props.indent;
  const char *text = NULL;
  int ret;

  if (line < 0)
    {
      line = 0;
    }
  else if (line >= text_nr_of_lines (win->text))
    {
      line = text_nr_of_lines (win->text) - 1;
    }
  n = line - 1;
  y = 0;
  part = 0;
  len = 0;
  while (y < win->height - 1)
    {
      if (part == len / width)
	{
	  n++;
	  text = text_get_line (win->text, n, &len);
	  if (text == NULL)
	    {
	      /* No more text */
	      goto out;
	    }
	  part = 0;
	}
      else
	{
	  part++;
	}
      win->line_info[y].len = len;
      win->line_info[y].n = n;
      win->line_info[y].part = part;
      win->line_info[y].text = text;
      ret = win_draw_line (win, y, width);
      y++;
    }
  return 0;

out:
  while (y < win->height - 2)
    {
      win->line_info[y].len = -1;
      win->line_info[y].n = -1;
      win->line_info[y].part = -1;
      y++;
    }
  return 0;
}

/**
 * @brief Shows the speciefied line on screen's last line.
 *
 * Redraws the window with the text line @a line as the last line in the
 * window.
 *
 * @param win The window.
 * @param line The line number that should be the last on the window.
 *
 * @return 0 on success. -1 on failure.
 */
int
win_redraw_show_bottom (Win * win, int line)
{
  int part;
  int n;
  int y;
  const char *text = NULL;
  int len;
  int width;
  int ret;

  assert (win);

  if (line < 0)
    {
      line = 0;
    }
  else if (line >= text_nr_of_lines (win->text))
    {
      line = text_nr_of_lines (win->text) - 1;
    }

  width = win->width - win->props.indent;
  y = win->height - 2;
  n = line + 1;
  part = 0;
  while (y >= 0 && (n > 0 || part > 0))
    {
      if (part == 0)
	{
	  n--;
	  text = text_get_line (win->text, n, &len);
	  if (text == NULL)
	    {
	      goto error;
	    }
	  part = len / width;
	}
      else
	{
	  part--;
	}
      win->line_info[y].len = len;
      win->line_info[y].n = n;
      win->line_info[y].part = part;
      win->line_info[y].text = text;
      ret = win_draw_line (win, y, width);
      y--;
    }
  return 0;

error:
  LOG_ERR ("Empty line.");
  return -1;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Create a window.
 *
 * @param starty Start of the window.
 * @param startx Start of the window.
 * @param height Height of the window.
 * @param width Width of the window.
 * @param props Properties of the window that should be created.
 *
 * @return A pointer to the new structure. If we failed to create a new window
 *         NULL is returned.
 */
Win *
win_create (int starty, int startx, int height, int width,
	    win_properties * props)
{
  Win *wnd;
  int i;
  int ret;

  wnd = (Win *) malloc (sizeof (*wnd));
  LOG_ERR_IF_FATAL (wnd == NULL, ERR_MSG_CREATE ("win"));
  memset (wnd, 0, sizeof (*wnd));

  /* Set up dimensions. */
  wnd->height = height - (props->properties & WIN_PROP_BORDER ? 2 : 0);
  wnd->width = width - (props->properties & WIN_PROP_BORDER ? 2 : 0);
  wnd->starty = starty;
  wnd->startx = startx;
  wnd->status_line = NULL;
  wnd->file_name = NULL;
  wnd->scanner = NULL;
  wnd->focus = 0;

  /* Create nurces objects. */
  wnd->window = newwin (height, width, starty, startx);
  if (wnd->window == NULL)
    {
      goto error;
    }
  wnd->panel = new_panel (wnd->window);
  if (wnd->panel == NULL)
    {
      goto error;
    }

  wnd->text = text_create ();
  if (wnd->text == NULL)
    {
      goto error;
    }
  wnd->line_info = (line_info *) malloc ((height - 1) * sizeof (line_info));
  for (i = 0; i < height - 1; i++)
    {
      wnd->line_info[i].n = -1;
      wnd->line_info[i].part = -1;
    }

  memcpy (&(wnd->props), props, sizeof (*props));
  if (wnd->props.indent >= MARKS_LEN - 1)
    {
      wnd->props.indent = MARKS_LEN - 2;
      LOG_ERR ("Setting indent to '%d'", MARKS_LEN - 2);
    }
  wnd->cursor_pos = -1;
  wnd->tli_len = 0;
  if (NEEDS_TEXT_LINE_INFO (props->properties))
    {
      win_add_text_line_info (wnd, 0);
    }

  if (props->properties & WIN_PROP_BORDER)
    {
      box (wnd->window, 0, 0);
    }

  /* Set windows properties. */
  scrollok (wnd->window, TRUE);
  wsetscrreg (wnd->window, wnd->props.properties & WIN_PROP_BORDER ? 1 : 0,
	      height - 2 - (props->properties & WIN_PROP_BORDER ? 1 : 0));
  leaveok (wnd->window, TRUE);

  /* Scanner */
  if (props->properties & WIN_PROP_SYNTAX)
    {
      ret = win_setup_scanner (wnd);
      if (ret < 0)
	{
	  LOG_ERR ("Failed to set up syntax");
	}
    }
  else
    {
      wnd->scanner = NULL;
    }

  DINFO (1, "Created new window height %d width %d at (%d; %d)", height,
	 width, starty, startx);
  return wnd;

error:
  if (wnd != NULL)
    {
      win_free (wnd);
    }
  LOG_ERR ("Failed creating window height %d width %d at (%d; %d)", height,
	   width, starty, startx);
  return NULL;
}

/**
 * @brief Free the window.
 *
 * Free the window and it's resources.
 *
 * @param win Window to be set free.
 */
void
win_free (Win * win)
{
  assert (win);

  if (win->panel != NULL)
    {
      del_panel (win->panel);
    }
  if (win->window != NULL)
    {
      delwin (win->window);
    }
  if (win->line_info != NULL)
    {
      free (win->line_info);
    }
  if (win->status_line != NULL)
    {
      free (win->status_line);
    }

  if (win->text_line_info != NULL)
    {
      free (win->text_line_info);
    }

  if (win->file_name != NULL)
    {
      free (win->file_name);
    }
  if (win->scanner != NULL)
    {
      vsscanner_free (win->scanner);
    }
  text_free (win->text);

  free (win);
}

/**
 * @brief Set a new status line of the window.
 *
 * Set a new status line for the window. For lines with line breaks etc only
 * the part upto the line break etc will be shown.
 *
 * @param win The window that will get the new status line.
 * @param line The new status line. If NULL an empty status line will be
 *        created.
 */
void
win_set_status (Win * win, const char *line)
{
  char *p;

  assert (win);

  if (win->status_line != NULL)
    {
      free (win->status_line);
    }

  if (line && strlen (line) != 0)
    {
      win->status_line = strdup (line);
    }
  else
    {
      win->status_line = strdup ("");
    }
  LOG_ERR_IF_FATAL (win->status_line == NULL, ERR_MSG_CREATE ("status line"));

  p = win->status_line;
  while (*p)
    {
      if (strchr ("\r\n\v\f\a", *p) != NULL)
	{
	  *p = '\0';
	  break;
	}
      p++;
    }
  win_redraw_status (win);

  DINFO (1, "Setting status to '%s'", win->status_line);
}

/**
 * @brief Add a line of text to the window.
 *
 * Add a line of text at the end of the text window. If scroll is not 0
 * the window scrolls so that the added line will be shown as the last line.
 *
 * @param win The window to add the text to.
 * @param line The line of text.
 * @param scroll If not 0 the last line will be shown in the window.
 * @param tag The tag number that will be set for this line. The tag for the
 *            current cursor position is retrieved be win_get_tag.
 *
 * @return 0 if successfull, otherwise < 0.
 */
int
win_add_line (Win * win, const char *line, int scroll, int tag)
{
  int pos;
  line_info *li;
  int ret;

  assert (win);
  assert (line);

  DINFO (5, "Add line '%s' to window '%s'", line,
	 win->status_line ? win->status_line : "-");

  /* Add the text to the text object. */
  pos = text_add_line (win->text, line);
  if (pos < 0)
    {
      LOG_ERR ("Could not add line to window.");
      return pos;
    }

  /* Check if we need more text line infos. */
  if (pos >= win->tli_len)
    {
      win_add_text_line_info (win, pos);
    }

  /* Set the tag. */
  win->text_line_info[pos - 1].tag = tag;

  if (scroll == 0)
    {
      /* No update of the window. */
      return 0;
    }

  /* Scroll or move to to the new line. */
  li = &win->line_info[win->height - 2];
  if (li->n == pos - 2 && li->n != -1)
    {
      ret = win_scroll (win, li->len / (win->width - win->props.indent) -
			li->part + strlen (line) / (win->width -
						    win->props.indent) + 1);
    }
  else
    {
      ret = win_redraw_show_bottom (win, pos - 1);
    }
  return ret;
}

/**
 * @brief Load a text file into window.
 *
 * Loads a text file into the window.
 *
 * @param win The window.
 * @param file_name The file name of the text file.
 *
 * @return 0 upon success, otherwise < 0.
 */
int
win_load_file (Win * win, const char *file_name)
{
  int ret;
  int len;
  int lines;
  int i;
  int j;

  assert (win);

  if (win->file_name && strcmp (file_name, win->file_name) == 0)
    {
      /* Return already loaded. */
      return 0;
    }
  /* Load file in text object. */
  ret = text_update_from_file (win->text, file_name);
  if (ret < 0)
    {
      LOG_ERR ("Loading file failed");
      return ret;
    }

  /* Check if we need more text line info. */
  lines = text_nr_of_lines (win->text);
  if (lines >= win->tli_len)
    {
      win_add_text_line_info (win, lines);
    }
  /* Clear previous marks etc. */
  if (win->props.properties & WIN_PROP_SYNTAX)
    {
      vsscanner_restart (win->scanner);
    }
  for (i = 0; i < lines; i++)
    {
      for (j = 0; j < win->props.indent; j++)
	{
	  win->text_line_info[i].marks[j] = ' ';
	}
      win->text_line_info[i].marks[j] = '\0';
      win->text_line_info[i].marked = 0;
      win->text_line_info[i].cur_pos = 0;
      if (win->props.properties & WIN_PROP_SYNTAX)
	{
	  ret = vsscanner_scan (win->scanner,
				text_get_line (win->text, i, &len),
				&win->text_line_info[i].ids);
	}
    }

  /* Clear line info. */
  for (i = 0; i < win->height - 1; i++)
    {
      win->line_info[i].n = -1;
      win->line_info[i].part = -1;
      win->line_info[i].len = -1;
      win->line_info[i].text = NULL;
    }

  /* Store the file name. */
  if (win->file_name != NULL)
    {
      free (win->file_name);
    }
  win->file_name = strdup (file_name);

  DINFO (1, "Loaded '%s' nr of lines %d", file_name,
	 text_nr_of_lines (win->text));

  /* Update cursor pos. */
  ret = win_update_cursor (win, 0);

  return win_redraw_show_top (win, 0);
}

/**
 * @brief Scroll the window.
 *
 * Scrolls the window @a nr_of_lines lines up or down.
 *
 * @param win The window.
 * @param nr_of_lines The number of screen lines to scroll. If > 0 the window
 *                    is scrolled up. Otherwise scrolled down.
 *
 * @return 0 on success. -1 if failed to scroll.
 */
int
win_scroll (Win * win, int nr_of_lines)
{
  int i;
  line_info *li;
  int n;
  int part;
  const char *line;
  int len;
  int ret;
  int text_width = win->width - win->props.indent;
  assert (win);

  li = &win->line_info[0];
  while (nr_of_lines < 0)
    {
      if (li->part == 0)
	{
	  n = li->n - 1;
	  part = -1;
	}
      else
	{
	  n = li->n;
	  part = li->part - 1;
	}
      line = text_get_line (win->text, n, &len);
      if (line == NULL)
	{
	  LOG_ERR ("Failed to retrieve line %d", n);
	  /* Can not show more lines. */
	  return -1;
	}
      if (part == -1)
	{
	  part = len / (text_width);
	}
      /* Scroll */
      wscrl (win->window, -1);
      for (i = win->height - 2; i > 0; i--)
	{
	  DINFO (5, "Moving line %d to %d", i - 1, i);
	  win->line_info[i] = win->line_info[i - 1];
	}

      /* Add new text. */
      win->line_info[0].len = len;
      win->line_info[0].n = n;
      win->line_info[0].part = part;
      win->line_info[0].text = line;
      ret = win_draw_line (win, 0, text_width);
      nr_of_lines++;
    }

  li = &win->line_info[win->height - 2];
  while (nr_of_lines > 0)
    {
      if (li->part == li->len / text_width)
	{
	  n = li->n + 1;
	  part = 0;
	}
      else
	{
	  n = li->n;
	  part = li->part + 1;
	}
      line = text_get_line (win->text, n, &len);
      if (line == NULL)
	{
	  /* Can not show more lines. */
	  return -1;
	}

      /* Scroll */
      wscrl (win->window, 1);
      for (i = 0; i < win->height - 2; i++)
	{
	  DINFO (5, "Moving line %d to %d", i + 1, i);
	  win->line_info[i] = win->line_info[i + 1];
	}

      /* Add new text. */
      scrollok (win->window, FALSE);
      win->line_info[win->height - 2].len = len;
      win->line_info[win->height - 2].n = n;
      win->line_info[win->height - 2].part = part;
      win->line_info[win->height - 2].text = line;
      ret = win_draw_line (win, win->height - 2, text_width);
      scrollok (win->window, TRUE);
      nr_of_lines--;
    }

  return 0;
}

/**
 * @brief Set the window on top.
 *
 * Set the window on top over all other windows.
 *
 * @param win The window.
 */
void
win_to_top (Win * win)
{
  assert (win);

  top_panel (win->panel);
}

/**
 * @brief Go to speciefied line.
 *
 * Set the specified line as the middle screen line.
 *
 * @param win The window.
 * @param line_nr The line number that should be in the middle of the screen.
 *
 * @return 0 on success. -1 on failure.
 */
int
win_go_to_line (Win * win, int line_nr)
{
  int middle;
  int y;
  int n;
  int part;
  int len;
  int width = win->width - win->props.indent;
  const char *line = NULL;
  int ret;
  int nr_of_lines;

  assert (win);

  nr_of_lines = text_nr_of_lines (win->text);

  LOG_ERR_IF_RETURN (line_nr >= nr_of_lines || line_nr < 0, -1,
		     "Line %d out of bound (%d)", line_nr, nr_of_lines);

  win_update_cursor (win, line_nr);

  wclear (win->window);
  win_redraw_status (win);

  middle = (win->height - 1) / 2;

  /* Fill from middle to top. */
  y = middle;
  n = line_nr + 1;
  part = 0;
  while (y >= 0 && (n > 0 || part > 0))
    {
      if (part == 0)
	{
	  n--;
	  line = text_get_line (win->text, n, &len);
	  LOG_ERR_IF_RETURN (line == NULL, -1, "Could not get line %d", n);
	  part = len / width;
	}
      else
	{
	  part--;
	}
      win->line_info[y].len = len;
      win->line_info[y].n = n;
      win->line_info[y].part = part;
      win->line_info[y].text = line;
      ret = win_draw_line (win, y, width);
      LOG_ERR_IF_RETURN (ret == ERR, -1, "Could not write to screen '%s'",
			 line);
      y--;
    }
  /* Set rest of line info to unused. */
  while (y >= 0)
    {
      win->line_info[y].len = -1;
      win->line_info[y].n = -1;
      win->line_info[y].part = -1;
      win->line_info[y].text = NULL;
      y--;
    }

  /* Fill from middle to bottom. */
  y = middle + 1;
  part = win->line_info[y - 1].part;
  len = win->line_info[y - 1].len;
  n = win->line_info[y - 1].n;
  while (y < win->height - 1 && (n < nr_of_lines - 1 || part < len / width))
    {
      if (part == len / width)
	{
	  n++;
	  line = text_get_line (win->text, n, &len);
	  LOG_ERR_IF_RETURN (line == NULL, -1, "Could not get line %d", n);
	  part = 0;
	}
      else
	{
	  part++;
	}
      win->line_info[y].len = len;
      win->line_info[y].n = n;
      win->line_info[y].part = part;
      win->line_info[y].text = line;
      ret = win_draw_line (win, y, width);
      LOG_ERR_IF_RETURN (ret == ERR, -1, "Could not write to screen '%s'",
			 line);
      y++;
    }
  /* Fill from middle to bottom. */
  while (y < win->height - 1)
    {
      win->line_info[y].len = -1;
      win->line_info[y].n = -1;
      win->line_info[y].part = -1;
      win->line_info[y].text = NULL;
      y++;
    }
  return 0;
}

/**
 * @brief Move the cursor to specified line.
 *
 * Move the cursor to the specified line and redraw the lines.
 *
 * @param win The window.
 * @param n The new cursor position.
 *
 * @return 0 upon success.
 */
int
win_move_cursor (Win * win, int n)
{
  int ret;

  assert (win);

  LOG_ERR_IF_RETURN ((win->props.properties & WIN_PROP_CURSOR) == 0, -1,
		     "No cursor window.");

  if (text_nr_of_lines (win->text) == 0)
    {
      return 0;
    }
  if (n + win->cursor_pos < 0)
    {
      n = 0;
    }
  else if (n + win->cursor_pos >= text_nr_of_lines (win->text))
    {
      n = text_nr_of_lines (win->text) - 1;
    }
  else
    {
      n += win->cursor_pos;
    }

  DINFO (5, "Moving cursor to line %d, top %d bottom %d", n,
	 win->line_info[0].n, win->line_info[win->height - 2].n);

  ret = win_update_cursor (win, n);

  LOG_ERR_IF_RETURN (ret < 0, -1, "Could not update cursor pos");

  /* Show new cursor pos. */
  if (win->line_info[0].n > n
      || (win->line_info[0].n == n && win->line_info[0].part > 0))
    {
      if (win->line_info[0].n == n + 1)
	{
	  ret = win_scroll (win, -1);
	}
      else
	{
	  ret = win_redraw_show_top (win, n);
	}
    }
  else if (win->line_info[win->height - 2].n < n
	   && win->line_info[win->height - 2].n >= 0)
    {
      if (win->line_info[win->height - 2].n == n - 1)
	{
	  ret = win_scroll (win, 1);
	}
      else
	{
	  ret = win_redraw_show_bottom (win, n);
	}
    }

  return ret;
}

/**
 * @brief Move the window.
 *
 * Move the window/cursor. If the window has a cursor the cursor position is
 * changed and the window is scroll if the cursor is outside the window. If the
 * window has no cursor, the window is scrolled.
 *
 * @param win The window.
 * @param n The number of step to move.
 *
 * @return 0 upon success.
 */
int
win_move (Win * win, int n)
{
  int ret;

  assert (win);

  if (win->props.properties & WIN_PROP_CURSOR)
    {
      ret = win_move_cursor (win, n);
    }
  else
    {
      ret = win_scroll (win, n);
    }

  return ret;
}

/**
 * @brief Set the focus for a window.
 *
 * Set the focus to @a focus for the window. Update the status line reflecting
 * the new focus.
 *
 * @param win The window.
 * @param focus 1 if the window should have focus. 0 if not.
 */
void
win_set_focus (Win * win, int focus)
{
  assert (win);

  if (focus == win->focus)
    {
      return;
    }

  win->focus = focus;
  win_redraw_status (win);
}

/**
 * @brief Clear the window.
 *
 * Clear the window and clear the text.
 *
 * @param win The window to be cleared.
 *
 * @return 0 on success. -1 on failure.
 */
void
win_clear (Win * win)
{
  int i;

  assert (win);

  DINFO (5, "Clearing window");
  for (i = 0; i < win->height - 1; i++)
    {
      win->line_info[i].len = -1;
      win->line_info[i].n = -1;
      win->line_info[i].part = -1;
      win->line_info[i].text = NULL;
    }

  wclear (win->window);
  win_redraw_status (win);

  if (win->file_name != NULL)
    {
      free (win->file_name);
      win->file_name = NULL;
    }

  text_clear (win->text);
}

/**
 * @brief Set a marker in window.
 *
 * Set a marker in the window at a given line number and mark position.
 *
 * @param win The window.
 * @param line The line number to set the marker.
 * @param nr The mark nr. First position is 0. Can not be larger than the
 *           indent - 1.
 * @param mark The mark itself. Must be printable.
 *
 * @return 0 if the marker is set. Otherwise -1.
 */
int
win_set_mark (Win * win, int line, int nr, char mark)
{
  assert (win);

  LOG_ERR_IF_RETURN (!(win->props.properties & WIN_PROP_MARKS), -1,
		     "Window does not support marks");
  LOG_ERR_IF_RETURN (line >= text_nr_of_lines (win->text), -1,
		     "Line %d out of bounds", line);
  LOG_ERR_IF_RETURN (nr >= win->props.indent || nr < 0, -1,
		     "Mark %d out of bounds", nr);
  LOG_ERR_IF_RETURN (!isprint (mark), -1, "Not printable mark '0x%02X'",
		     mark);

  if (line < 0)
    {
      line = text_nr_of_lines (win->text) - 1;
    }
  win->text_line_info[line].marks[nr] = mark;
  DINFO (1, "Set Mark to '%s' at line %d", win->text_line_info[line].marks,
	 line);
  win_draw_text_line (win, line);

  return 0;
}

/**
 * @brief Get the tag of the current line.
 *
 * The tag under the cursor.
 *
 * @param win The window.
 *
 * @return The tag.
 */
int
win_get_tag (Win * win)
{
  assert (win);

  /* Return -1 if cursor is out of bounds. */
  LOG_ERR_IF_RETURN (win->cursor_pos < 0
		     || win->cursor_pos >= text_nr_of_lines (win->text), -1,
		     "Current cursor not set.");

  return win->text_line_info[win->cursor_pos].tag;
}

/**
 * @brief Get the current cursor position.
 *
 * Get the current cursor position if the window has a cursor.
 *
 * @param win The window.
 *
 * @return The cursor position if the window has a cursor, otherwise -1.
 */
int
win_get_cursor (Win * win)
{
  assert (win);

  return (win->props.properties & WIN_PROP_CURSOR) ==
    0 ? -1 : win->cursor_pos;
}

/**
 * @brief Get the filename associated with the window.
 *
 * Returns the filename associated with the window.
 *
 * @param win The window.
 *
 * @return If the window is associated with a file, return the file name. If
 *         the window is not loaded from a file, return NULL.
 */
const char *
win_get_filename (Win * win)
{
  assert (win);

  return win->file_name;
}

/**
 * @brief Get text line.
 *
 * Get the text line with line number @a line_nr.
 *
 * @param win The window.
 * @param line_nr The line number.
 *
 * @return The line if it exists.
 */
const char *
win_get_line (Win * win, int line_nr)
{
  int len;

  assert (win);

  return text_get_line (win->text, line_nr, &len);
}

/**
 * @brief Dump win conten to stdout.
 *
 * @param win The window.
 */
void
win_dump (Win * win)
{
  assert (win);

  text_dump (win->text);
}
