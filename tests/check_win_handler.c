#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <term.h>

#include "../src/win_handler.h"

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;

void
setup (void)
{
  initscr ();
}

void
teardown (void)
{
  endwin ();
}

START_TEST (test_win_handler_create)
{
  Win *w;
  int ret;
  win_properties props = { 0, 0 };

  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);

  win_set_focus (w, 1);
  win_set_focus (w, 0);
  win_set_focus (w, 0);

  win_free (w);
}
END_TEST

START_TEST (test_win_handler_text)
{
  int ret;
  Win *w;
  win_properties props = { 0, 0 };

  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);

  win_set_status (w, "test");
  win_set_status (w, "");
  win_set_status (w, "test test\ntest");

  ret = win_add_line (w, "line 1", 0, 1);
  fail_unless (ret == 0);

  ret = win_add_line (w, "line 1", 1, 1);
  fail_unless (ret == 0);

  ret = win_add_line (w, "line 1", 0, 1);
  fail_unless (ret == 0);
  ret = win_add_line (w, "line 1", 1, 1);
  fail_unless (ret == 0);
  ret = win_add_line (w, "line 1", 1, 1);
  fail_unless (ret == 0);
  ret = win_add_line (w, "line\001 1", 1, 1);
  fail_unless (ret < 0);

  ret = win_load_file (w, CONFDIR "text_test_bad.txt");
  fail_unless (ret < 0);

  ret = win_load_file (w, CONFDIR "text_test_short_file.txt");
  fail_unless (ret == 0);

  ret = win_load_file (w, CONFDIR "text_test.txt");
  fail_unless (ret == 0);

  ret = win_load_file (w, CONFDIR "text_test_long.txt");
  fail_unless (ret == 0);

  win_free (w);
}
END_TEST

START_TEST (test_win_handler_scroll)
{
  int ret;
  Win *w;
  win_properties props = { 0, 0 };

  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);
  ret = win_load_file (w, CONFDIR "text_test_long.txt");

  fail_unless (ret == 0);
  ret = win_scroll (w, -1);
  fail_unless (ret < 0);	/* at the first line already. */

  ret = win_scroll (w, 130);
  fail_unless (ret < 0);	/* scroll to far down. */

  ret = win_scroll (w, -15);
  fail_unless (ret == 0);

  ret = win_scroll (w, -1);
  fail_unless (ret == 0);

  ret = win_scroll (w, 5);
  fail_unless (ret == 0);

  ret = win_scroll (w, 1);
  fail_unless (ret == 0);

  win_to_top (w);

  ret = win_go_to_line (w, -2);
  fail_unless (ret < 0);
  ret = win_go_to_line (w, 180);
  fail_unless (ret < 0);

  ret = win_go_to_line (w, 15);
  fail_unless (ret == 0);

  ret = win_move (w, -1);
  fail_unless (ret == 0);
  ret = win_move (w, -4);
  fail_unless (ret == 0);
  ret = win_move (w, 1);
  fail_unless (ret == 0);
  ret = win_move (w, 4);
  fail_unless (ret == 0);

  win_clear (w);

  win_free (w);

  props.properties = WIN_PROP_CURSOR;
  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);
  ret = win_load_file (w, CONFDIR "text_test_long.txt");
  ret = win_go_to_line (w, 15);
  fail_unless (ret == 0);
  ret = win_move (w, -1);
  fail_unless (ret == 0);
  ret = win_move (w, -4);
  fail_unless (ret == 0);
  ret = win_move (w, 1);
  fail_unless (ret == 0);
  ret = win_move (w, 4);
  fail_unless (ret == 0);

  win_clear (w);

  win_free (w);
}
END_TEST

START_TEST (test_win_handler_mark)
{
  Win *w;
  int ret;
  win_properties props = { 0, 0 };

  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);
  ret = win_load_file (w, CONFDIR "text_test.txt");
  fail_unless (ret == 0);

  ret = win_set_mark (w, 15, 1, '?');
  fail_unless (ret < 0);
  win_free (w);

  props.indent = 100;
  props.properties = WIN_PROP_MARKS | WIN_PROP_CURSOR;
  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);
  ret = win_load_file (w, CONFDIR "text_test.txt");
  fail_unless (ret == 0);
  ret = win_set_mark (w, 1115, 1, '?');
  fail_unless (ret < 0);
  ret = win_set_mark (w, -1, 1, '?');
  fail_unless (ret == 0);	/* Last line */
  ret = win_set_mark (w, 15, 100, '?');
  fail_unless (ret < 0);
  ret = win_set_mark (w, 15, 9, '?');
  fail_unless (ret < 0);
  ret = win_set_mark (w, 15, -1, '?');
  fail_unless (ret < 0);
  ret = win_set_mark (w, 15, 2, '\001');
  fail_unless (ret < 0);

  ret = win_set_mark (w, 2, 2, '?');
  fail_unless (ret == 0);
  win_free (w);

}
END_TEST

