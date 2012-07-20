#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/objects.h"

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;

void checkRunning (thread_group * tg, int group_id, int id, int running,
		   int core);
void
checkRunning (thread_group * tg, int group_id, int id, int running, int core)
{
  thread_group *pg;
  thread *p;

  pg = tg;
  while (pg != NULL)
    {
      if (pg->id == group_id)
	{
	  p = pg->first;
	  while (p != NULL)
	    {
	      if (p->id == id)
		{
		  fail_unless (p->running == running, "thread %d group %d",
			       id, group_id);
		  fail_unless (p->core == core, "thread %d group %d", id,
			       group_id);
		  return;
		}
	      p = p->next;
	    }
	}
      pg = pg->next;
    }
  fail ("no thread %d in group %d", id, group_id);
}

START_TEST (test_breakpoint)
{
  int ret;
  breakpoint_table *bpt;
  breakpoint *bp;
  int i;
  char buf[64];

  bpt = bp_table_create ();
  fail_unless (bpt != NULL);
  for (i = 0; i < bpt->rows; i++)
    {
      fail_unless (bpt->breakpoints[i] == NULL);
    }

  for (i = 0; i < 20; i++)
    {
      bp = bp_create ();
      fail_unless (bp != NULL);
      snprintf (buf, 64, "func%d", i);
      bp->func = strdup (buf);
      snprintf (buf, 64, "file%d", i);
      bp->file = strdup (buf);
      snprintf (buf, 64, "full%d", i);
      bp->fullname = strdup (buf);
      bp->addr = i;
      bp->number = i;
      bp_table_insert (bpt, bp);
      fail_unless (bpt->breakpoints[i]->number == i);
      fail_unless (strcmp (bpt->breakpoints[i]->fullname, buf) == 0);
    }

  for (i = 0; i < 20; i++)
    {
      ret = bp_table_remove (bpt, i);
      fail_unless (ret == 0);
      fail_unless (bpt->breakpoints[i] == NULL);
    }
  ret = bp_table_remove (bpt, 5);
  fail_unless (ret < 0);

  for (i = 0; i < 20; i++)
    {
      bp = bp_create ();
      fail_unless (bp != NULL);
      bp->number = i;
      bp_table_insert (bpt, bp);
    }

  bp = bp_create ();
  fail_unless (bp != NULL);
  bp->number = 5;
  bp_table_insert (bpt, bp);

  bp_table_free (bpt);
}
END_TEST

START_TEST (test_thread)
{
  int ret;
  thread_group *tg = NULL;
  thread_group *ptg;
  thread *t;
  thread *pt;
  int i;

  /* Test thread groups. */
  for (i = 0; i < 20; i++)
    {
      ret = thread_group_add (&tg, i);
      fail_unless (ret == 0);
    }
  ptg = tg;
  i = 19;
  while (ptg != NULL)
    {
      fail_unless (ptg->id == i, "%d != %d", ptg->id, i);
      fail_unless (ptg->first == NULL);
      i--;
      ptg = ptg->next;
    }
  fail_unless (i == -1);
  ret = thread_group_add (&tg, 5);
  fail_unless (ret < 0);
  thread_group_remove (&tg, 0);
  thread_group_remove (&tg, 19);
  ptg = tg;
  i = 18;
  while (ptg != NULL)
    {
      fail_unless (ptg->id == i, "%d != %d", ptg->id, i);
      i--;
      ptg = ptg->next;
    }
  fail_unless (i == 0);
  thread_group_remove (&tg, 10);
  thread_group_remove (&tg, 10);
  thread_group_remove_all (&tg);
  fail_unless (tg == NULL);

  /* Test thread. */
  ret = thread_group_add (&tg, 5);
  fail_unless (ret == 0);
  ret = thread_add (tg, 2, 2);
  fail_unless (ret <= 0);
  for (i = 0; i < 20; i++)
    {
      ret = thread_add (tg, 5, i);
      fail_unless (ret == 0);
    }
  fail_unless (tg->first != NULL);
  pt = tg->first;
  pt->frame.file = strdup ("Hello");
  pt->frame.fullname = strdup ("Hello World!");
  pt->frame.func = strdup ("funx");
  thread_clear (pt);
  thread_clear (pt);
  fail_unless (pt->frame.func == NULL);
  fail_unless (pt->frame.fullname == NULL);
  fail_unless (pt->frame.file == NULL);

  i = 19;
  while (pt != NULL)
    {
      fail_unless (pt->id == i);
      i--;
      pt = pt->next;
    }
  thread_remove (tg, 2, 0);
  thread_remove (tg, 5, 100);
  thread_remove (tg, 5, 19);
  thread_remove (tg, 5, 0);
  thread_remove (tg, 5, 10);
  thread_remove_all (&tg->first);
  fail_unless (tg->first == NULL);

  ret = thread_group_add (&tg, 10);
  for (i = 0; i < 20; i++)
    {
      ret = thread_add (tg, 5, i);
      fail_unless (ret == 0);
      ret = thread_add (tg, 10, i);
      fail_unless (ret == 0);
    }
  ret = thread_set_running (tg, 0, 5, 1, 1);
  fail_unless (ret < 0);
  ret = thread_set_running (tg, 5, 25, 1, 1);
  fail_unless (ret < 0);
  ret = thread_set_running (tg, 5, 15, 10, 1);
  fail_unless (ret < 0);
  ret = thread_set_running (tg, -1, -1, 1, 1);
  fail_unless (ret == 0);
  for (i = 0; i < 20; i++)
    {
      checkRunning (tg, 5, i, 1, 1);
      checkRunning (tg, 10, i, 1, 1);
    }
  for (i = 0; i < 20; i++)
    {
      ret = thread_set_running (tg, 5, i, i % 2, i);
      fail_unless (ret == 0);
      ret = thread_set_running (tg, 10, i, i % 2, i);
      fail_unless (ret == 0);
      checkRunning (tg, 5, i, i % 2, i);
      checkRunning (tg, 10, i, i % 2, i);
    }
  thread_group_remove_all (&tg);
  fail_unless (tg == NULL);

}
END_TEST

