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
 * @file win_form.c
 *
 * @brief Implementation of the form window.
 *
 * Implements the form window used to retrieve user input.
 */
#include <ncurses.h>
#include <panel.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "win_form.h"
#include "lvdbg.h"
#include "win_handler.h"
#include "debug.h"

#define HELP_STRING _("Use keys <UP> and <DOWN> to select item and <ENTER> " \
		       "to chose item. <ESC> or `q' to cancel.")
#define ENT_INCREASE 20
/*******************************************************************************
 * Internal Functions
 ******************************************************************************/
static void form_wait (void);
static char *form_get_input (const char *value, const char *header);
static int form_draw (input_field * fields, const char *header, WINDOW * win,
		      int height, int width, int indent, int start_index,
		      int selected);
static int form_exec (input_field * fields, const char *header, WINDOW * win,
		      int height, int width, int indent);
static int form_exec_selection (Win * win, char **list);
static int form_exec_file (Win * win, char **list, const char *header);

/**
 * @brief Wait for user input.
 *
 * Wait for user action, by doing a select on stdin.
 */
static void
form_wait (void)
{
  int ret;
  fd_set rfds;

  FD_ZERO (&rfds);
  FD_SET (0, &rfds);

  ret = select (1, &rfds, NULL, NULL, NULL);
}

/**
 * @brief Get user input.
 *
 * Draws a box and get user input. The input is ended by '@<ESC@>' or '@<CR@>'.
 * '@<CTRL-a@>' moves cursor to beginning of line. '@<CTRL-e@>' moves cursor to
 * end of line. The user does not need to be at end of line while pressing
 * '@<CR@>'.
 *
 * @param value The default value that is shown.
 * @param header A small help text shown in the box.
 *
 * @return A pointer to a new value if user pressed @<CR@>. If the user pressed
 *         @<ESC@> a NULL pointer is returned.
 */
static char *
form_get_input (const char *value, const char *header)
{
  char *out;
  int len;
  WINDOW *win;
  int max_width;
  int max_height;
  int c;
  int ret;
  int index = 0;
  int end;
  int x;
  int y;
  int i;
  int insert = 1;
  int part = 0;
  int width;

  curs_set (1);

  if (value == NULL)
    {
      len = 64;
      end = 0;
    }
  else
    {
      len = 64 + strlen (value);
      end = strlen (value);
    }
  out = (char *) malloc (len);
  if (value != NULL)
    {
      sprintf (out, "%s", value);
    }
  else
    {
      out[0] = '\0';
    }
  getmaxyx (stdscr, max_height, max_width);
  width = max_width / 4 - 2;
  win = newwin (3, max_width / 4, max_height / 2 - 1,
		max_width / 2 - max_width / 8);
  box (win, 0, 0);
  keypad (win, 1);
  if (header != NULL)
    {
      ret = mvwaddnstr (win, 0, 1, header, max_width / 8);
    }
  c = 0;
  x = 1;
  y = 1;
  part = 0;
  while (c != '\r')
    {
      ret = mvwaddnstr (win, 1, 1, out + part * width, width);
      wclrtoeol (win);
      box (win, 0, 0);
      wmove (win, y, x);
      wnoutrefresh (win);
      doupdate ();
      form_wait ();
      c = getch ();
      if (c == '\r')
	{
	  break;
	}
      if (c == KEY_RIGHT)
	{
	  if (index < end)
	    {
	      index++;
	      x++;
	      if (x > width)
		{
		  x = 1;
		  part++;
		}
	    }
	}
      else if (c == 1)
	{
	  index = 0;
	  x = 1;
	  part = 0;
	}
      else if (c == 5)
	{
	  index = end;
	  x = end % width + 1;
	  part = end / width;
	}
      else if (c == '\x1b')
	{
	  free (out);
	  out = NULL;
	  break;
	}
      else if (c == KEY_DC && index != end)
	{
	  for (i = index; i < end; i++)
	    {
	      out[i] = out[i + 1];
	    }
	  end--;
	}
      else if (c == KEY_IC)
	{
	  insert = insert ? 0 : 1;
	}
      else if (c == KEY_LEFT)
	{
	  if (index > 0)
	    {
	      index--;
	      x--;
	      if (x == 0)
		{
		  part--;
		  x = width;
		}
	    }
	}
      else if (c == KEY_BACKSPACE)
	{
	  if (index > 0)
	    {
	      for (i = index - 1; out[i] != '\0'; i++)
		{
		  out[i] = out[i + 1];
		}
	      index--;
	      x--;
	      if (x == 0)
		{
		  part--;
		  x = width;
		}
	      end--;
	    }
	}
      else if (isprint (c))
	{
	  if (insert)
	    {
	      for (i = end + 1; i > index; i--)
		{
		  out[i] = out[i - 1];
		}
	      end++;
	      if (end == len - 2)
		{
		  out = (char *) realloc (out, len + 64);
		  if (out == NULL)
		    {
		      return NULL;
		    }
		  len += 64;
		}
	      out[index++] = c;
	      x++;
	      if (x > width)
		{
		  part++;
		  x = 1;
		}
	    }
	  else
	    {
	      if (index == end)
		{
		  end++;
		  out[end] = '\0';
		  if (end == len - 2)
		    {
		      out = (char *) realloc (out, len + 64);
		      if (out == NULL)
			{
			  return NULL;
			}
		      len += 64;
		    }
		}
	      out[index++] = c;
	      x++;
	      if (x > width)
		{
		  part++;
		  x = 1;
		}
	    }
	}
      else
	{
	  DINFO (1, "KEY %d", c);
	}
    }
  wclear (win);
  wnoutrefresh (win);
  doupdate ();
  delwin (win);
  noecho ();
  curs_set (0);
  return out;
}