START_TEST (test_win_handler_move_cursor)
{
  Win *w;
  int ret;
  int i;
  win_properties props = { 0, 0 };

  /* Test non cursor window. */
  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);
  ret = win_load_file (w, CONFDIR "text_test.txt");
  fail_unless (ret == 0);
  ret = win_move_cursor (w, -1);
  fail_unless (ret < 0);
  win_free (w);

  /* Test cursor window */
  props.properties = WIN_PROP_CURSOR;
  w = win_create (0, 0, 20, 80, &props);
  fail_unless (w != NULL);
  ret = win_load_file (w, CONFDIR "text_test.txt");
  fail_unless (ret == 0);
  ret = win_go_to_line (w, 60);
  fail_unless (ret == 0);
  ret = win_move_cursor (w, 1);
  fail_unless (ret == 0);
  ret = win_move_cursor (w, -1);
  fail_unless (ret == 0);
  ret = win_move_cursor (w, 25);
  fail_unless (ret == 0);
  ret = win_move_cursor (w, -25);
  fail_unless (ret == 0);


  for (i = 0; i < 125; i++)
    {
      ret = win_move_cursor (w, 1);
      fail_unless (ret == 0);
    }
  for (i = 0; i < 125; i++)
    {
      ret = win_move_cursor (w, -1);
      fail_unless (ret == 0);
    }
  win_free (w);
}
END_TEST

START_TEST (test_win_handler_tag)
{
  Win *w;
  int ret;
  int i;
  char buf[64];

  win_properties props = { 0, WIN_PROP_CURSOR };

  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);

  for (i = 0; i < 150; i++)
    {
      sprintf (buf, "Line nr %d", i);
      ret = win_add_line (w, buf, 0, i);
      fail_unless (ret == 0);
    }
  for (i = 0; i < 150; i++)
    {
      ret = win_go_to_line (w, i);
      fail_unless (ret == 0);
      ret = win_get_tag (w);
      fail_unless (ret == i, "tag = %d != %d", ret, i);
    }
  win_free (w);
}
END_TEST

START_TEST (test_win_handler_get_curpos)
{
  Win *w;
  int ret;
  int i;
  char buf[64];
  const char *name;
  int line;

  win_properties props = { 0, WIN_PROP_CURSOR };

  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);

  /* No name */
  name = win_get_filename (w);
  fail_unless (name == NULL);

  /* With name. */
  ret = win_load_file (w, CONFDIR "text_test.txt");
  fail_unless (ret == 0);
  name = win_get_filename (w);
  fail_unless (name != NULL);

  for (i = 0; i < 10; i++)
    {
      ret = win_go_to_line (w, i);
      fail_unless (ret == 0);
      line = win_get_cursor (w);
      fail_unless (line == i);
    }

  win_free (w);
}
END_TEST

START_TEST (test_win_handler_get_line)
{
  Win *w;
  int ret;
  int i;
  char buf[64];
  const char *line;

  win_properties props = { 0, WIN_PROP_CURSOR, NULL, 0, NULL };

  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);

  for (i = 0; i < 150; i++)
    {
      sprintf (buf, "Line %d", i);
      ret = win_add_line (w, buf, 0, i);
      fail_unless (ret == 0);
    }

  for (i = 0; i < 150; i++)
    {
      sprintf (buf, "Line %d", i);
      line = win_get_line (w, i);
      fail_unless (strcmp (line, buf) == 0);
    }
  line = win_get_line (w, -1);
  fail_unless (line == NULL);
  line = win_get_line (w, 200);
  fail_unless (line == NULL);

  win_free (w);
}
END_TEST

