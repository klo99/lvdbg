#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "../src/configuration.h"

int VERBOSE_LEVEL = 1;
FILE *OUT_FILE = NULL;

conf_parameter head_group[] = {
  {"h_int", PARAM_INT, -100, 100, {.int_value = 1}},
  {"h_uint", PARAM_UINT, 0, 100, {.uint_value = 10}},
  {"h_bool_true", PARAM_BOOL, 0, 0, {.bool_value = 1}},
  {"h_bool_false", PARAM_BOOL, 0, 0, {.bool_value = 0}},
  {"h_char", PARAM_CHAR, 0, 0, {.char_value = 'A'}},
  {"h_float", PARAM_FLOAT, -100, 100, {.float_value = 5.5f}},
  {"h_string", PARAM_STRING, 0, 0, {.string_value = "default"}},
  NULL,
};

conf_parameter bad_1_group[] = {
  {"h_int", PARAM_INT, -100, 100, {.int_value = 1}},
  {"h_uint", PARAM_UINT, 0, 100, {.uint_value = 10}},
  {"h_bool_true", -1, 0, 0, {.bool_value = 1}},
  {"h_bool_false", PARAM_BOOL, 0, 0, {.bool_value = 0}},
  {"h_char", PARAM_CHAR, 0, 0, {.char_value = 'A'}},
  {"h_float", PARAM_FLOAT, -100, 100, {.float_value = 5.5f}},
  {"h_string", PARAM_STRING, 0, 0, {.string_value = "default"}},
  NULL,
};

START_TEST (test_conf_new)
{
  configuration *conf;

  conf = conf_create ();
  fail_unless (conf != 0);

  conf_free (conf);
}
END_TEST

START_TEST (test_conf_add_group)
{
  configuration *conf;
  int ret;

  conf = conf_create ();
  fail_unless (conf != 0);

  ret = conf_add_group (conf, NULL, head_group);
  fail_unless (ret == 0);
  conf_free (conf);

  conf = conf_create ();
  fail_unless (conf != 0);
  ret = conf_add_group (conf, "agroup", head_group);
  fail_unless (ret == 0);

  conf_free (conf);
}
END_TEST

START_TEST (test_conf_default)
{
  configuration *conf;
  int ret;
  int i;
  unsigned int u;
  char c;
  int bt;
  int bf;
  float f;
  const char *s;
  int valid;

  conf = conf_create ();
  fail_unless (conf != 0);

  ret = conf_add_group (conf, NULL, head_group);
  fail_unless (ret == 0);
  ret = conf_add_group (conf, "agroup", head_group);
  fail_unless (ret == 0);

  i = conf_get_int (conf, NULL, "h_int", &valid);
  fail_unless (valid == 1);
  fail_unless (i == 1, "Got %d expected %d", i, 1);
  u = conf_get_uint (conf, NULL, "h_uint", &valid);
  fail_unless (valid == 1);
  fail_unless (u == 10);
  c = conf_get_char (conf, NULL, "h_char", &valid);
  fail_unless (valid == 1);
  fail_unless (c == 'A');
  bf = conf_get_bool (conf, NULL, "h_bool_false", &valid);
  fail_unless (valid == 1);
  fail_unless (bf == 0);
  bt = conf_get_bool (conf, NULL, "h_bool_true", &valid);
  fail_unless (valid == 1);
  fail_unless (bt == 1);
  f = conf_get_float (conf, NULL, "h_float", &valid);
  fail_unless (valid == 1);
  fail_unless (abs (f - 5.5f) < 0.0001f, "Got %f expected %f", f, 5.5f);
  s = conf_get_string (conf, NULL, "h_string", &valid);
  fail_unless (valid == 1);
  fail_unless (strcasecmp (s, "default") == 0);

  i = conf_get_int (conf, "agroup", "h_int", &valid);
  fail_unless (valid == 1);
  fail_unless (i == 1, "Got %d expected %d", i, 1);
  u = conf_get_uint (conf, "agroup", "h_uint", &valid);
  fail_unless (valid == 1);
  fail_unless (u == 10);
  c = conf_get_char (conf, "agroup", "h_char", &valid);
  fail_unless (valid == 1);
  fail_unless (c == 'A');
  bf = conf_get_bool (conf, "agroup", "h_bool_false", &valid);
  fail_unless (valid == 1);
  fail_unless (bf == 0);
  bt = conf_get_bool (conf, "agroup", "h_bool_true", &valid);
  fail_unless (valid == 1);
  fail_unless (bt == 1);
  f = conf_get_float (conf, "agroup", "h_float", &valid);
  fail_unless (valid == 1);
  fail_unless (abs (f - 5.5f) < 0.0001f, "Got %f expected %f", f, 5.5f);
  s = conf_get_string (conf, "agroup", "h_string", &valid);
  fail_unless (valid == 1);
  fail_unless (strcasecmp (s, "default") == 0);

  conf_free (conf);
}
END_TEST

