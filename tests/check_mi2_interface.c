#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/mi2_interface.h"
#include "../src/win_form.h"

#define LONG_FILE_NAME "1                                                     "\
                       "1                                                     "\
                       "1                                                     "\
                       "1                                                     "\
                       "1                                                     "

typedef struct mi2_parser_t
{
  int fake;
} mi2_parser;

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;
mi2_parser *GLOB_PARSER = NULL;
int GLOB_RET = 0;
int GLOB_FORM_RET;
int GLOB_CMD = -1;
int GLOB_WIN;
int GLOB_LINE;
int GLOB_FORM_NEW;
int GLOB_FORM_ENUM = 0;
int GLOB_FORM_BOOL = 0;
int GLOB_FORM_INT = 0;
char *GLOB_STRING = NULL;
char *GLOB_NAME;
char *g_to_find[99];
int g_found[99];
int g_safe_write;
int g_fd;
int g_form_run;
int g_thread;
char *g_regs;
char *g_file;
breakpoint *GLOB_BP;

/* Overloading dependence to mi2_parser functions. */
int
safe_write (int fd, const char *msg)
{
  char **p;
  int i;

  printf ("msg [%d]: '%s'", fd, msg);
  g_fd = fd;
  g_safe_write++;
  p = g_to_find;
  i = 0;
  while (p && *p)
    {
      if (strstr (msg, *p) != NULL)
	{
	  g_found[i] = 1;
	}
      p++;
      i++;
    }

  return GLOB_RET;
}

int
form_run (input_field * fields, const char *header)
{
  g_form_run++;
  input_field *f;

  if (GLOB_FORM_ENUM)
    {
      f = fields;
      while (f && f->text)
	{
	  if (f->type == INPUT_TYPE_ENUM)
	    {
	      f->enum_value = GLOB_FORM_ENUM - 1;
	    }
	  f++;
	}
    }

  if (GLOB_FORM_NEW)
    {
      f = fields;
      while (f && f->text)
	{
	  if (f->type == INPUT_TYPE_STRING)
	    {
	      f->string_value = GLOB_STRING ? strdup (GLOB_STRING) : NULL;
	    }
	  f++;
	}
    }
  if (GLOB_FORM_BOOL)
    {
      f = fields;
      while (f && f->text)
	{
	  if (f->type == INPUT_TYPE_BOOL)
	    {
	      f->bool_value = 1;
	    }
	  f++;
	}
    }
  if (GLOB_FORM_INT)
    {
      f = fields;
      while (f && f->text)
	{
	  if (f->type == INPUT_TYPE_INT)
	    {
	      f->bool_value = GLOB_FORM_INT;
	    }
	  f++;
	}
    }

  return GLOB_FORM_RET;
}

mi2_parser *
mi2_parser_create (view * view, configuration * conf)
{
  return GLOB_PARSER;
}

void
mi2_parser_free (mi2_parser * parser)
{
  ;
}

int
mi2_parser_get_thread (mi2_parser * parser)
{
  return GLOB_RET;
}

int
mi2_parser_set_thread (mi2_parser * parser, int id)
{
  g_thread = id;
  return GLOB_RET;
}

void
mi2_parser_set_frame (mi2_parser * parser, int frame)
{
}

int
mi2_parser_parse (mi2_parser * parser, char *line, int *update, char **regs)
{
  *update = GLOB_CMD;
  *regs = g_regs;
  return GLOB_RET;
};

breakpoint *
mi2_parser_get_bp (mi2_parser * parser, const char *file_name, int line_nr)
{
  return GLOB_BP;
}

void
mi2_parser_remove_bp (mi2_parser * parser, int number)
{
}

int
mi2_parser_get_location (mi2_parser * parser, char **file, int *line)
{
  *line = 99;
  *file = g_file = strdup ("A file");

  return GLOB_RET;
}

void
mi2_parser_toggle_disassemble (mi2_parser * parser)
{
}

int
view_get_cursor (view * view, int *win, int *line_nr, const char **file_name)
{
  if (win == NULL || line_nr == NULL || file_name == NULL)
    {
      return -1;
    }
  *win = GLOB_WIN;
  *line_nr = GLOB_LINE;
  *file_name = GLOB_NAME;

  return GLOB_RET;
}