START_TEST (test_win_handler_syntax)
{
  Win *w;
  int ret;
  int i;
  char buf[64];
  const char *line;
  win_attribute attr[] = { {1, 0x01}, {2, 0x02}, {3, 0x03} };
  char *scan =
    strdup
    ("{id='1',type='1',match='int'},{id='2',match='Line'},{id='1',match='[[:digit:]]*'}");

  win_properties props =
    { 0, WIN_PROP_CURSOR | WIN_PROP_SYNTAX, attr, 3, scan };

  w = win_create (0, 0, 40, 80, &props);
  fail_unless (w != NULL);

  for (i = 0; i < 45; i++)
    {
      sprintf (buf, "Line int %d", i);
      ret = win_add_line (w, buf, 1, i);
      fail_unless (ret == 0);
    }
  win_go_to_line (w, 5);
  free (scan);

  win_free (w);
}
END_TEST

/**
 * @test Test win_handler.c functions.
 *
 * The tests only tests if function's return is (not) valid, and not the
 * actual outputs to the screen.
 *
 * Tests:
 *
 * - _create: Test if creation is ok. Test setting focus.
 * - _text: Test adding line, set status and loading file.
 * - _scroll: Test scrolling, go to lines, bring window to top. Test moving
 *            window, by win_move().
 * - _mark: Test marking in window.
 * - _move_cursor: Test moving cursor. Test moving/scroll if
 *   window has no cursor.
 * - _tag: Test setting/getting tags.
 * - _get_curpos: Test getting cursor pos and filename.
 * - get_line: Get the text line.
 * - _syntax: Highlighting and scanner.
 */
  Suite * win_handler_suite (void)
{
  Suite *s = suite_create ("win_handler");

  TCase *tc_win_handler_create = tcase_create ("win_handler_create");
  tcase_add_checked_fixture (tc_win_handler_create, setup, teardown);
  tcase_add_test (tc_win_handler_create, test_win_handler_create);
  suite_add_tcase (s, tc_win_handler_create);

  TCase *tc_win_handler_text = tcase_create ("win_handler_text");
  tcase_add_checked_fixture (tc_win_handler_text, setup, teardown);
  tcase_add_test (tc_win_handler_text, test_win_handler_text);
  suite_add_tcase (s, tc_win_handler_text);

  TCase *tc_win_handler_scroll = tcase_create ("win_handler_scroll");
  tcase_add_checked_fixture (tc_win_handler_scroll, setup, teardown);
  tcase_add_test (tc_win_handler_scroll, test_win_handler_scroll);
  suite_add_tcase (s, tc_win_handler_scroll);

  TCase *tc_win_handler_mark = tcase_create ("win_handler_mark");
  tcase_add_checked_fixture (tc_win_handler_mark, setup, teardown);
  tcase_add_test (tc_win_handler_mark, test_win_handler_mark);
  suite_add_tcase (s, tc_win_handler_mark);

  TCase *tc_win_handler_move_cursor =
    tcase_create ("win_handler_move_cursor");
  tcase_add_checked_fixture (tc_win_handler_move_cursor, setup, teardown);
  tcase_add_test (tc_win_handler_move_cursor, test_win_handler_move_cursor);
  suite_add_tcase (s, tc_win_handler_move_cursor);

  TCase *tc_win_handler_tag = tcase_create ("win_handler_tag");
  tcase_add_checked_fixture (tc_win_handler_tag, setup, teardown);
  tcase_add_test (tc_win_handler_tag, test_win_handler_tag);
  suite_add_tcase (s, tc_win_handler_tag);

  TCase *tc_win_handler_get_curpos = tcase_create ("win_handler_get_curpos");
  tcase_add_checked_fixture (tc_win_handler_get_curpos, setup, teardown);
  tcase_add_test (tc_win_handler_get_curpos, test_win_handler_get_curpos);
  suite_add_tcase (s, tc_win_handler_get_curpos);

  TCase *tc_win_handler_get_line = tcase_create ("win_handler_get_line");
  tcase_add_checked_fixture (tc_win_handler_get_line, setup, teardown);
  tcase_add_test (tc_win_handler_get_line, test_win_handler_get_line);
  suite_add_tcase (s, tc_win_handler_get_line);

  TCase *tc_win_handler_syntax = tcase_create ("win_handler_syntax");
  tcase_add_checked_fixture (tc_win_handler_syntax, setup, teardown);
  tcase_add_test (tc_win_handler_syntax, test_win_handler_syntax);
  suite_add_tcase (s, tc_win_handler_syntax);

  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = win_handler_suite ();
  SRunner *sr = srunner_create (s);
  srunner_set_fork_status (sr, CK_NOFORK);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