START_TEST (test_conf_file)
{
  configuration *conf;
  int ret;
  int i;
  unsigned int u;
  char c;
  int bt;
  int bf;
  float f;
  const char *s;
  int valid;

  conf = conf_create ();
  fail_unless (conf != 0);

  ret = conf_add_group (conf, NULL, head_group);
  fail_unless (ret == 0);
  ret = conf_add_group (conf, "agroup", head_group);
  fail_unless (ret == 0);
  ret = conf_add_group (conf, "c group", head_group);
  fail_unless (ret == 0);
  ret = conf_add_group (conf, "b group", head_group);
  fail_unless (ret == 0);
  ret = conf_add_group (conf, "e group", head_group);
  fail_unless (ret == 0);
  ret = conf_add_group (conf, "d group", head_group);
  fail_unless (ret == 0);
  ret = conf_load (conf, CONFDIR "test.conf");
  fail_unless (ret == 0, "ret = %d", ret);

  i = conf_get_int (conf, NULL, "h_int", &valid);
  fail_unless (valid == 1);
  fail_unless (i == 10, "Got %d expected %d", i, 10);
  u = conf_get_uint (conf, NULL, "h_uint", &valid);
  fail_unless (valid == 1);
  fail_unless (u == 55, "Got %u expected %u", u, 55);
  c = conf_get_char (conf, NULL, "h_char", &valid);
  fail_unless (valid == 1);
  fail_unless (c == 'B', "Got %c expected %c", c, 'B');
  bf = conf_get_bool (conf, NULL, "h_bool_false", &valid);
  fail_unless (valid == 1);
  fail_unless (bf == 0, "Got %d expected %d", bf, 0);
  bt = conf_get_bool (conf, NULL, "h_bool_true", &valid);
  fail_unless (valid == 1);
  fail_unless (bt == 1, "Got %d expected %d", i, 1);
  f = conf_get_float (conf, NULL, "h_float", &valid);
  fail_unless (valid == 1);
  fail_unless (abs (f - 15.5f) < 0.0001f, "Got %f expected %f", f, 15.5f);
  s = conf_get_string (conf, NULL, "h_string", &valid);
  fail_unless (valid == 1);
  fail_unless (strcasecmp (s, "Not Default") == 0,
	       "Got %d expected 'Not Default", i);

  i = conf_get_int (conf, "agroup", "h_int", &valid);
  fail_unless (valid == 1);
  fail_unless (i == 10, "Got %d expected %d", i, 10);
  u = conf_get_uint (conf, "agroup", "h_uint", &valid);
  fail_unless (valid == 1);
  fail_unless (u == 75, "Got %u expected %u", u, 75);
  c = conf_get_char (conf, "agroup", "h_char", &valid);
  fail_unless (valid == 1);
  fail_unless (c == 'B', "Got %c expected %c", c, 'B');
  bf = conf_get_bool (conf, "agroup", "h_bool_false", &valid);
  fail_unless (valid == 1);
  fail_unless (bf == 0, "Got %d expected %d", bf, 0);
  bt = conf_get_bool (conf, "agroup", "h_bool_true", &valid);
  fail_unless (valid == 1);
  fail_unless (bt == 1, "Got %d expected %d", i, 1);
  f = conf_get_float (conf, "agroup", "h_float", &valid);
  fail_unless (valid == 1);
  fail_unless (abs (f - 15.5f) < 0.0001f, "Got %f expected %f", f, 15.5f);
  s = conf_get_string (conf, "agroup", "h_string", &valid);
  fail_unless (valid == 1);
  fail_unless (strcasecmp (s, "Not") == 0, "Got '%s' expected 'Not'", s);

  i = conf_get_int (conf, "b group", "h_int", &valid);
  fail_unless (valid == 1);
  fail_unless (i == 10, "Got %d expected %d", i, 10);
  u = conf_get_uint (conf, "b group", "h_uint", &valid);
  fail_unless (valid == 1);
  fail_unless (u == 85, "Got %u expected %u", u, 85);
  c = conf_get_char (conf, "b group", "h_char", &valid);
  fail_unless (valid == 1);
  fail_unless (c == 'B', "Got %c expected %c", c, 'B');
  bf = conf_get_bool (conf, "b group", "h_bool_false", &valid);
  fail_unless (valid == 1);
  fail_unless (bf == 0, "Got %d expected %d", bf, 0);
  bt = conf_get_bool (conf, "b group", "h_bool_true", &valid);
  fail_unless (valid == 1);
  fail_unless (bt == 1, "Got %d expected %d", i, 1);
  f = conf_get_float (conf, "b group", "h_float", &valid);
  fail_unless (valid == 1);
  fail_unless (abs (f + 25.5f) < 0.0001f, "Got %f expected %f", f, -25.5f);
  s = conf_get_string (conf, "b group", "h_string", &valid);
  fail_unless (valid == 1);
  fail_unless (strcasecmp (s, "Not Default") == 0,
	       "Got %d expected 'Not Default", i);

  i = conf_get_int (conf, "c group", "h_int", &valid);
  fail_unless (valid == 1);
  fail_unless (i == -16, "Got %d expected %d", i, -16);
  u = conf_get_uint (conf, "c group", "h_uint", &valid);
  fail_unless (valid == 1);
  fail_unless (u == 16, "Got %u expected %u", u, 16);
  c = conf_get_char (conf, "c group", "h_char", &valid);
  fail_unless (valid == 1);
  fail_unless (c == '\0', "Got %c expected %c", c, '\0');
  bf = conf_get_bool (conf, "c group", "h_bool_false", &valid);
  fail_unless (valid == 1);
  fail_unless (bf == 0, "Got %d expected %d", bf, 0);
  bt = conf_get_bool (conf, "c group", "h_bool_true", &valid);
  fail_unless (valid == 1);
  fail_unless (bt == 1, "Got %d expected %d", i, 1);
  f = conf_get_float (conf, "c group", "h_float", &valid);
  fail_unless (valid == 1);
  fail_unless (abs (f - 15.5f) < 0.0001f, "Got %f expected %f", f, 15.5f);
  s = conf_get_string (conf, "c group", "h_string", &valid);
  fail_unless (valid == 1);
  fail_unless (strcasecmp (s, " Not Default") == 0,
	       "Got %d expected 'Not Default", s);

  i = conf_get_int (conf, "d group", "h_int", &valid);
  fail_unless (valid == 1);
  fail_unless (i == 100, "Got %d expected %d", i, 100);
  u = conf_get_uint (conf, "d group", "h_uint", &valid);
  fail_unless (valid == 1);
  fail_unless (u == 100, "Got %u expected %u", u, 100);
  c = conf_get_char (conf, "d group", "h_char", &valid);
  fail_unless (valid == 1);
  fail_unless (c == '\07', "Got %c expected %c", c, '\07');
  bf = conf_get_bool (conf, "d group", "h_bool_false", &valid);
  fail_unless (valid == 1);
  fail_unless (bf == 0, "Got %d expected %d", bf, 0);
  bt = conf_get_bool (conf, "d group", "h_bool_true", &valid);
  fail_unless (valid == 1);
  fail_unless (bt == 1, "Got %d expected %d", i, 1);
  f = conf_get_float (conf, "d group", "h_float", &valid);
  fail_unless (valid == 1);
  fail_unless (abs (f - 100.0f) < 0.0001f, "Got %f expected %f", f, +100.0f);
  s = conf_get_string (conf, "d group", "h_string", &valid);
  fail_unless (valid == 1);
  fail_unless (strcasecmp (s, "\b\v\t\n\r\f\a\\\?\'\"\x10\010") == 0,
	       "Got %d expected 'Not Default", s);

  i = conf_get_int (conf, "e group", "h_int", &valid);
  fail_unless (valid == 1);
  fail_unless (i == -100, "Got %d expected %d", i, -100);
  u = conf_get_uint (conf, "e group", "h_uint", &valid);
  fail_unless (valid == 1);
  fail_unless (u == 0, "Got %u expected %u", u, 0);
  c = conf_get_char (conf, "e group", "h_char", &valid);
  fail_unless (valid == 1);
  fail_unless (c == '\t', "Got %c expected %c", c, '\t');
  bf = conf_get_bool (conf, "e group", "h_bool_false", &valid);
  fail_unless (valid == 1);
  fail_unless (bf == 0, "Got %d expected %d", bf, 0);
  bt = conf_get_bool (conf, "e group", "h_bool_true", &valid);
  fail_unless (valid == 1);
  fail_unless (bt == 1, "Got %d expected %d", i, 1);
  f = conf_get_float (conf, "e group", "h_float", &valid);
  fail_unless (valid == 1);
  fail_unless (abs (f + 100) < 0.0001f, "Got %f expected %f", f, -100.0f);
  s = conf_get_string (conf, "e group", "h_string", &valid);
  fail_unless (valid == 1);
  fail_unless (strlen (s) == 0, "Got '%s' expected ''", s);

  conf_free (conf);
}
END_TEST