START_TEST (test_library)
{
  int i;
  int count;
  char buf_id[64];
  char buf_target[64];
  char buf_host[64];
  int ret;
  library *l = NULL;
  library *pl;

  ret = library_add (&l, NULL, NULL, NULL, 0);
  fail_unless (ret < 0);
  fail_unless (l == NULL);
  ret = library_add (&l, "id", "target", "host", 5);
  fail_unless (ret < 0);
  fail_unless (l == NULL);

  for (i = 0; i < 20; i++)
    {
      sprintf (buf_id, "id_%d", i);
      sprintf (buf_target, "target_%d", i);
      sprintf (buf_host, "host_%d", i);
      ret = library_add (&l, buf_id, buf_target, buf_host, i % 2);
      fail_unless (ret == 0);
      count = i;
      pl = l;
      while (pl)
	{
	  sprintf (buf_id, "id_%d", count);
	  sprintf (buf_target, "target_%d", count);
	  sprintf (buf_host, "host_%d", count);
	  fail_unless (strcmp (pl->id, buf_id) == 0);
	  fail_unless (strcmp (pl->target_name, buf_target) == 0);
	  fail_unless (strcmp (pl->host_name, buf_host) == 0);
	  count--;
	  pl = pl->next;
	}
      fail_unless (count == -1);
    }
  ret = library_add (&l, "id", NULL, NULL, 1);
  fail_unless (ret == 0);
  fail_unless (strcmp (l->id, "id") == 0);
  fail_unless (strcmp (l->target_name, "") == 0);
  fail_unless (strcmp (l->host_name, "") == 0);
  ret = library_add (&l, "id", "target", NULL, 1);
  fail_unless (ret == 0);
  fail_unless (strcmp (l->id, "id") == 0);
  fail_unless (strcmp (l->target_name, "target") == 0);
  fail_unless (strcmp (l->host_name, "") == 0);
  ret = library_add (&l, "id", NULL, "host", 1);
  fail_unless (ret == 0);
  fail_unless (strcmp (l->id, "id") == 0);
  fail_unless (strcmp (l->target_name, "") == 0);
  fail_unless (strcmp (l->host_name, "host") == 0);

  /* remove */
  library_remove (&l, "cccc", NULL, NULL);
  library_remove (&l, "id", NULL, NULL);
  library_remove (&l, "id", NULL, "host");

  library_remove_all (&l);
  fail_unless (l == 0);

}
END_TEST