/**
 * @brief Draw the input form.
 *
 * Draw the input form.
 *
 * @param fields The fields to be drawn.
 * @param header A label header to be draw in the box.
 * @param win The WINDOW to draw in.
 * @param height The height of the window.
 * @param width The width of the window.
 * @param indent The column to start draw the field values.
 * @param start_index The start index of the @a fields to be drawn.
 * @param selected The index of the current selected item. If -1 'OK' is
 *        selected. If -2 'Cancel' is selected.
 *
 * @return 0 upon success.
 */
static int
form_draw (input_field * fields, const char *header, WINDOW * win, int height,
	   int width, int indent, int start_index, int selected)
{
  int y = 1;
  int i = start_index;
  char buf[512];
  char *p = buf;
  int left;
  int ret;

  if (width + 1 > 512)
    {
      p = (char *) malloc (width + 1);
      LOG_ERR_IF_RETURN (p == NULL, -1, "Could not create line");
    }

  y += 2;
  while (y < height - 2)
    {
      if (fields[i].text == NULL)
	{
	  break;
	}
      if (i == selected)
	{
	  wattron (win, A_REVERSE);
	}
      ret = mvwaddstr (win, y, 2, fields[i].text);
      left = width - indent - 1;
      switch (fields[i].type)
	{
	case INPUT_TYPE_INT:
	  snprintf (p, left, "%d", fields[i].int_value);
	  break;
	case INPUT_TYPE_STRING:
	  snprintf (p, left, "%s",
		    fields[i].string_value ==
		    NULL ? "" : fields[i].string_value);
	  break;
	case INPUT_TYPE_BOOL:
	  snprintf (p, left, "%s", fields[i].bool_value ? "YES" : "NO");
	  break;
	case INPUT_TYPE_ENUM:
	  snprintf (p, left, "%s", fields[i].enum_text[fields[i].enum_value]);
	  break;
	default:
	  return -1;
	}
      ret = mvwaddstr (win, y, indent + 1, p);
      wclrtoeol (win);
      if (i == selected)
	{
	  wattroff (win, A_REVERSE);
	}
      i++;
      y++;
    }

  if (selected == -1)
    {
      wattron (win, A_REVERSE);
    }
  ret = mvwaddstr (win, height - 3, width / 2 - 1, "OK");
  if (selected == -1)
    {
      wattroff (win, A_REVERSE);
    }
  if (selected == -2)
    {
      wattron (win, A_REVERSE);
    }
  ret =
    mvwaddstr (win, height - 2, width / 2 - strlen ("Cancel") / 2, "Cancel");
  if (selected == -2)
    {
      wattroff (win, A_REVERSE);
    }

  if (p != buf)
    {
      free (p);
    }
  box (win, 0, 0);
  mvwaddnstr (win, 1, 1, header, width);
  wnoutrefresh (win);
  doupdate ();
  return 0;
}

/**
 * @brief Execute the input form.
 *
 * Execute the input form. Draw the form and wait for user action.  To change
 * a value '@<CR@>' is pressed. For integer and string types a new window is
 * opened and the user can enter a value. For bool and enum, enter changes
 * the values to the next in line. User can also press '@<LEFT@>' and '@<RIGHT@>'
 * keys to change values of bools and enums.
 *
 * Leave the dialog by either 'OK', 'Cancel' or pressing '@<ESC@>'. OK returns 0,
 * and the later two returns -1.
 *
 * @param fields The fields of the form.
 * @param header Header text to be shown.
 * @param win The win to draw in.
 * @param height Height of the @a win.
 * @param width Width of the @a win.
 * @param indent First column to draw the values in.
 *
 * @return -1 if users pressed 'Cancel' or '@<ESC@>'. 0 if OK was pressed.
 */