START_TEST (test_conf_errors)
{
  configuration *conf;
  int ret;
  int i;
  unsigned int u;
  char c;
  int bt;
  int bf;
  float f;
  const char *s;
  int valid;

  /*
   * Test Add group errors.
   */
  conf = conf_create ();
  fail_unless (conf != 0);

  /* Add group to non conf. */
  ret = conf_add_group (NULL, NULL, head_group);
  fail_unless (ret == -1);
  conf_free (conf);

  /* Add two groups with no group name under root. */
  conf = conf_create ();
  fail_unless (conf != 0);
  ret = conf_add_group (conf, NULL, head_group);
  fail_unless (ret == 0);
  ret = conf_add_group (conf, NULL, head_group);
  fail_unless (ret == -1);
  conf_free (conf);

  /* Add group with bad parameter type. */
  conf = conf_create ();
  fail_unless (conf != 0);
  ret = conf_add_group (conf, NULL, bad_1_group);
  fail_unless (ret < 0);
  conf_free (conf);

  /*
   * Test load errors.
   */
  conf = conf_create ();
  fail_unless (conf != 0);

  /* Load with no groups. */
  ret = conf_load (conf, CONFDIR "test.conf");
  fail_unless (ret == -1);

  /* Test filename. */
  ret = conf_add_group (conf, "a group", head_group);
  fail_unless (ret == 0);
  ret = conf_load (conf, NULL);
  fail_unless (ret == -1);
  ret = conf_load (conf, "/dev/null/something");
  fail_unless (ret == -1);

  /* Test group names. */
  ret = conf_load (conf, CONFDIR "test_bad1.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad2.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad3.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad4.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad5.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad6.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad7.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad8.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad9.conf");
  fail_unless (ret < 0);
  ret = conf_load (conf, CONFDIR "test_bad10.conf");
  fail_unless (ret != 0);
  ret = conf_add_group (conf, NULL, head_group);
  fail_unless (ret == 0);
  ret = conf_load (conf, CONFDIR "test_bad11.conf");
  fail_unless (ret == -15, "%d", ret);
  ret = conf_load (conf, CONFDIR "test_bad12.conf");
  fail_unless (ret == -13);
  ret = conf_load (conf, CONFDIR "test_bad13.conf");
  fail_unless (ret != -14);
  ret = conf_load (conf, CONFDIR "test_bad14.conf");
  fail_unless (ret == -17, "%d", ret);

  conf_free (conf);

  /* Test getting values */
  conf = conf_create ();
  fail_unless (conf != 0);
  ret = conf_add_group (conf, "a group", head_group);
  fail_unless (ret == 0);

  i = conf_get_int (conf, NULL, "h_int", &valid);
  fail_unless (valid == 0);
  u = conf_get_uint (conf, NULL, "h_uint", &valid);
  fail_unless (valid == 0);
  c = conf_get_char (conf, NULL, "h_char", &valid);
  fail_unless (valid == 0);
  bf = conf_get_bool (conf, NULL, "h_bool_false", &valid);
  fail_unless (valid == 0);
  bt = conf_get_bool (conf, NULL, "h_bool_true", &valid);
  fail_unless (valid == 0);
  f = conf_get_float (conf, NULL, "h_float", &valid);
  fail_unless (valid == 0);
  s = conf_get_string (conf, NULL, "h_string", &valid);
  fail_unless (valid == 0);

  i = conf_get_int (conf, "a group", "X_int", &valid);
  fail_unless (valid == 0);
  u = conf_get_uint (conf, "a group", "X_uint", &valid);
  fail_unless (valid == 0);
  c = conf_get_char (conf, "a group", "X_char", &valid);
  fail_unless (valid == 0);
  bf = conf_get_bool (conf, "a group", "X_bool_false", &valid);
  fail_unless (valid == 0);
  bt = conf_get_bool (conf, "a group", "X_bool_true", &valid);
  fail_unless (valid == 0);
  f = conf_get_float (conf, "a group", "X_float", &valid);
  fail_unless (valid == 0);
  s = conf_get_string (conf, "a group", "X_string", &valid);
  fail_unless (valid == 0);

  conf_free (conf);
}
END_TEST