int
view_add_message (view * view, int level, const char *msg, ...)
{
  return 0;
}

START_TEST (test_mi2_interface_create)
{
  mi2_interface *mi2;

  GLOB_PARSER = NULL;
  mi2 = mi2_create (1, 0, NULL, NULL);
  fail_unless (mi2 == NULL);

  GLOB_PARSER = (mi2_parser *) 1;
  mi2 = mi2_create (1, 0, NULL, NULL);
  fail_unless (mi2 != NULL);

  mi2_free (mi2);
}
END_TEST

START_TEST (test_mi2_interface_do_action)
{
  int ret;
  mi2_interface *mi2;
  char buf[512];

  GLOB_PARSER = (mi2_parser *) 1;
  mi2 = mi2_create (99, 0, NULL, NULL);
  fail_unless (mi2 != NULL);

  g_to_find[0] = "-break-insert -t main";
  g_to_find[1] = "-exec-run";
  g_to_find[2] = NULL;
  g_found[0] = 0;
  g_found[1] = 0;
  ret = mi2_do_action (mi2, ACTION_INT_START, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);
  fail_unless (g_found[1] == 1);

  g_to_find[0] = "-stack-list-frames --thread";
  g_to_find[1] = "-thread-info";
  g_to_find[2] = "-stack-list-variables --thread";
  g_to_find[3] = NULL;
  g_found[0] = 0;
  g_found[1] = 0;
  g_found[2] = 0;
  ret = mi2_do_action (mi2, ACTION_INT_UPDATE, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);
  fail_unless (g_found[1] == 1);
  fail_unless (g_found[2] == 1);

  g_to_find[0] = "-exec-next";
  g_to_find[1] = NULL;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_NEXT, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);

  g_to_find[0] = "-exec-step";
  g_to_find[1] = NULL;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_STEP, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);

  g_to_find[0] = "-exec-next-instruction";
  g_to_find[1] = NULL;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_NEXTI, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);

  g_to_find[0] = "-exec-step-instruction";
  g_to_find[1] = NULL;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_STEPI, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);

  g_to_find[0] = "-stack-list-frames";
  g_to_find[1] = NULL;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_STACK_LIST_FRAMES, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);

  g_to_find[0] = "-stack-list-variables";
  g_to_find[1] = NULL;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_STACK_LIST_VARIABLES, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);

  g_to_find[0] = NULL;
  g_thread = -1;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_THREAD_SELECT, 99);
  fail_unless (ret == 0);
  fail_unless (g_thread == 99);
  fail_unless (g_safe_write == 0);

  g_to_find[0] = "-file-list-exec-source-files";
  g_to_find[1] = NULL;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_FILE_LIST_EXEC_SORCES, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);

  g_to_find[0] = "-data-disassemble ";
  g_to_find[1] = NULL;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_DATA_DISASSEMBLE, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0] == 1);
  if (g_file)
    {
      free (g_file);
    }
  g_file = NULL;

  ret = mi2_do_action (mi2, -1, 0);
  fail_unless (ret == -1);
  mi2_free (mi2);

  GLOB_RET = -1;
  mi2 = mi2_create (-1, 0, NULL, NULL);
  fail_unless (mi2 != NULL);
  ret = mi2_do_action (mi2, ACTION_INT_START, 0);
  fail_unless (ret == -1);
  mi2_free (mi2);
}
END_TEST