static int
form_exec (input_field * fields, const char *header, WINDOW * win, int height,
	   int width, int indent)
{
  int ret;
  int index = 0;
  int selected = 0;
  int c;
  int done = 0;
  int on_ok = 0;
  int on_cancel = 0;
  char *p;
  char buf[64];
  int i;
  input_field *f;

  /* Mark string value as original, that should not be freed. */
  f = fields;
  while (f && f->text)
    {
      if (f->type == INPUT_TYPE_STRING)
	{
	  f->max = 1;		/* Max is unused for strings. A bit of a hack... */
	}
      f++;
    }

  box (win, 0, 0);
  while (!done)
    {
      ret = form_draw (fields, header, win, height, width, indent, index,
		       on_ok ? -1 : (on_cancel ? -2 : selected));
      form_wait ();
      c = getch ();
      DINFO (10, "%X: %d %d", c, on_ok, selected);
      switch (c)
	{
	case '\x1b':
	case 'q':
	  ret = -1;
	  done = 1;
	  break;
	case KEY_DOWN:
	  if (fields[selected].text != NULL)
	    {
	      selected++;
	      if (fields[selected].text == NULL)
		{
		  on_ok = 1;
		}
	    }
	  else
	    {
	      on_ok = 0;
	      on_cancel = 1;
	    }
	  break;
	case KEY_UP:
	  if (on_cancel)
	    {
	      on_cancel = 0;
	      on_ok = 1;
	    }
	  else if (selected > 0)
	    {
	      selected--;
	      on_ok = fields[selected].text == NULL;
	    }
	  break;
	case KEY_RIGHT:
	  if (fields[selected].type == INPUT_TYPE_BOOL)
	    {
	      fields[selected].bool_value =
		fields[selected].bool_value ? 0 : 1;
	    }
	  else if (fields[selected].type == INPUT_TYPE_ENUM)
	    {
	      fields[selected].enum_value++;
	      if (fields[selected].enum_text[fields[selected].enum_value] ==
		  NULL)
		{
		  fields[selected].enum_value = 0;
		}
	    }
	  break;
	case KEY_LEFT:
	  if (fields[selected].type == INPUT_TYPE_BOOL)
	    {
	      fields[selected].bool_value =
		fields[selected].bool_value ? 0 : 1;
	    }
	  else if (fields[selected].type == INPUT_TYPE_ENUM)
	    {
	      fields[selected].enum_value--;
	      if (fields[selected].enum_value < 0)
		{
		  i = 0;
		  while (fields[selected].enum_text[i] != NULL)
		    {
		      i++;
		    }
		  fields[selected].enum_value = i - 1;
		}
	    }
	  break;

	case '\r':
	  if (on_ok)
	    {
	      ret = 0;
	      done = 1;
	      break;
	    }
	  else if (on_cancel)
	    {
	      ret = -1;
	      done = 1;
	      break;
	    }
	  if (fields[selected].type == INPUT_TYPE_STRING)
	    {
	      p = form_get_input (fields[selected].string_value,
				  fields[selected].help);
	      if (p != NULL)
		{
		  if (fields[selected].string_value != NULL
		      && fields[selected].max == 0)
		    {
		      free (fields[selected].string_value);
		    }
		  fields[selected].string_value = p;
		  fields[selected].max = 0;
		}
	    }
	  else if (fields[selected].type == INPUT_TYPE_INT)
	    {
	      snprintf (buf, 64, "%d", fields[selected].int_value);
	      p = form_get_input (buf, fields[selected].help);
	      if (p != NULL)
		{
		  fields[selected].int_value = strtol (p, NULL, 0);
		}
	      free (p);
	    }
	  else if (fields[selected].type == INPUT_TYPE_BOOL)
	    {
	      fields[selected].bool_value =
		fields[selected].bool_value ? 0 : 1;
	    }
	  else if (fields[selected].type == INPUT_TYPE_ENUM)
	    {
	      fields[selected].enum_value++;
	      if (fields[selected].enum_text[fields[selected].enum_value] ==
		  NULL)
		{
		  fields[selected].enum_value = 0;
		}
	    }
	}
    }
  return ret;
}

/**
 * @brief Make selection.
 *
 * Clrears the window and draws a help text and the items in list. Let user
 * select and chose an item.
 *
 * @param win The window.
 * @param list The list of items.
 *
 * @return -2 on error. -1 if canceld. >=0 the selected index from the @a list.
 */
