#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>

#include "../src/view.h"
#include "../src/objects.h"

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;
char *GLOB_STR = NULL;
char *GLOB_PAIR = NULL;
char *GLOB_COLOR_STR = NULL;
char *GLOB_GROUPS = NULL;

void
setup (void)
{
}

void
teardown (void)
{
}

/* Overloaded functions. */
const char *
conf_get_string (configuration * conf, const char *group_name,
		 const char *name, int *valid)
{
  if (strcmp (group_name, "Syntax"))
    {
      return GLOB_STR;
    }

  if (strcmp (name, "groups") == 0)
    {
      return GLOB_GROUPS;
    }
  return strcmp (name, "colors") == 0 ? GLOB_PAIR : GLOB_COLOR_STR;
}

START_TEST (test_view_create)
{
  view *view;
  int ret;

  GLOB_STR = strdup ("{name='maaa'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret < 0);
  free (GLOB_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;

  GLOB_STR = strdup ("{name=?maaa?}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret < 0);
  free (GLOB_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;


  GLOB_STR = strdup ("\"cols='2',name='Main'\"," "{name='Messages'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret < 0);
  free (GLOB_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;

  GLOB_STR = strdup ("?cols='2',name='Main'?," "{name='Messages'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret < 0);
  free (GLOB_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;

  GLOB_STR = strdup ("{cooool='A',name='Main'}," "{name='Messages'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret < 0);
  free (GLOB_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;

  GLOB_STR = strdup ("{cols='A',name='Main'}," "{name='Messages'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret < 0);
  free (GLOB_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;

  GLOB_STR = strdup ("view={{height='50',name='Main'}," "{name='Messages'}}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);

  GLOB_STR = NULL;
  GLOB_PAIR = strdup ("{fg_color='1',bg_color='2'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);

  GLOB_STR = NULL;
  GLOB_PAIR = strdup ("{fg_color='K',bg_color='2'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_STR = NULL;
  GLOB_PAIR = strdup ("{fg_color='2',bg_color='K'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_STR = NULL;
  GLOB_PAIR = strdup ("[fg_color='K',bg_color='2']");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_STR = NULL;
  GLOB_PAIR = strdup ("{KKfg_color='1',bg_color='2'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_STR = NULL;
  GLOB_PAIR = strdup ("{[id='3']}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);

  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_COLOR_STR = strdup ("{color='1',attr='0x0'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_COLOR_STR = strdup ("{color='7',attr='0x0'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_COLOR_STR = strdup ("{color='k',attr='0x0'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_COLOR_STR = strdup ("{color='1',attr='K0x0'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_COLOR_STR = strdup ("[color='k',attr='0x0']");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_COLOR_STR = strdup ("{[caolor='1',attr='0x0']}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_COLOR_STR = strdup ("{caolor='1',attr='0x0'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);

  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_GROUPS = strdup ("Main={id='1',match='int'}");
  GLOB_COLOR_STR = strdup ("{color='1',attr='0x0'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  free (GLOB_GROUPS);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);
  GLOB_PAIR = strdup ("{fg_color='2',bg_color='1'}");
  GLOB_GROUPS = strdup ("MOON={id='1',match='int'}");
  GLOB_COLOR_STR = strdup ("{color='1',attr='0x0'}");
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  free (GLOB_PAIR);
  free (GLOB_COLOR_STR);
  free (GLOB_GROUPS);
  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  view_cleanup (view);

  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  view_cleanup (view);
}
END_TEST

START_TEST (test_view_misc)
{
  view *view;
  int ret;
  int i;

  GLOB_STR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  ret = view_add_line (view, 0, "kk\001", 1);
  fail_unless (ret < 0);

  ret = view_add_line (view, 0, "kk", 1);
  fail_unless (ret == 0);

  ret = view_show_file (view, "", 15, 1);
  fail_unless (ret == 0);
  ret = view_show_file (view, CONFDIR "text_test.txt", 15, 1);
  fail_unless (ret == 0);
  ret = view_show_file (view, CONFDIR "text_test.txt", 13, 1);
  fail_unless (ret == 0);

  ret = view_scroll_up (view);
  fail_unless (ret == 0);
  ret = view_scroll_down (view);
  fail_unless (ret == 0);

  for (i = 0; i < 15; i++)
    {
      ret = view_next_window (view, 1, 0);
      fail_unless (ret == 0);
    }

  for (i = 0; i < 15; i++)
    {
      ret = view_next_window (view, -1, 0);
      fail_unless (ret == 0);
    }

  for (i = 0; i < 15; i++)
    {
      ret = view_next_window (view, 1, 1);
      fail_unless (ret == 0);
    }

  for (i = 0; i < 15; i++)
    {
      ret = view_next_window (view, -1, 1);
      fail_unless (ret == 0);
    }

  for (i = 0; i < 15; i++)
    {
      ret = view_next_window (view, 1, 2);
      fail_unless (ret == 0);
    }

  for (i = 0; i < 15; i++)
    {
      ret = view_next_window (view, -1, 2);
      fail_unless (ret == 0);
    }

  ret = view_set_status (view, 0, "Hello");
  fail_unless (ret == 0);

  ret = view_add_message (view, 0, "Hello");
  fail_unless (ret == 0);
  ret = view_add_message (view, 0, "Hel\x01o");
  fail_unless (ret < 0);

  view_cleanup (view);
}
END_TEST

START_TEST (test_view_objects_thread)
{
  view *view;
  int ret;
  int i;
  int j;
  thread *pt;

  thread_group *tg = NULL;

  GLOB_STR = NULL;

  /* Create view. */
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  /* Test with no threads. */
  view_update_threads (view, tg);

  /* The some thread groups. */
  for (i = 0; i < 5; i++)
    {
      ret = thread_group_add (&tg, i);
      fail_unless (ret == 0);
      for (j = 0; j < i + 2; j++)
	{
	  ret = thread_add (tg, i, j);
	  fail_unless (ret == 0);
	}
    }
  pt = thread_group_get_thread (tg, 0, 1);
  pt->frame.file = strdup ("file");
  pt->frame.fullname = strdup ("full");
  pt->frame.func = strdup ("func");

  view_update_threads (view, tg);

  /* Release resources. */
  thread_group_remove_all (&tg);
  view_cleanup (view);
}
END_TEST

START_TEST (test_view_objects_library)
{
  view *view;
  char buf[64];
  int ret;
  int i;
  int j;
  library *l = NULL;

  GLOB_STR = NULL;

  /* Create view. */
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  /* Test with no library. */
  view_update_libraries (view, l);

  /* Set up with a couple of libraries. */
  ret = library_add (&l, "BOOK1", "TARG", "HOST", 1);
  fail_unless (ret == 0);
  ret = library_add (&l, "BOOK2", "TARG", "HOST", 0);
  fail_unless (ret == 0);
  ret = library_add (&l, "BOOK2", "TARG", NULL, 0);
  fail_unless (ret == 0);
  ret = library_add (&l, "BOOK2", NULL, "HOST", 0);
  fail_unless (ret == 0);
  ret = library_add (&l, "BOOK2", NULL, NULL, 0);
  fail_unless (ret == 0);
  view_update_libraries (view, l);

  library_remove_all (&l);
  view_cleanup (view);
}
END_TEST

START_TEST (test_view_objects_breakpoint)
{
  view *view;
  char buf[64];
  int ret;
  int i;
  int j;
  breakpoint_table *bpt = NULL;
  breakpoint *bp;

  GLOB_STR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);
  ret = view_show_file (view, CONFDIR "text_test.txt", 15, 1);
  fail_unless (ret == 0);

  bpt = bp_table_create ();
  fail_unless (bpt != NULL);

  /* No breakpoints. */
  view_update_breakpoints (view, bpt);

  /* With breakpoints defined. */
  bp = bp_create ();
  bp->func = strdup ("FUNC");
  bp->file = strdup ("test.txt");
  bp->line = 15;
  bp->fullname = strdup (CONFDIR "text_test.txt");
  /* Test with long breakpoints outputs so we need to allocate more. */
  bp->cond = strdup ("cond                                    "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                       1");
  bp->number = 2;
  bp_table_insert (bpt, bp);
  bp = bp_create ();
  bp->func = strdup ("FUNC");
  bp->file = strdup ("test.txt");
  bp->line = 13;
  bp->fullname = strdup (CONFDIR "text_test.txt");
  bp->cond = strdup ("cond                                    "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                        "
		     "                                       1");
  bp->number = 3;
  bp_table_insert (bpt, bp);
  bp = bp_create ();
  bp->func = strdup ("FUNC");
  bp->file = strdup ("FILE");
  bp->fullname = strdup ("FULL");
  bp->cond = NULL;
  bp->number = 5;
  bp_table_insert (bpt, bp);
  bp = bp_create ();
  bp->func = strdup ("FUNC");
  bp->file = strdup ("FILE");
  bp->fullname = strdup ("FULL");
  bp->type = BP_TYPE_WATCHPOINT;
  bp->value = strdup ("...");
  bp->expression = strdup ("5 + x");
  bp->cond = NULL;
  bp->number = 5;
  bp_table_insert (bpt, bp);

  view_update_breakpoints (view, bpt);

  /* Remove breakpoint */
  view_remove_breakpoint (view, NULL, 5);
  view_remove_breakpoint (view, "SOMETHING", 5);
  view_remove_breakpoint (view, CONFDIR "text_test.txt", 5);
  view_remove_breakpoint (view, CONFDIR "text_test.txt", 15);

  bp_table_free (bpt);

  view_cleanup (view);
}
END_TEST

START_TEST (test_view_objects_stack)
{
  view *view;
  int ret;
  int i;
  frame *f = NULL;
  stack *stack = NULL;

  /* Setup the view. */
  GLOB_STR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  stack = stack_create (10);
  fail_unless (stack != NULL);
  view_update_stack (view, stack);
  view_update_frame (view, stack, 0);

  f = stack_get_frame (stack, 0);
  f->addr = 1;
  f->line = 1;
  f->file = strdup ("file");
  f->fullname = NULL;
  f->func = strdup ("func");
  stack->depth = 1;
  ret = frame_insert_variable (f, "id", NULL, "VAL", 1, 1);
  fail_unless (ret == 0);
  ret = frame_insert_variable (f, "id", NULL, "VAL", 0, 1);
  fail_unless (ret == 0);
  view_update_stack (view, stack);
  view_update_frame (view, stack, 0);
  stack_clean_frame (stack, -1);

  f = stack_get_frame (stack, 0);
  f->line = 10;
  fail_unless (f != NULL);
  f->addr = -1;
  f->file = strdup ("file");
  f->fullname = NULL;
  f->func = strdup ("func");
  view_update_frame (view, stack, 0);
  f = stack_get_frame (stack, 1);
  fail_unless (f != NULL);
  f->addr = -1;
  f->line = 5;
  f->file = strdup ("file");
  f->fullname = NULL;
  f->func = strdup ("func");
  view_update_frame (view, stack, 1);
  f = stack_get_frame (stack, 2);
  fail_unless (f != NULL);
  f->addr = 0;
  f->line = 0;
  f->file = strdup ("file");
  f->fullname = NULL;
  f->func = strdup ("func");
  view_update_frame (view, stack, 2);
  stack->depth = 3;
  view_update_stack (view, stack);
  stack_free (stack);

  view_cleanup (view);
}
END_TEST

START_TEST (test_view_objects_dis)
{
  view *view;
  int ret;
  int i;
  assembler *ass;

  /* Setup the view. */
  GLOB_STR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  ass = ass_create ();
  ass_add_line (ass, "123", "123", 10, 0x10, 0, "123");
  ass_add_line (ass, "123", "123", 10, 0x11, 1, "1234");
  ass_add_line (ass, "123", "123", 11, 0x12, 2, "1234");

  view_update_ass (view, ass, 0x11);

  ass_free (ass);
  view_cleanup (view);
}
END_TEST

START_TEST (test_view_objects_reg)
{
  view *view;
  int ret;
  int i;
  data_registers *reg;

  /* Setup the view. */
  GLOB_STR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  reg = data_registers_create ();
  data_registers_add (reg, 0, "r1");
  data_registers_add (reg, 1, "r1");
  data_registers_add (reg, 2, "r1");
  data_registers_set_str_value (reg, 0, "something");
  data_registers_set_str_value (reg, 1, "else");

  view_update_registers (view, reg);

  data_registers_free (reg);
  view_cleanup (view);
}
END_TEST

START_TEST (test_view_goto)
{
  view *view;
  int ret;
  int i;
  int j;
  char buf[64];
  int type;

  GLOB_STR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  /* Set up lines. */
  for (i = 0; i < 10; i++)
    {
      sprintf (buf, "Line nr %d", i);
      for (j = WIN_MAIN; j <= WIN_STACK; j++)
	{
	  ret = view_add_line (view, j, buf, -1);
	  fail_unless (ret == 0);
	}
    }

  /* Go to all lines in all windows. */
  for (i = 0; i < 10; i++)
    {
      for (j = WIN_MAIN; j <= WIN_STACK; j++)
	{
	  ret = view_go_to_line (view, j, i);
	  fail_unless (ret == 0);
	}
    }

  /* Errors */
  ret = view_go_to_line (view, -1, 5);
  fail_unless (ret < 0);
  ret = view_go_to_line (view, 0, 15);
  fail_unless (ret < 0);

  view_cleanup (view);
}
END_TEST

START_TEST (test_view_move)
{
  view *view;
  int ret;
  int i;
  int j;
  char buf[64];
  int type;

  GLOB_STR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  /* Set up lines. */
  for (i = 0; i < 80; i++)
    {
      sprintf (buf, "Line nr %d", i);
      for (j = WIN_MAIN; j <= WIN_STACK; j++)
	{
	  ret = view_add_line (view, j, buf, -1);
	  fail_unless (ret == 0);
	}
    }

  /* Move the or scroll i all windows. */
  for (j = 0; j <= WIN_STACK; j++)
    {
      ret = view_go_to_line (view, j, 40);
      fail_unless (ret == 0);
      for (i = 0; i < 80; i++)
	{
	  ret = view_move_cursor (view, i);
	  fail_unless (ret == (j >= 1 && j <= 5 ? -1 : 0), "ret %d i %d j %d",
		       ret, i, j);
	}
    }
  view_cleanup (view);
}
END_TEST

START_TEST (test_view_tag)
{
  view *view;
  int ret;
  int i;
  char buf[64];
  int type;

  GLOB_STR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  /* Set up the tags for the main window. */
  for (i = 0; i < 150; i++)
    {
      sprintf (buf, "Line %d", i);
      ret = view_add_line (view, WIN_MAIN, buf, i);
      fail_unless (ret == 0);
    }

  /* Check that tags is ok. */
  ret = view_set_focus (view, WIN_MAIN);
  fail_unless (ret == 0);
  for (i = 0; i < 150; i++)
    {
      ret = view_go_to_line (view, WIN_MAIN, i);
      fail_unless (ret == 0);
      type = -1;
      ret = view_get_tag (view, &type);
      fail_unless (ret == i);
      fail_unless (type == WIN_MAIN);
    }

  /* Test to retriev tag in main when focus (current) is another window. */
  ret = view_go_to_line (view, WIN_MAIN, 15);
  fail_unless (ret == 0);
  ret = view_set_focus (view, WIN_TARGET);
  fail_unless (ret == 0);
  type = WIN_MAIN;
  ret = view_get_tag (view, &type);
  fail_unless (ret == 15);

  view_cleanup (view);
}
END_TEST

START_TEST (test_view_curpos)
{
  view *view;
  int ret;
  int i;
  char buf[64];
  int type;
  int win;
  const char *name;
  int line;

  GLOB_STR = NULL;
  GLOB_GROUPS = NULL;
  GLOB_COLOR_STR = NULL;
  GLOB_PAIR = NULL;
  ret = view_setup (&view, (configuration *) 42);
  fail_unless (ret == 0);

  /* Set up the main window (has cursor pos). */
  ret = view_show_file (view, CONFDIR "text_test.txt", 15, 1);
  fail_unless (ret == 0);

  /* Set up the target window (has no cursor pos). */
  for (i = 0; i < 10; i++)
    {
      sprintf (buf, "Line %d", i);
      ret = view_add_line (view, WIN_TARGET, buf, i);
      fail_unless (ret == 0);
    }

  /* Test main window. */
  ret = view_go_to_line (view, WIN_MAIN, 4);
  fail_unless (ret == 0);
  ret = view_set_focus (view, WIN_MAIN);
  fail_unless (ret == 0);
  win = -1;
  name = NULL;
  line = -1;

  ret = view_get_cursor (view, &win, &line, &name);
  fail_unless (ret == 0);
  fail_unless (line == 4);
  fail_unless (win == WIN_MAIN);
  fail_unless (strcmp (CONFDIR "text_test.txt", name) == 0);
  ret = view_scroll_up (view);
  ret = view_get_cursor (view, &win, &line, &name);
  fail_unless (ret == 0);
  fail_unless (line == 5);
  fail_unless (win == WIN_MAIN);
  fail_unless (strcmp (CONFDIR "text_test.txt", name) == 0);
  ret = view_scroll_down (view);
  ret = view_get_cursor (view, &win, &line, &name);
  fail_unless (ret == 0);
  fail_unless (line == 4);
  fail_unless (win == WIN_MAIN);
  fail_unless (strcmp (CONFDIR "text_test.txt", name) == 0);

  /* Test target window. */
  ret = view_go_to_line (view, WIN_TARGET, 4);
  fail_unless (ret == 0);
  ret = view_set_focus (view, WIN_TARGET);
  fail_unless (ret == 0);
  win = -1;
  name = NULL;
  line = -1;

  ret = view_get_cursor (view, &win, &line, &name);
  fail_unless (ret < 0);
  /* Get main once more. */
  win = WIN_MAIN;
  ret = view_get_cursor (view, &win, &line, &name);
  fail_unless (ret == 0);
  fail_unless (line == 4);
  fail_unless (win == WIN_MAIN);
  fail_unless (strcmp (CONFDIR "text_test.txt", name) == 0);

  view_cleanup (view);
}
END_TEST

/**
 * @test Test view.c functions.
 *
 * The tests only tests if function's return is (not) valid, and not the
 * actual outputs to the windows.
 *
 * Tests:
 *
 * - Create: Test if creation is ok. Test valid and non-valid layouts.
 * - Misc: Test showFile, scrolling, outputs, changing outputs and
 *         setting statuses.
 * - viewUpdate*: Test updating thread-, library-, stack-, frame-,
 *   breakpoints-window, disassembler, registers.
 * - view_goto,view_move: Test goto specific lines and moving cursor.
 * - view_get_tag: Test to retrieve tags.
 * - view_curpos: Test to retrieve cursor pos.
 */
  Suite * view_suite (void)
{
  Suite *s = suite_create ("view");


  TCase *tc_view_create = tcase_create ("view_create");
  tcase_add_checked_fixture (tc_view_create, NULL, teardown);
  tcase_add_test (tc_view_create, test_view_create);
  suite_add_tcase (s, tc_view_create);

  TCase *tc_view_misc = tcase_create ("view_misc");
  tcase_add_checked_fixture (tc_view_misc, setup, teardown);
  tcase_add_test (tc_view_misc, test_view_misc);
  suite_add_tcase (s, tc_view_misc);

  TCase *tc_view_objects_thread = tcase_create ("view_objects_thread");
  tcase_add_checked_fixture (tc_view_objects_thread, NULL, teardown);
  tcase_add_test (tc_view_objects_thread, test_view_objects_thread);
  suite_add_tcase (s, tc_view_objects_thread);

  TCase *tc_view_objects_library = tcase_create ("view_objects_library");
  tcase_add_checked_fixture (tc_view_objects_library, NULL, teardown);
  tcase_add_test (tc_view_objects_library, test_view_objects_library);
  suite_add_tcase (s, tc_view_objects_library);

  TCase *tc_view_objects_breakpoint =
    tcase_create ("view_objects_breakpoint");
  tcase_add_checked_fixture (tc_view_objects_breakpoint, NULL, teardown);
  tcase_add_test (tc_view_objects_breakpoint, test_view_objects_breakpoint);
  suite_add_tcase (s, tc_view_objects_breakpoint);

  TCase *tc_view_objects_stack = tcase_create ("view_objects_stack");
  tcase_add_checked_fixture (tc_view_objects_stack, NULL, teardown);
  tcase_add_test (tc_view_objects_stack, test_view_objects_stack);
  suite_add_tcase (s, tc_view_objects_stack);

  TCase *tc_view_objects_dis = tcase_create ("view_objects_dis");
  tcase_add_checked_fixture (tc_view_objects_dis, NULL, teardown);
  tcase_add_test (tc_view_objects_dis, test_view_objects_dis);
  suite_add_tcase (s, tc_view_objects_dis);

  TCase *tc_view_objects_reg = tcase_create ("view_objects_reg");
  tcase_add_checked_fixture (tc_view_objects_reg, NULL, teardown);
  tcase_add_test (tc_view_objects_reg, test_view_objects_reg);
  suite_add_tcase (s, tc_view_objects_reg);

  TCase *tc_view_goto = tcase_create ("view_goto");
  tcase_add_checked_fixture (tc_view_goto, NULL, teardown);
  tcase_add_test (tc_view_goto, test_view_goto);
  suite_add_tcase (s, tc_view_goto);

  TCase *tc_view_move = tcase_create ("view_move");
  tcase_add_checked_fixture (tc_view_move, NULL, teardown);
  tcase_add_test (tc_view_move, test_view_move);
  suite_add_tcase (s, tc_view_move);

  TCase *tc_view_tag = tcase_create ("view_tag");
  tcase_add_checked_fixture (tc_view_tag, NULL, teardown);
  tcase_add_test (tc_view_tag, test_view_tag);
  suite_add_tcase (s, tc_view_tag);

  TCase *tc_view_curpos = tcase_create ("view_curpos");
  tcase_add_checked_fixture (tc_view_curpos, NULL, teardown);
  tcase_add_test (tc_view_curpos, test_view_curpos);
  suite_add_tcase (s, tc_view_curpos);

  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = view_suite ();
  SRunner *sr = srunner_create (s);
  srunner_set_fork_status (sr, CK_NOFORK);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