START_TEST (test_mi2_interface_parse)
{
  int ret;
  mi2_interface *mi2;
  char buf[512];

  GLOB_PARSER = (mi2_parser *) 1;
  mi2 = mi2_create (99, 0, NULL, NULL);
  fail_unless (mi2 != NULL);

  GLOB_RET = 0;
  g_regs = NULL;
  sprintf (buf, "FOO");
  ret = mi2_parse (mi2, buf);
  fail_unless (ret == 0);

  GLOB_RET = -1;
  g_regs = NULL;
  ret = mi2_parse (mi2, buf);
  fail_unless (ret == -1);

  g_safe_write = 0;
  g_regs = NULL;
  GLOB_CMD = 1;
  GLOB_RET = 0;
  g_to_find[0] = "-stack-list";
  g_to_find[1] = "-thread-info";
  g_to_find[2] = NULL;
  g_found[0] = 0;
  g_found[1] = 0;
  sprintf (buf, "FOO");
  ret = mi2_parse (mi2, buf);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 2);
  fail_unless (g_found[0] == 1);
  fail_unless (g_found[1] == 1);
  if (g_file)
    {
      free (g_file);
    }
  g_file = NULL;

  mi2_toggle_disassemble (mi2);
  g_safe_write = 0;
  g_regs = "1 3 5";
  GLOB_CMD = 1;
  GLOB_RET = 0;
  g_to_find[0] = "-stack-list";
  g_to_find[1] = "-thread-info";
  g_to_find[2] = "-data-disassemble";
  g_to_find[3] = "-data-list-register-values x 1 3 5";
  g_to_find[4] = "-data-list-changed-registers";
  g_to_find[5] = "-data-evaluate-expression $pc";
  g_to_find[6] = NULL;
  g_found[0] = 0;
  g_found[1] = 0;
  g_found[2] = 0;
  g_found[3] = 0;
  g_found[4] = 0;
  g_found[5] = 0;
  if (g_file)
    {
      free (g_file);
    }
  g_file = NULL;
  sprintf (buf, "FOO");
  ret = mi2_parse (mi2, buf);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 6);
  fail_unless (g_found[0] == 1);
  fail_unless (g_found[1] == 1);
  fail_unless (g_found[2] == 1);
  fail_unless (g_found[3] == 1);
  fail_unless (g_found[4] == 1);
  fail_unless (g_found[5] == 1);
  if (g_file)
    {
      free (g_file);
    }
  g_file = NULL;

  mi2_free (mi2);
}
END_TEST

/* Test breakpoint functionality. */
START_TEST (test_mi2_interface_bp)
{
  int ret;
  mi2_interface *mi2;
  breakpoint bp;

  GLOB_PARSER = (mi2_parser *) 1;
  mi2 = mi2_create (99, 0, NULL, NULL);
  fail_unless (mi2 != NULL);

  /* Insert simple. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  g_to_find[0] = "-break-insert  foo.c:42";	/* NB extra space. */
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_SIMPLE, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);
  /* Again to delete */
  bp.number = 43;
  bp.fullname = "foo.c";
  bp.cond = NULL;
  bp.ignore = 0;
  bp.thread = -1;
  bp.disp = 1;
  bp.enabled = 1;
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = &bp;
  g_to_find[0] = "-break-delete 43";	/* NB extra space. */
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_SIMPLE, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Insert advanced. Form canceled */
  GLOB_FORM_NEW = 0;
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = LONG_FILE_NAME;
  GLOB_BP = NULL;
  GLOB_FORM_RET = -1;
  g_to_find[0] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_ADVANCED, 0);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 0);

  /* Insert advanced. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-break-insert  foo.c:43";	/* NB extra space. */
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_ADVANCED, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Insert advanced. Change string fields */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "SOMETHING";
  g_to_find[0] = "-break-insert";
  g_to_find[1] = "SOMETHING";
  g_to_find[2] = "-c";
  g_to_find[3] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  g_found[1] = 0;
  g_found[2] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_ADVANCED, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_found[1]);
  fail_unless (g_found[2]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Insert advanced. Change string fields */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = LONG_FILE_NAME;
  g_to_find[0] = "-break-insert";
  g_to_find[1] = LONG_FILE_NAME;
  g_to_find[2] = "-c";
  g_to_find[3] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  g_found[1] = 0;
  g_found[2] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_ADVANCED, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_found[1]);
  fail_unless (g_found[2]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Insert advanced. Change string to NULL */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = NULL;
  g_to_find[0] = NULL;
  g_safe_write = 0;
  ret = mi2_do_action (mi2, ACTION_BP_ADVANCED, 0);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 0);

  /* Insert advanced. Flags */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_NEW = 0;
  GLOB_FORM_BOOL = 1;
  g_to_find[0] = "-break-insert";
  g_to_find[1] = "-t";
  g_to_find[2] = "-d";
  g_to_find[3] = "-f";
  g_to_find[4] = "-h";
  g_to_find[5] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  g_found[1] = 0;
  g_found[2] = 0;
  g_found[3] = 0;
  g_found[4] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_ADVANCED, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_found[1]);
  fail_unless (g_found[2]);
  fail_unless (g_found[3]);
  fail_unless (g_found[4]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Insert advanced. thread + ignore */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_NEW = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 99;
  g_to_find[0] = "-break-insert";
  g_to_find[1] = "-p 99";
  g_to_find[2] = "-i 99";
  g_to_find[3] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  g_found[1] = 0;
  g_found[2] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_ADVANCED, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_found[1]);
  fail_unless (g_found[2]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Insert advanced - With breakpoint. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = &bp;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-break-insert  foo.c:43";	/* NB extra space. */
  g_to_find[1] = "-break-delete 43";
  g_to_find[2] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_ADVANCED, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_found[1]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 2);

  /* Insert wp - With breakpoint. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = &bp;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-break-watch foo.c:42";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  g_found[1] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_WATCHPOINT, 0);
  fail_unless (ret == -1);
  fail_unless (g_found[0] == 0);
  fail_unless (g_found[1] == 0);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 0);

  /* Insert wp. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = &bp;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "foo.c:42                                                  "
    "                                                          ";
  g_to_find[0] = "-break-watch foo.c:42";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_WATCHPOINT, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Insert wp - Cancel */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = &bp;
  GLOB_FORM_RET = -1;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "foo.c:42";
  g_to_find[0] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_WATCHPOINT, 0);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 0);

  /* Insert wp. -a */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = &bp;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 1;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "foo.c:42";
  g_to_find[0] = "-break-watch -a foo.c:42";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_WATCHPOINT, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Insert wp. -r */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = &bp;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 2;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "foo.c:42";
  g_to_find[0] = "-break-watch -r foo.c:42";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_BP_WATCHPOINT, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  mi2_free (mi2);
}
END_TEST