static int
form_exec_selection (Win * win, char **list)
{
  char **p;
  int count;
  int ret = 0;
  int c;

  assert (list);

  win_to_top (win);
  win_clear (win);

  ret = win_add_line (win, HELP_STRING, 1, -1);
  LOG_ERR_IF_RETURN (ret < 0, -2, "Could not add lines.");
  /* Draw list items. */
  p = list;
  count = 0;
  while (*p)
    {
      ret = win_add_line (win, *p, 1, count++);
      LOG_ERR_IF_RETURN (ret < 0, -2, "Could not add lines.");
      p++;
    }
  win_go_to_line (win, 1);

  ret = 0;
  do
    {
      update_panels ();
      doupdate ();
      /* Wait for input. */
      form_wait ();
      c = getch ();
      switch (c)
	{
	case 'q':
	case '\x1b':		/* <ESC> */
	  return -1;
	  break;
	case KEY_UP:
	  win_move_cursor (win, -1);
	  break;
	case KEY_DOWN:
	  win_move_cursor (win, 1);
	  break;
	case '\r':
	  ret = win_get_tag (win);
	  if (ret < 0)
	    {
	      /* Help text... */
	      continue;
	    }
	  return ret;
	}
      refresh ();
    }
  while (1);

  /* Never reached. */
  return -1;
}

/**
 * @brief Execute the form for file selection.
 *
 * Set up a form for file selection.
 *
 * @param win The win where the form should be shown.
 * @param list Where the selected file is stored, if any.
 * @param header Header shown in the form.
 *
 * @return -2 on error. -1 if user canceled. 0 if selected a file.
 */
