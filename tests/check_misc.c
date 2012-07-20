#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "../src/misc.h"

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;

START_TEST (test_misc_str_next)
{
  char *text;
  int len;
  int ret;
  char *name;
  char *value;
  char *next;

  text = strdup ("var1=\"5\"");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '\"');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "5") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1=\"[5\"");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '\"');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "[5") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1=\"{5\"");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '\"');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "{5") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1=\"5]\"");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '\"');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "5]") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1=\"5}\"");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '\"');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "5}") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1=\'5\'");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '\'');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "5") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1=\"\\\"\"");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '\"');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "\"\"") == 0, "value = '%s'", value);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1={var2=\"5\"}");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '{');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "var2=\"5\"") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1=[var2=\"5\"]");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '[');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "var2=\"5\"") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("var1=[var2=\"5\",var3=\"7\"]");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '[');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "var2=\"5\",var3=\"7\"") == 0, "value='%s'",
	       value);
  fail_unless (next == text + len,
	       "text=%p; next=%p; next - text = %d" "; strlen(text)=%d", text,
	       next, next - text, len);
  free (text);

  text = strdup ("var1={var2=\"5\",var3=\"7\"}");
  len = strlen (text);
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '{');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "var2=\"5\",var3=\"7\"") == 0, "value='%s'",
	       value);
  fail_unless (next == text + len,
	       "text=%p; next=%p; next - text = %d" "; strlen(text)=%d", text,
	       next, next - text, len);
  free (text);

  text = strdup ("var1=\"5\",var2=\"6\"");
  len = 8;
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == '\"');
  fail_unless (strcmp (name, "var1") == 0);
  fail_unless (strcmp (value, "5") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  ret = get_next_param (next, &name, &value, &next);
  len = 17;
  fail_unless (ret == '\"');
  fail_unless (strcmp (name, "var2") == 0);
  fail_unless (strcmp (value, "6") == 0);
  fail_unless (next == text + len, "text=%p; next=%p; next - text = %d"
	       "; strlen(text)=%d", text, next, next - text, len);
  free (text);

  text = strdup ("}");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == 0);
  fail_unless (name == NULL);
  fail_unless (next == NULL);
  fail_unless (value == NULL);
  free (text);

  text = strdup ("]");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == 0);
  fail_unless (name == NULL);
  fail_unless (next == NULL);
  fail_unless (value == NULL);
  free (text);

  text = strdup ("");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret == 0);
  fail_unless (name == NULL);
  fail_unless (next == NULL);
  fail_unless (value == NULL);
  free (text);

  /* Test errors. */
  text = strdup ("var1=\"5");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("var1='5");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("var1=[5");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("var1={5\"");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("var1=}5\"");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("var1=]5\"");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("var1='5");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("var1=\"5'");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("var1\"5\"");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

  text = strdup ("?var1\"5\"");
  ret = get_next_param (text, &name, &value, &next);
  fail_unless (ret < 0);
  free (text);

}
END_TEST

START_TEST (test_misc_unescape)
{
  char *text;
  int ret;

  text = strdup ("ABCcde\\n\\r\\a\\f\\v\\b\\x30\\100\\17\\7");
  ret = unescape (text, "");
  fail_unless (ret == 0);
  fail_unless (memcmp (text, "ABCcde\n\r\a\f\v\b\x30\100\17\7", 16) == 0,
	       "failed: '%s'", text);
  free (text);

  text = strdup ("\"ABC\"");
  ret = unescape (text, "");
  fail_unless (ret == 0);
  fail_unless (strcmp (text, "ABC") == 0, "failed: '%s'", text);
  free (text);

  text = strdup ("\'ABC\'\r\n");
  ret = unescape (text, "");
  fail_unless (ret == 0);
  fail_unless (strcmp (text, "ABC") == 0, "failed: '%s'", text);
  free (text);

  text = strdup ("ABC\r\nB");
  ret = unescape (text, "\rB\n");
  fail_unless (ret == 0);
  fail_unless (strcmp (text, "AC") == 0, "failed: '%s'", text);
  free (text);

  /* Errors */
  text = strdup ("ABC\\P");
  ret = unescape (text, "");
  fail_unless (ret < 0);
  free (text);
}
END_TEST

START_TEST (test_misc_safe_write)
{
  int fd[2];
  int ret;
  char msg[] = "Dr Livingstone I presume?";
  char buf[100];

  ret = pipe (fd);
  fail_unless (ret == 0, "Failed to create pipe: %m");

  ret = safe_write (fd[1], msg);
  fail_unless (ret == 0);
  ret = read (fd[0], buf, 100);
  fail_unless (ret > 0);
  buf[ret] = '\0';
  fail_unless (strcmp (msg, buf) == 0);

  close (fd[0]);
  close (fd[1]);
}
END_TEST

START_TEST (test_misc_safe_write_signal)
{
  int fd[2];
  int ret;
  char msg[] = "Dr Livingstone I presume?";

  ret = pipe (fd);
  fail_unless (ret == 0, "Failed to create pipe: %m");

  close (fd[0]);
  ret = safe_write (fd[1], msg);
  fail_unless (ret < 0);
  close (fd[1]);
}
END_TEST

/**
 * @test Test misc.c functions.
 *
 * Test the utility functions.
 * - _str_next: Test get_next_param.
 * - _unescape: Test that unescaping is ok.
 * - _safe_write(_signal): Test that safe write is ok.
 */
  Suite * misc_suite (void)
{
  Suite *s = suite_create ("get_next_param");

  TCase *tc_misc_get_next = tcase_create ("str_next");
  tcase_add_test (tc_misc_get_next, test_misc_str_next);
  suite_add_tcase (s, tc_misc_get_next);

  TCase *tc_misc_unsescape = tcase_create ("unescape");
  tcase_add_test (tc_misc_unsescape, test_misc_unescape);
  suite_add_tcase (s, tc_misc_unsescape);

  TCase *tc_misc_safe_write = tcase_create ("safe_write");
  tcase_add_test (tc_misc_safe_write, test_misc_safe_write);
  suite_add_tcase (s, tc_misc_safe_write);

/* Test work but it seems like some sort of leak in check.
  TCase *tc_misc_safe_write_signal = tcase_create ("safe_write_signal");
  tcase_add_test_raise_signal (tc_misc_safe_write_signal,
                               test_misc_safe_write_signal, SIGPIPE);
  suite_add_tcase (s, tc_misc_safe_write_signal);
*/

  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = misc_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