START_TEST (test_mi2_interface_exec)
{
  int ret;
  mi2_interface *mi2;
  breakpoint bp;

  GLOB_PARSER = (mi2_parser *) 1;
  mi2 = mi2_create (99, 0, NULL, NULL);
  fail_unless (mi2 != NULL);

  /* Continue. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  g_to_find[0] = "-exec-continue";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_CONT, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Continue. Advanced cancel */
  GLOB_FORM_RET = -1;
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  g_to_find[0] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_CONT_OPT, 0);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 0);

  /* Continue. Advanced. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 1;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-exec-continue";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_CONT_OPT, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Continue. Advanced. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 33;
  GLOB_FORM_ENUM = 1;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-exec-continue --thread-group 33";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_CONT_OPT, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Continue. Advanced. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 1;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-exec-continue --reverse --all";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_CONT_OPT, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Finish. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-exec-finish";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_FINISH, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Interrupt. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-exec-interrupt";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_INTR, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Interrupt. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-exec-interrupt";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_INTR, 1);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Interrupt. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 44;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "-exec-interrupt --thread-group 44";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_INTR, 1);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Interrupt. Cancel. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = -1;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_INTR, 1);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 0);

  /* Jump. */
  GLOB_RET = 0;
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "exec-jump foo.c:43";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_JUMP, 1);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Jump. Cancel. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "bar.c:43";
  g_to_find[0] = "-exec-jump bar.c:43";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_JUMP, 1);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Jump. Cancel. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = -1;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "bar.c:43";
  g_to_find[0] = "exec-jump bar.c:43";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_JUMP, 1);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 0);

  /* Jump. Cancel. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = NULL;
  g_to_find[0] = "exec-jump bar.c:43";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_JUMP, 1);
  fail_unless (ret < 0);
  fail_unless (g_found[0] == 0);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 0);

  /* Until. */
  GLOB_RET = 0;
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  g_to_find[0] = "exec-until foo.c:43";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_UNTIL, 0);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Until. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "bar.c:43";
  g_to_find[0] = "exec-until bar.c:43";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_UNTIL, 1);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Jump. Cancel. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = -1;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = "bar.c:43";
  g_to_find[0] = "exec-until bar.c:43";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_UNTIL, 1);
  fail_unless (ret == 0);
  fail_unless (g_safe_write == 0);

  /* Until. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 1;
  GLOB_STRING = NULL;
  g_to_find[0] = "exec-until";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_UNTIL, 1);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Return. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  GLOB_STRING = NULL;
  g_to_find[0] = "exec-return";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_RETURN, 1);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  /* Run. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  GLOB_FORM_RET = 0;
  GLOB_FORM_BOOL = 0;
  GLOB_FORM_INT = 0;
  GLOB_FORM_ENUM = 0;
  GLOB_FORM_NEW = 0;
  GLOB_STRING = NULL;
  g_to_find[0] = "exec-run";
  g_to_find[1] = NULL;
  g_safe_write = 0;
  g_found[0] = 0;
  ret = mi2_do_action (mi2, ACTION_EXEC_RUN, 1);
  fail_unless (ret == 0);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 1);

  mi2_free (mi2);
}
END_TEST

/* Test breakpoint functionality. */
START_TEST (test_mi2_interface_dis)
{
  int ret;
  mi2_interface *mi2;

  GLOB_PARSER = (mi2_parser *) 1;
  mi2 = mi2_create (99, 0, NULL, NULL);
  fail_unless (mi2 != NULL);

  /* Insert simple. */
  GLOB_WIN = 0;
  GLOB_LINE = 42;
  GLOB_NAME = "foo.c";
  GLOB_BP = NULL;
  g_to_find[0] = "-data-disassemble ";	/* NB extra space. */
  g_to_find[1] = "-data-list-changed-registers";
  g_to_find[2] = 0;
  g_safe_write = 0;
  g_found[0] = 0;
  mi2_toggle_disassemble (mi2);
  fail_unless (g_found[0]);
  fail_unless (g_fd == 99);
  fail_unless (g_safe_write == 2);
  if (g_file)
    {
      free (g_file);
    }
  g_file = NULL;

  mi2_free (mi2);
}
END_TEST