START_TEST (test_stack)
{
  int ret;
  char func[64];
  char file[64];
  char full[64];
  char name[64];
  char type[64];
  char val[64];
  stack *stack;
  frame *f;
  variable *v;
  int i;
  int j;

  /* Simple creation. */
  stack = stack_create (10);
  fail_unless (stack != NULL);
  fail_unless (stack->depth == -1);
  fail_unless (stack->thread_id == -1);
  fail_unless (stack->core == -1);
  fail_unless (stack->max_depth == 10);

  for (i = 0; i < 25; i++)
    {
      f = stack_get_frame (stack, i);
      fail_unless (f != 0);
      fail_unless (f->func == NULL);
      fail_unless (f->file == NULL);
      fail_unless (f->fullname == NULL);
      fail_unless (stack->max_depth >= 10);
    }

  /* Test clear frame */
  for (i = 0; i < 25; i++)
    {
      f = stack_get_frame (stack, i);
      sprintf (func, "func %d", i);
      sprintf (file, "file %d", i);
      sprintf (full, "full %d", i);
      f->func = strdup (func);
      f->file = strdup (file);
      f->fullname = strdup (full);
      ret = frame_insert_variable (f, "var1", "int", "value", 1, 1);
      fail_unless (ret == 0);
      ret = frame_insert_variable (f, "var1", "int", "value", 0, 1);
      fail_unless (ret == 0);
    }
  stack_clean_frame (stack, 5);
  fail_unless (stack->stack[5].func == NULL);
  fail_unless (stack->stack[5].file == NULL);
  fail_unless (stack->stack[5].fullname == NULL);
  fail_unless (stack->stack[5].args == NULL);
  fail_unless (stack->stack[5].variables == NULL);

  stack_free (stack);

  /* Test variables */
  stack = stack_create (10);
  fail_unless (stack != NULL);
  fail_unless (stack->depth == -1);
  fail_unless (stack->thread_id == -1);
  fail_unless (stack->core == -1);
  fail_unless (stack->max_depth == 10);

  /* Errors. */
  f = stack_get_frame (stack, 0);
  ret = frame_insert_variable (f, "name", NULL, NULL, 1, 0);
  fail_unless (ret < 0);
  fail_unless (f->args == NULL);
  fail_unless (f->variables == NULL);

  /* OK  - variables */
  ret = frame_insert_variable (f, "name", "int", "10", 1, 0);
  fail_unless (ret == 0);
  fail_unless (f->args == NULL);
  fail_unless (f->variables != NULL);
  fail_unless (strcmp (f->variables->name, "name") == 0);
  fail_unless (strcmp (f->variables->type, "int") == 0);
  fail_unless (strcmp (f->variables->value, "10") == 0);
  ret = frame_insert_variable (f, "name2", "int2", "102", 1, 0);
  fail_unless (ret == 0);
  fail_unless (f->args == NULL);
  fail_unless (f->variables != NULL);
  fail_unless (strcmp (f->variables->name, "name2") == 0);
  fail_unless (strcmp (f->variables->type, "int2") == 0);
  fail_unless (strcmp (f->variables->value, "102") == 0);
  ret = frame_insert_variable (f, "name3", "int3", "103", 1, 1);
  fail_unless (ret == 0);
  fail_unless (f->args == NULL);
  fail_unless (f->variables != NULL);
  fail_unless (strcmp (f->variables->name, "name2") == 0);
  fail_unless (strcmp (f->variables->type, "int2") == 0);
  fail_unless (strcmp (f->variables->value, "102") == 0);
  fail_unless (strcmp (f->variables->next->name, "name") == 0);
  fail_unless (strcmp (f->variables->next->type, "int") == 0);
  fail_unless (strcmp (f->variables->next->value, "10") == 0);
  fail_unless (strcmp (f->variables->next->next->name, "name3") == 0);
  fail_unless (strcmp (f->variables->next->next->type, "int3") == 0);
  fail_unless (strcmp (f->variables->next->next->value, "103") == 0);
  stack_free (stack);

  /* OK  - variables */
  stack = stack_create (10);
  fail_unless (stack != NULL);
  fail_unless (stack->depth == -1);
  fail_unless (stack->thread_id == -1);
  fail_unless (stack->core == -1);
  fail_unless (stack->max_depth == 10);
  f = stack_get_frame (stack, 0);


  ret = frame_insert_variable (f, "name", "int", "10", 0, 0);
  fail_unless (ret == 0);
  fail_unless (f->variables == NULL);
  fail_unless (f->args != NULL);
  fail_unless (strcmp (f->args->name, "name") == 0);
  fail_unless (strcmp (f->args->type, "int") == 0);
  fail_unless (strcmp (f->args->value, "10") == 0);
  ret = frame_insert_variable (f, "name2", "int2", "102", 0, 0);
  fail_unless (ret == 0);
  fail_unless (f->variables == NULL);
  fail_unless (f->args != NULL);
  fail_unless (strcmp (f->args->name, "name2") == 0);
  fail_unless (strcmp (f->args->type, "int2") == 0);
  fail_unless (strcmp (f->args->value, "102") == 0);
  ret = frame_insert_variable (f, "name3", "int3", "103", 0, 1);
  fail_unless (ret == 0);
  fail_unless (f->variables == NULL);
  fail_unless (f->args != NULL);
  fail_unless (strcmp (f->args->name, "name2") == 0);
  fail_unless (strcmp (f->args->type, "int2") == 0);
  fail_unless (strcmp (f->args->value, "102") == 0);
  fail_unless (strcmp (f->args->next->name, "name") == 0);
  fail_unless (strcmp (f->args->next->type, "int") == 0);
  fail_unless (strcmp (f->args->next->value, "10") == 0);
  fail_unless (strcmp (f->args->next->next->name, "name3") == 0);
  fail_unless (strcmp (f->args->next->next->type, "int3") == 0);
  fail_unless (strcmp (f->args->next->next->value, "103") == 0);

  stack_free (stack);

  /* Test updating variables */
  stack = stack_create (10);
  fail_unless (stack != NULL);
  fail_unless (stack->depth == -1);
  fail_unless (stack->thread_id == -1);
  fail_unless (stack->core == -1);
  fail_unless (stack->max_depth == 10);
  f = stack_get_frame (stack, 0);

  ret = frame_insert_variable (f, "var", NULL, "val1", 0, 0);
  fail_unless (ret == 0);
  fail_unless (strcmp (f->args->name, "var") == 0);
  fail_unless (strcmp (f->args->value, "val1") == 0);
  fail_unless (f->args->type == NULL);
  ret = frame_insert_variable (f, "var", NULL, "v1", 0, 0);
  fail_unless (ret == 0);
  fail_unless (strcmp (f->args->name, "var") == 0);
  fail_unless (strcmp (f->args->value, "v1") == 0);
  fail_unless (f->args->type == NULL);
  ret = frame_insert_variable (f, "var", NULL, "val1_long", 0, 0);
  fail_unless (ret == 0);
  fail_unless (strcmp (f->args->name, "var") == 0);
  fail_unless (strcmp (f->args->value, "val1_long") == 0);
  fail_unless (f->args->type == NULL);
  ret = frame_insert_variable (f, "var", "char*", "val1", 0, 0);
  fail_unless (ret == 0);
  fail_unless (strcmp (f->args->name, "var") == 0);
  fail_unless (strcmp (f->args->value, "val1") == 0);
  fail_unless (strcmp (f->args->type, "char*") == 0);
  ret = frame_insert_variable (f, "var", "char*", NULL, 0, 0);
  fail_unless (ret == 0);
  fail_unless (strcmp (f->args->name, "var") == 0);
  fail_unless (f->args->value == NULL);
  fail_unless (strcmp (f->args->type, "char*") == 0);
  ret = frame_insert_variable (f, "var", "char*", "...", 0, 0);
  fail_unless (ret == 0);
  fail_unless (strcmp (f->args->name, "var") == 0);
  fail_unless (strcmp (f->args->value, "...") == 0);
  fail_unless (strcmp (f->args->type, "char*") == 0);

  ret = frame_insert_variable (f, "var", "const char*", "val1", 0, 0);
  fail_unless (ret < 0);

  stack_free (stack);
}
END_TEST

