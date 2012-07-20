#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/text.h"

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;

START_TEST (test_text_create)
{
  text *text;

  text = text_create ();
  fail_unless (text != NULL);

  text_free (text);
}
END_TEST

START_TEST (test_text_line)
{
  text *text;
  int i;
  char buf[128];
  const char *p;
  int ret;
  int len;

  text = text_create ();
  fail_unless (text != NULL);

  for (i = 0; i < 120; i++)
    {
      sprintf (buf, "Line nr %d", i);
      ret = text_add_line (text, buf);
      fail_unless (ret == i + 1);
      fail_unless (text_nr_of_lines (text) == i + 1);
    }

  for (i = 0; i < 120; i++)
    {
      sprintf (buf, "Line nr %d", i);
      p = text_get_line (text, i, &len);
      fail_unless (p != NULL);
      fail_unless (strlen (buf) == len);
      fail_unless (strcmp (p, buf) == 0);
    }
  p = text_get_line (text, 500, &len);
  fail_unless (p == NULL);
  p = text_get_line (text, -50, &len);
  fail_unless (p == NULL);

  text_clear (text);
  fail_unless (text_nr_of_lines (text) == 0);
  ret = text_add_line (text, "\001");
  fail_unless (ret < 0);
  fail_unless (text_nr_of_lines (text) == 0);
  ret = text_add_line (text, "\t");
  fail_unless (ret == 1);
  fail_unless (text_nr_of_lines (text) == 1);
  p = text_get_line (text, 0, &len);
  fail_unless (p != NULL);
  fail_unless (len == 2);
  fail_unless (strcmp (p, "  ") == 0);

  text_free (text);
}
END_TEST

START_TEST (test_text_load)
{
  text *text;
  int i;
  int ret;
  char buf[64];
  const char *p;
  int len;

  text = text_load_file (CONFDIR "/text_WRONG.txt");
  fail_unless (text == NULL);

  text = text_load_file (CONFDIR "/text_test.txt");
  fail_unless (text != NULL);
  for (i = 0; i < 120; i++)
    {
      sprintf (buf, "  Line %d", i);
      p = text_get_line (text, i, &len);
      fail_unless (p != NULL);
      fail_unless (strlen (buf) == len, "%d len = %d != %d", i, len,
		   strlen (buf));
      fail_unless (strcmp (p, buf) == 0);
    }
  fail_unless (text_nr_of_lines (text) == 120);

  text_clear (text);
  fail_unless (text_nr_of_lines (text) == 0);

  ret = text_update_from_file (text, CONFDIR "/text_WRONG.txt");
  fail_unless (ret < 0);

  ret = text_update_from_file (text, CONFDIR "/text_test.txt");
  fail_unless (ret == 0);
  for (i = 0; i < 120; i++)
    {
      sprintf (buf, "  Line %d", i);
      p = text_get_line (text, i, &len);
      fail_unless (p != NULL);
      fail_unless (strlen (buf) == len);
      fail_unless (strcmp (p, buf) == 0);
    }
  fail_unless (text_nr_of_lines (text) == 120);

  ret = text_update_from_file (text, CONFDIR "/text_test.txt");
  fail_unless (ret == 0);
  for (i = 0; i < 120; i++)
    {
      sprintf (buf, "  Line %d", i);
      p = text_get_line (text, i, &len);
      fail_unless (p != NULL);
      fail_unless (strlen (buf) == len);
      fail_unless (strcmp (p, buf) == 0);
    }
  fail_unless (text_nr_of_lines (text) == 120);

  ret = text_update_from_file (text, CONFDIR "/text_test_bad.txt");
  fail_unless (ret < 0);

  text_free (text);
}
END_TEST

/**
 * @test Test text.x functions.
 *
 * Test text functions.
 * - _create: Test creation.
 * - _line: Test adding lines to text.
 * - _load: Test loading files.
 */
  Suite * text_suite (void)
{
  Suite *s = suite_create ("text");

  TCase *tc_text_create = tcase_create ("text_create");
  tcase_add_test (tc_text_create, test_text_create);
  suite_add_tcase (s, tc_text_create);

  TCase *tc_text_line = tcase_create ("text_line");
  tcase_add_test (tc_text_line, test_text_line);
  suite_add_tcase (s, tc_text_line);

  TCase *tc_text_load = tcase_create ("text_load");
  tcase_add_test (tc_text_load, test_text_load);
  suite_add_tcase (s, tc_text_load);

  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = text_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