static int
form_exec_file (Win * win, char **list, const char *header)
{
  int ret;
  DIR *dir;
  char **items = NULL;
  char **tmp;
  char *stmp;
  int entries = 0;
  int size = 0;
  char buf[512];
  int buf_len = 512;
  int needed;
  char *dname = buf;
  struct dirent *entry;
  int i;

  win_set_status (win, header);
  win_set_focus (win, 1);
  dname[0] = '.';
  dname[1] = '\0';
  do
    {
      DINFO (1, "Open: '%s'", dname);
      dir = opendir (dname);
      if (dir == NULL)
	{
	  if (errno == ENOTDIR && strlen (dname) > 1)
	    {
	      /* Ok hopefully it is a file. Need to check with stat? */
	      ret = 0;
	    }
	  else
	    {
	      ret = -2;
	    }
	  goto out;
	}

      /* Read the dirs and filename from dir. */
      entries = 0;
      while ((entry = readdir (dir)) != NULL)
	{
	  if (entries + 1 >= size)	/* One extra for NULL */
	    {
	      /* Make room for more entries */
	      tmp = (char **) realloc (items,
				       (size +
					ENT_INCREASE) * sizeof (char *));
	      if (tmp == 0)
		{
		  ret = -2;
		  closedir (dir);
		  goto out;
		}
	      items = tmp;
	      memset (&items[size], 0, ENT_INCREASE * sizeof (char *));
	      size += ENT_INCREASE;
	    }
	  if (errno == EBADF)
	    {
	      ret = -2;
	      goto out;
	    }
	  if (items[entries] != NULL)
	    {
	      /* Check if new name fits in old place. */
	      if (strlen (items[entries]) - 1 < strlen (entry->d_name))
		{
		  free (items[entries]);
		  items[entries] = strdup (entry->d_name);
		}
	      else
		{
		  sprintf (items[entries], "%s", entry->d_name);
		}
	    }
	  else
	    {
	      items[entries] = strdup (entry->d_name);
	    }
	  if (items[entries] == NULL)
	    {
	      ret = -2;
	      closedir (dir);
	      goto out;
	    }
	  entries++;
	}
      closedir (dir);


      /* NULL terminate list. */
      if (items[entries])
	{
	  free (items[entries]);
	  items[entries] = NULL;
	}

      ret = form_exec_selection (win, items);
      if (ret < 0)
	{
	  goto out;
	}

      /* Does dname + '/' + selected name fit in old ? */
      needed = strlen (dname) + strlen (items[ret]) + 2;
      if (buf_len < needed)
	{
	  if (dname == buf)
	    {
	      dname = NULL;
	    }
	  stmp = (char *) realloc (dname, needed);
	  if (stmp == NULL)
	    {
	      ret = -2;
	      goto out;
	    }
	  if (dname == NULL)
	    {
	      strcpy (dname, buf);
	    }
	  dname = stmp;
	}

      /* Append name to dir */
      strcat (dname, "/");
      strcat (dname, items[ret]);
    }
  while (1);

out:
  for (i = 0; i < size; i++)
    {
      if (items[i] != NULL)
	{
	  free (items[i]);
	}
    }
  if (ret < 0)
    {
      if (dname != buf)
	{
	  free (dname);
	}
    }
  else
    {
      /* Let the caller get the name of selected file. */
      if (dname == buf)
	{
	  dname = strdup (buf);
	  if (dname == NULL)
	    {
	      ret = -2;
	    }
	}
      *list = dname;
    }
  return ret;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Set up a form and execute it.
 *
 * Set up the window to be used to display the form. The values in the @a fields
 * are updated when leaving the form.
 *
 * NOTE: It is the caller who need to free _all_ string values. The initial
 *       string value might be discarded and it is the caller who needs to
 *       free it.
 *
 * @param fields Fields of the form.
 * @param header Header to show in the form.
 *
 * @return -1 if user canceled the form. 0 if user pressed ok.
 */
int
form_run (input_field * fields, const char *header)
{
  int width;
  int height;
  int starty;
  int startx;
  int max_height;
  int max_width;
  int max_text;
  int max_enum;
  int len;
  int ret;
  int indent;
  const char **p;
  WINDOW *win;

  input_field *pi;

  assert (fields);

  /* Get the max width of the 'text' of the fields. */
  pi = fields;
  max_text = 0;
  max_enum = 0;
  len = 0;
  while (pi != NULL && pi->text)
    {
      len++;
      if (strlen (pi->text) > max_text)
	{
	  max_text = strlen (pi->text);
	}
      if (pi->type == INPUT_TYPE_STRING && pi->string_value != NULL
	  && strlen (pi->string_value) > max_enum)
	{
	  max_enum = strlen (pi->string_value);
	}
      p = pi->enum_text;
      while (p && *p)
	{
	  if (strlen (*p) > max_enum)
	    {
	      max_enum = strlen (*p);
	    }
	  p++;
	}
      pi++;
    }
  assert (max_text > 0);
  if (max_enum == 0)
    {
      max_enum = 10;
    }
  indent = max_text + 2;

  /* Get screen dimensions. */
  getmaxyx (stdscr, max_height, max_width);

  LOG_ERR_IF_RETURN (max_height < 4 || max_width - 2 < 2 * max_text, -1,
		     "Screen too small");

  if (len < max_height * 0.8 - 7)
    {
      height = len + 7;
    }
  else
    {
      height = max_height * 0.8;
    }
  if (strlen (header) > max_text + max_enum + 5)
    {
      width = strlen (header);
    }
  else
    {
      width = max_text + max_enum + 5;
    }
  starty = max_height / 2 - height / 2;
  startx = max_width / 2 - width / 2;
  win = newwin (height, width, starty, startx);

  DINFO (1, "Running form '%s' %dx%d at (%d; %d)", header, height, width,
	 starty, startx);

  ret = form_exec (fields, header, win, height, width, indent);

  wclear (win);
  wnoutrefresh (win);
  delwin (win);
  refresh ();
  doupdate ();

  return ret;
}

/**
 * @brief Make a selection form.
 *
 * Make a form where the user select an item from a lists. If first element in
 * @a list is NULL, the form shows a list of files.
 *
 * @param list The list of items. Last item should be NULL. If first element is
 * NULL a file selection form is shown and the filename chosen is stored in @a
 * list. Caller must free the returned filename.
 * @param header Header to be shown.
 *
 * @return -1 if the user canceled, otherwise the index of the item selected. -2
 * on failure.
 */
int
form_selection (char **list, const char *header)
{
  int width;
  int height;
  int starty;
  int startx;
  int max_height;
  int max_width;
  int ret;
  Win *win;
  win_properties props =
    { 0, WIN_PROP_CURSOR | WIN_PROP_BORDER, NULL, 0, NULL };

  assert (list);
  assert (header);

  /* Get screen dimensions. */
  getmaxyx (stdscr, max_height, max_width);

  height = max_height * 0.5;
  width = max_width / 3;
  starty = max_height / 2 - height / 2;
  startx = max_width / 2 - width / 2;

  win = win_create (starty, startx, height, width, &props);
  LOG_ERR_IF_RETURN (win == NULL, -2, "Could not create selection window");

  DINFO (1, "Running form selection '%s' %dx%d at (%d; %d)", header, height,
	 width, starty, startx);

  if (*list == NULL)
    {
      ret = form_exec_file (win, list, header);
    }
  else
    {
      win_set_status (win, header);
      ret = form_exec_selection (win, list);
    }

  win_free (win);
  return ret;
}