START_TEST (test_asm)
{
  int ret;

  assembler *ass;

  ass = ass_create ();
  fail_unless (ass != NULL);

  ret = ass_add_line (ass, "foo.c", "bar", 10, 0x10, 0x00, "hello");
  fail_unless (ret == 1);
  ret = ass_add_line (ass, "foo.c", "bar", 10, 0x11, 0x01, "world");
  fail_unless (ret == 0);
  ret = ass_add_line (ass, "foo.c", "bar", 11, 0x12, 0x02, NULL);
  fail_unless (ret == 0);
  ret = ass_add_line (ass, "foo.c", "foo", 5, 0x05, 0x00, "again");
  fail_unless (ret == 1);
  ret = ass_add_line (ass, "foo.c", NULL, -1, -1, -1, NULL);
  fail_unless (ret == 0);
  ret = ass_add_line (ass, "foo.c", "foo", 1, 0x07, 0x02, "back");
  fail_unless (ret == 0);
  ret = ass_add_line (ass, "foo.c", "foo", 1, 0x06, 0x01, "back");
  fail_unless (ret == 0);

  ret =
    ass_add_line (ass, "foo_long.c", "foo_long", 10, 0x06, 0x00, "back_long");
  fail_unless (ret == 1);
  ret =
    ass_add_line (ass, "foo_long.c", "foo_long", 11, 0x07, 0x01, "back_long");
  fail_unless (ret == 0);
  ret =
    ass_add_line (ass, "foo_long.c", "foo_long", 12, 0x08, 0x02, "back_long");
  fail_unless (ret == 0);
  ret =
    ass_add_line (ass, "foo_long.c", "foo_long", 12, 0x08, 0x02, "back_long");
  fail_unless (ret == 0);
  ret =
    ass_add_line (ass, "foo_long.c", "foo_long", 12, 0x09, 0x03, "back_long");
  fail_unless (ret == 0);
  ret =
    ass_add_line (ass, "foo_long.c", "foo_long", 12, 0x0A, 0x04, "back_long");
  fail_unless (ret == 0);

  ass_reset (ass);

  ret =
    ass_add_line (ass, "foo_long.c", "foo_long", 10, 0x06, 0x00,
		  "back_longer");
  fail_unless (ret == 1);
  ret =
    ass_add_line (ass, "foo_long.c", "foo_long", 11, 0x08, 0x02,
		  "back_longer");
  fail_unless (ret == 0);

  ass_free (ass);
}
END_TEST