/**
 * @test Test mi2_interface.c functions.
 *
 * Test the mi2 interface.
 * - _create: Creation of the interface.
 * - _do_action: Test that the interface is sending the right commands
 *               to the debugger.
 * - _parser: Test that the interface is dispatching parsing to the parser. 
 * - _bp: Test handling of breakpoint commands.
 * - _exec: Test execution commands.
 * - _dis: Test setting disassamble.
 */
  Suite * mi2_interface_suite (void)
{
  Suite *s = suite_create ("mi2_interface");

  TCase *tc_mi2_interface_create = tcase_create ("mi2_interface_create");
  tcase_add_test (tc_mi2_interface_create, test_mi2_interface_create);
  suite_add_tcase (s, tc_mi2_interface_create);

  TCase *tc_mi2_interface_do_action =
    tcase_create ("mi2_interface_do_action");
  tcase_add_test (tc_mi2_interface_do_action, test_mi2_interface_do_action);
  suite_add_tcase (s, tc_mi2_interface_do_action);

  TCase *tc_mi2_interface_parse = tcase_create ("mi2_interface_parse");
  tcase_add_test (tc_mi2_interface_parse, test_mi2_interface_parse);
  suite_add_tcase (s, tc_mi2_interface_parse);

  TCase *tc_mi2_interface_bp = tcase_create ("mi2_interface_bp");
  tcase_add_test (tc_mi2_interface_bp, test_mi2_interface_bp);
  suite_add_tcase (s, tc_mi2_interface_bp);

  TCase *tc_mi2_interface_exec = tcase_create ("mi2_interface_exec");
  tcase_add_test (tc_mi2_interface_exec, test_mi2_interface_exec);
  suite_add_tcase (s, tc_mi2_interface_exec);

  TCase *tc_mi2_interface_dis = tcase_create ("mi2_interface_dis");
  tcase_add_test (tc_mi2_interface_dis, test_mi2_interface_dis);
  suite_add_tcase (s, tc_mi2_interface_dis);

  return s;

}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = mi2_interface_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