/**
 * @test Test configuration.c functions.
 *
 * Tests:
 * - _new: Test creation of configuration objects.
 * - _add_group: Test adding groups.
 * - _default: Test setting and retrieving default values.
 * - _file: Test loading configurations from file.
 * - _errors: Test configuration is reporting errors correctly.
 */
  Suite * conf_suite (void)
{
  Suite *s = suite_create ("conf");

  TCase *tc_conf_new = tcase_create ("new");
  tcase_add_test (tc_conf_new, test_conf_new);
  suite_add_tcase (s, tc_conf_new);

  TCase *tc_conf_add_group = tcase_create ("add_group");
  tcase_add_test (tc_conf_add_group, test_conf_add_group);
  suite_add_tcase (s, tc_conf_add_group);

  TCase *tc_conf_default = tcase_create ("default");
  tcase_add_test (tc_conf_default, test_conf_default);
  suite_add_tcase (s, tc_conf_default);

  TCase *tc_conf_file = tcase_create ("file");
  tcase_add_test (tc_conf_file, test_conf_file);
  suite_add_tcase (s, tc_conf_file);

  TCase *tc_conf_errors = tcase_create ("errors");
  tcase_add_test (tc_conf_errors, test_conf_errors);
  suite_add_tcase (s, tc_conf_errors);

  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;

  Suite *s = conf_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