START_TEST (test_reg)
{
  int ret;
  char buf[32];
  int i;

  data_registers *reg;

  reg = data_registers_create ();
  fail_unless (reg != NULL);

  data_registers_add (reg, 0, "Hello");
  for (i = 0; i < 150; i++)
    {
      sprintf (buf, "reg %d", i);
      data_registers_add (reg, i, buf);
    }

  ret = data_registers_set_value (reg, 190, 1);
  fail_unless (ret < 0);
  ret = data_registers_set_str_value (reg, 190, "ll");
  fail_unless (ret < 0);

  ret = data_registers_set_value (reg, 100, 99);
  fail_unless (ret == 0);
  fail_unless (reg->registers[100].u64 == 99);
  ret = data_registers_set_str_value (reg, 100, "Yes");
  fail_unless (ret == 0);
  fail_unless (strcmp (reg->registers[100].svalue, "Yes") == 0);
  ret = data_registers_set_str_value (reg, 100, "Yes1234");
  fail_unless (ret == 0);
  fail_unless (strcmp (reg->registers[100].svalue, "Yes1234") == 0);
  ret = data_registers_set_value (reg, 100, 99);
  fail_unless (ret == 0);
  fail_unless (reg->registers[100].u64 == 99);

  data_registers_free (reg);
}
END_TEST

/**
 * @test Test objects.c functions.
 *
 * Test that creating, using and freeing debugger objects
 * are ok.
 *
 * - _breakpoint: breakpoint and breakpoint_table tests.
 * - _thread: Test for thread and thread group is ok.
 * - _library: Test library is ok.
 * - _stack: Test stack, frame and variables is ok.
 * - _asm: Test asm objects.
 * - _reg: Test register functions.
 */
  Suite * objects_suite (void)
{
  Suite *s = suite_create ("objects");

  TCase *tc_breakpoint = tcase_create ("breakpoint");
  tcase_add_test (tc_breakpoint, test_breakpoint);
  suite_add_tcase (s, tc_breakpoint);

  TCase *tc_thread = tcase_create ("thread");
  tcase_add_test (tc_thread, test_thread);
  suite_add_tcase (s, tc_thread);

  TCase *tc_library = tcase_create ("library");
  tcase_add_test (tc_library, test_library);
  suite_add_tcase (s, tc_library);

  TCase *tc_stack = tcase_create ("stack");
  tcase_add_test (tc_stack, test_stack);
  suite_add_tcase (s, tc_stack);

  TCase *tc_asm = tcase_create ("asm");
  tcase_add_test (tc_asm, test_asm);
  suite_add_tcase (s, tc_asm);

  TCase *tc_reg = tcase_create ("reg");
  tcase_add_test (tc_reg, test_reg);
  suite_add_tcase (s, tc_reg);

  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = objects_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
