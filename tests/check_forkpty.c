#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/pseudo_fork.h"

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;

START_TEST (test_forkpty_create)
{
  int fd;
  pid_t pid;
  char buf[1024];
  char *args[] = { "\"Hello World\"", NULL };
  int ret;

  ret = start_forkpty (&fd, &pid, "echo", args);
  fail_unless (ret == 0);
  usleep (1000000);
  ret = read (fd, buf, 1024);
  fail_unless (ret > 0);
  buf[ret] = '\0';
  fail_unless (strstr (buf, "Hello World") != 0, "Got '%s'", buf);
  close (fd);
}
END_TEST

/**
 * \test Test pseudo_fork.c function.
 *
 * Test that it possible to set up a psuedo terminal and read/write to the
 * terminal.
 */
  Suite * forkpty_suite (void)
{
  Suite *s = suite_create ("forkpty");

  TCase *tc_forkpty_create = tcase_create ("forkpty_create");
  tcase_add_test (tc_forkpty_create, test_forkpty_create);
  suite_add_tcase (s, tc_forkpty_create);
  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = forkpty_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
