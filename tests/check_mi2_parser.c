#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/mi2_parser.h"
#include "../src/mi2_interface.h"
#include "../src/objects.h"

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;
int GLOB_RET = 0;
breakpoint_table *g_bpt = NULL;
library *g_libraries = NULL;
thread_group *g_threads = NULL;
frame *g_frames;
stack *g_stack;
int g_level;
int g_ass;
int g_pc;
char *g_file_name = NULL;
int g_line = -1;
data_registers *g_regs;

int conf_get_bool (configuration * conf, const char *group_name,
		   const char *name, int *valid);
void view_update_breakpoints (view * view, breakpoint_table * bpt);
void view_update_threads (view * view, thread_group * thread_groups);
void view_update_libraries (view * view, library * libraries);
void view_update_frame (view * view, stack * stack, int level);
void view_update_stack (view * view, stack * stack);
int view_show_file (view * view, const char *file_name, int line,
		    int mark_stop);
void view_remove_breakpoint (view * view, const char *file_name, int line_nr);
int view_add_message (view * view, int level, const char *msg, ...);
void view_update_ass (view * view, assembler * ass, int pc);
void view_update_registers (view * view, data_registers * regs);

int
conf_get_bool (configuration * conf, const char *group_name,
	       const char *name, int *valid)
{
  return GLOB_RET;
}

void
view_update_breakpoints (view * view, breakpoint_table * bpt)
{
  g_bpt = bpt;
}

void
view_update_threads (view * view, thread_group * thread_groups)
{
  g_threads = thread_groups;
}

void
view_update_libraries (view * view, library * libraries)
{
  g_libraries = libraries;
}

void
view_update_frame (view * view, stack * stack, int level)
{
  g_stack = stack;
  g_level = level;
}

void
view_update_stack (view * view, stack * stack)
{
  g_stack = stack;
}

void
view_update_ass (view * view, assembler * ass, int pc)
{
  g_pc = pc;
  g_ass++;
}

void
view_update_registers (view * view, data_registers * regs)
{
  g_regs = regs;
}

int
view_show_file (view * view, const char *file_name, int line, int mark_stop)
{
  if (g_file_name)
    {
      free (g_file_name);
    }
  g_file_name = strdup (file_name ? file_name : "");
  g_line = line;
  return GLOB_RET;
}

void
view_remove_breakpoint (view * view, const char *file_name, int line_nr)
{
}

int
view_add_message (view * view, int level, const char *msg, ...)
{
  return GLOB_RET;
}

int
form_selection (char **list, const char *header)
{
  return GLOB_RET;
}

START_TEST (test_mi2_parser_create)
{
  mi2_parser *mi2;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 2);
  fail_unless (mi2 != NULL);

  mi2_parser_toggle_disassemble (mi2);
  mi2_parser_toggle_disassemble (mi2);
  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  /* Test non mi2 record. */
  ret = mi2_parser_parse (mi2, "NOT A MI2 RECORD", &cmd, &regs);
  fail_unless (ret < 0);

  /* Test ^done */
  ret = mi2_parser_parse (mi2, "^done", &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "^done,%s", "none='?'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_threads)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  /* Test wrong threads forms. */
  snprintf (buf, 512, "^done,%s", "threads=[ ]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={something={thread-id='5'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{ }}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{something='7'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{id='K'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{target-id='K'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{thread-id='process K'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{thread-id='process 99'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{state='?'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{core='K'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "threads={{frame={something='0'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "%s", "=thread-group-created,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='5',id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "^done,%s",
	    "threads={{id='99',target-id='process 5'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s",
	    "threads={{id='42',target-id='process 5',state='something'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s",
	    "threads={{id='42',target-id='process 5',core='K'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s",
	    "threads={{id='42',target-id='process 5',frame={something='0'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s",
	    "threads={{id='42',target-id='process 5',frame={level='0',file='bar.c',addr='0x1'},state='stopped'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "^done,%s",
	    "threads={{id='42',target-id='process 5',frame={level='0',file='bar.c',addr='0x1'},state='running',core='1'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_files)
{
  int ret;
  mi2_parser *mi2;
  char buf[10240];
  int cmd;
  char *regs = NULL;
  int i;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  /* Test wrong files forms. */
  snprintf (buf, 512, "^done,%s", "files=[ ]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "^done,%s", "files={some='thing'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "^done,%s", "files={{some='thing'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "^done,%s", "files={{[ ]}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  /* Ok */
  snprintf (buf, 512, "^done,%s", "files={{file='bar.c',fullname='foo.c'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);


  snprintf (buf, 512, "^done,%s", "files={");
  for (i = 0; i < 90; i++)
    {
      strcat (buf, "{file='?',fullname='ba.c'}");
      if (i != 89)
	{
	  strcat (buf, ",");
	}
    }
  strcat (buf, "}");

  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_asm)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;
  int i;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  /* No updates - wrong format */
  g_ass = 0;
  snprintf (buf, 512, "^done,%s", "asm_insns=[ ]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s", "asm_insns=[something='+']");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s", "asm_insns=[{src_and_asm_line={ }}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s", "asm_insns=[src_and_asm_line={ }]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s", "asm_insns=[src_and_asm_line={line='K'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s",
	    "asm_insns=[src_and_asm_line={some='thing'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s",
	    "asm_insns=[src_and_asm_line={line_asm_insn={ }}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s",
	    "asm_insns=[src_and_asm_line={line_asm_insn={ }}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s",
	    "asm_insns=[src_and_asm_line={line_asm_insn={some='thing'}}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  snprintf (buf, 512, "^done,%s",
	    "asm_insns=[src_and_asm_line={line='5',file='bar.c',line_asm_insn=[{aKKdress='0x1',offset='0x42',func-name='bar',inst='xxx'}]}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 0);

  /* Ok */
  g_pc = -1;
  snprintf (buf, 512, "^done,value='0x99'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "^done,%s",
	    "asm_insns=[src_and_asm_line={line='5',file='bar.c',line_asm_insn=[{address='0x1',offset='0x42',func-name='bar',inst='xxx'}]}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_ass == 1);
  fail_unless (g_pc == 0x99);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_regs)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  snprintf (buf, 512, "^done,%s", "register-names=[\"a1\",\"a2\",\"a3\"]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  g_regs = NULL;
  snprintf (buf, 512, "^done,%s",
	    "register-values=[{number='0',value='0x00'},{number='1',value='{v4_float={0x00, 0x00, 0x00, 0x00}}'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_regs != NULL);
  fail_unless (g_regs->len == 3);
  fail_unless (strcmp (g_regs->registers[0].reg_name, "a1") == 0);
  fail_unless (strcmp (g_regs->registers[1].reg_name, "a2") == 0);
  fail_unless (strcmp (g_regs->registers[0].svalue, "0x00") == 0);
  fail_unless (strcmp (g_regs->registers[1].svalue, "0x00, 0x00, 0x00, 0x00")
	       == 0);

  g_regs = NULL;
  snprintf (buf, 512, "^done,%s",
	    "register-values=[number='0',value='0x00',{number='1',value='{v4_float={0x00, 0x00, 0x00, 0x00}}'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_regs == NULL);

  g_regs = NULL;
  snprintf (buf, 512, "^done,%s",
	    "register-values=[{number='K',value='0x00'},{number='1',value='{v4_float={0x00, 0x00, 0x00, 0x00}}'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_regs == NULL);

  g_regs = NULL;
  snprintf (buf, 512, "^done,%s",
	    "register-values=[{nKKKumber='K',value='0x00'},{number='1',value='{v4_float={0x00, 0x00, 0x00, 0x00}}'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_regs == NULL);

  g_regs = NULL;
  snprintf (buf, 512, "^done,%s", "changed-registers=[\"1\",\"2\"]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (regs != NULL);
  fail_unless (strcmp (regs, " 1 2") == 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_bkpt)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);


  /* Test wrong bkpt forms. */
  snprintf (buf, 512, "^done,%s", "bkpt={wrong='?'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={{number='3'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={number='-3'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={number='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={type='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={disp='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={enabled='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={addr='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={line='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={times='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  /* not an error? */
  snprintf (buf, 512, "^done,%s", "bkpt={thread='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "^done,%s", "bkpt={ignore='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={ }");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "bkpt={[name='test'] }");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);


  /* OK bkpts. */
  snprintf (buf, 512, "^done,%s", "bkpt={number='1',disp='del',enabled='y',"
	    "type='breakpoint',addr='42',func='main',file='foo.c',"
	    "fullname='bar/foo.c',line='3',times='43',ignore='9',"
	    "cond='hello'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_bpt != NULL);
  fail_unless (g_bpt->breakpoints[1] != NULL);
  fail_unless (g_bpt->breakpoints[1]->disp == 0);
  fail_unless (g_bpt->breakpoints[1]->enabled == 1);
  fail_unless (g_bpt->breakpoints[1]->type == BP_TYPE_BREAKPOINT);
  fail_unless (g_bpt->breakpoints[1]->addr == 42);
  fail_unless (strcmp (g_bpt->breakpoints[1]->func, "main") == 0,
	       "Func: %s", g_bpt->breakpoints[1]->func);
  fail_unless (strcmp (g_bpt->breakpoints[1]->file, "foo.c") == 0,
	       "File: %s", g_bpt->breakpoints[1]->file);
  fail_unless (strcmp (g_bpt->breakpoints[1]->fullname, "bar/foo.c") == 0,
	       "Fullname: %s", g_bpt->breakpoints[1]->fullname);
  fail_unless (g_bpt->breakpoints[1]->line == 3);
  fail_unless (g_bpt->breakpoints[1]->times == 43);
  fail_unless (g_bpt->breakpoints[1]->ignore == 9);
  fail_unless (strcmp (g_bpt->breakpoints[1]->cond, "hello") == 0);

  g_bpt = NULL;
  snprintf (buf, 512, "^done,%s", "bkpt={number='2',disp='keep',enabled='n',"
	    "type='watchpoint',func='main',file='foo.c',addr='42'"
	    "fullname='bar/foo.c',line='30',cond='1',ignore='3'"
	    "original-location='foobar.c'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_bpt != NULL);
  fail_unless (g_bpt->breakpoints[2] != NULL);
  fail_unless (g_bpt->breakpoints[2]->disp == 1);
  fail_unless (g_bpt->breakpoints[2]->enabled == 0);
  fail_unless (g_bpt->breakpoints[2]->type == BP_TYPE_WATCHPOINT);
  fail_unless (g_bpt->breakpoints[2]->addr == 42);
  fail_unless (strcmp (g_bpt->breakpoints[2]->func, "main") == 0,
	       "Func: %s", g_bpt->breakpoints[2]->func);
  fail_unless (strcmp (g_bpt->breakpoints[2]->file, "foo.c") == 0,
	       "File: %s", g_bpt->breakpoints[2]->file);
  fail_unless (strcmp (g_bpt->breakpoints[2]->fullname, "bar/foo.c") == 0,
	       "Fullname: %s", g_bpt->breakpoints[2]->fullname);
  fail_unless (g_bpt->breakpoints[2]->line == 30);
  fail_unless (g_bpt->breakpoints[2]->ignore == 3);
  fail_unless (strcmp (g_bpt->breakpoints[2]->cond, "1") == 0);
  fail_unless (strcmp (g_bpt->breakpoints[2]->original_location, "foobar.c")
	       == 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_variables)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  /* Set up parser with 2 frames. */
  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);
  snprintf (buf, 512, "%s",
	    "^done,stack=[frame={level='0',addr='42',func='bar',file='foo.c',"
	    "fullname='bar/foo.c',line='99'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  mi2_parser_set_frame (mi2, 1);

  /* Test wrong variable forms. */
  snprintf (buf, 512, "%s", "^done,variables=[{kk='something'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "^done,variables=[[name='something']]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  /* Ok messages */
  g_level = -1;
  mi2_parser_set_frame (mi2, 1);
  snprintf (buf, 512, "%s", "^done,variables=[{name='p',arg='1',type='int',"
	    "value='9'},{name='q',arg='0',type='struct P'},{name='r',value='99'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_level == 1);
  fail_unless (g_stack != NULL);
  fail_unless (g_stack->stack[1].args != NULL);
  fail_unless (g_stack->stack[1].variables != NULL);
  fail_unless (strcmp (g_stack->stack[1].args->name, "p") == 0);
  fail_unless (strcmp (g_stack->stack[1].args->type, "int") == 0);
  fail_unless (strcmp (g_stack->stack[1].args->value, "9") == 0);
  fail_unless (strcmp (g_stack->stack[1].variables->next->name, "q") == 0);
  fail_unless (strcmp (g_stack->stack[1].variables->next->type, "struct P") ==
	       0);
  fail_unless (g_stack->stack[1].variables->next->value == NULL);
  fail_unless (strcmp (g_stack->stack[1].variables->name, "r") == 0);
  fail_unless (g_stack->stack[1].variables->type == NULL);
  fail_unless (strcmp (g_stack->stack[1].variables->value, "99") == 0);

  g_level = -1;
  mi2_parser_set_frame (mi2, 0);
  snprintf (buf, 512, "%s", "^done,variables=[{name='p',arg='1',type='int',"
	    "value='9'},{name='q',arg='0',type='struct P'},{name='r',value='99'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_level == 0);



  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_stack)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  /* Test wrong stack forms. */
  snprintf (buf, 512, "%s", "^done,stack=[frame={,}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "^done,stack=[frame={name=''}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "^done,stack=[frame={level='0',some='4'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "^done,stack=[frame={level='0',addr='K'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "^done,stack=[frame={level='0',line='K'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "^done,stack=[frame={level='0',level='K'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s",
	    "^done,stack=[frame={level='0',func='main',func='foo'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s",
	    "^done,stack=[frame={level='0',file='foo.c',file='bar.c'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s",
	    "^done,stack=[frame={level='0',fullname='main',fullname='foo'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  /* OK message. */
  g_frames = NULL;
  snprintf (buf, 512, "%s",
	    "^done,stack=[frame={level='0',addr='42',func='bar',file='foo.c',"
	    "fullname='bar/foo.c',line='99',level='0'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_stack != NULL);
  fail_unless (g_stack->stack[0].args == NULL);
  fail_unless (g_stack->stack[0].variables == NULL);
  fail_unless (g_stack->stack[0].line == 99);
  fail_unless (g_stack->stack[0].addr == 42);
  fail_unless (strcmp (g_stack->stack[0].func, "bar") == 0);
  fail_unless (strcmp (g_stack->stack[0].file, "foo.c") == 0);
  fail_unless (strcmp (g_stack->stack[0].fullname, "bar/foo.c") == 0);

  g_frames = NULL;
  snprintf (buf, 512, "%s",
	    "^done,stack=[frame={level='0',addr='42',func='bar',file='foo.c',"
	    "fullname='bar/foo.c',line='99'},"
	    "frame={level='1',addr='43',func='bar2',file='foo2.c',"
	    "fullname='bar2/foo2.c',line='100'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_stack != NULL);
  fail_unless (g_stack->depth == 2);
  fail_unless (g_stack->stack[0].args == NULL);
  fail_unless (g_stack->stack[0].variables == NULL);
  fail_unless (g_stack->stack[0].line == 99);
  fail_unless (g_stack->stack[0].addr == 42);
  fail_unless (strcmp (g_stack->stack[0].func, "bar") == 0);
  fail_unless (strcmp (g_stack->stack[0].file, "foo.c") == 0);
  fail_unless (strcmp (g_stack->stack[0].fullname, "bar/foo.c") == 0);
  fail_unless (g_stack->stack[1].args == NULL);
  fail_unless (g_stack->stack[1].variables == NULL);
  fail_unless (g_stack->stack[1].line == 100);
  fail_unless (g_stack->stack[1].addr == 43);
  fail_unless (strcmp (g_stack->stack[1].func, "bar2") == 0);
  fail_unless (strcmp (g_stack->stack[1].file, "foo2.c") == 0);
  fail_unless (strcmp (g_stack->stack[1].fullname, "bar2/foo2.c") == 0);

  /* Test mi2_parser_set_frame */
  g_level = -1;
  mi2_parser_set_frame (mi2, 1);
  snprintf (buf, 512, "%s",
	    "^done,variables=[{name='var1',arg='1',type='int',value='0'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_level == 1);
  g_level = -1;
  mi2_parser_set_frame (mi2, 0);
  snprintf (buf, 512, "%s",
	    "^done,variables=[{name='var2',arg='1',type='int',value='0'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_level == 0);

  if (g_file_name != NULL)
    {
      free (g_file_name);
      g_file_name = NULL;
    }

  mi2_parser_free (mi2);

}
END_TEST

START_TEST (test_mi2_parser_parse_running)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  ret = mi2_parser_parse (mi2, "^running", &cmd, &regs);
  fail_unless (ret == 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_connected)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  ret = mi2_parser_parse (mi2, "^connected", &cmd, &regs);
  fail_unless (ret == 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_error)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  ret = mi2_parser_parse (mi2, "^error", &cmd, &regs);
  /* TODO change to 0 */
  fail_unless (ret == -1);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_exit)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  ret = mi2_parser_parse (mi2, "^exit", &cmd, &regs);
  fail_unless (ret == 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_async_running)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  ret = mi2_parser_parse (mi2, "*running", &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*running,something-else'42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*running,thread-id='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  /* Ok message. */
  snprintf (buf, 512, "%s", "=thread-group-created,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='5',id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  g_threads = NULL;
  snprintf (buf, 512, "%s", "*running,thread-id='all'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_threads != NULL);
  fail_unless (g_threads->id == 5);
  fail_unless (g_threads->first != 0);
  fail_unless (g_threads->next == NULL);

  /* Fail message. */
  snprintf (buf, 512, "%s", "*running,somthing='all'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_async_stopped)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  /* Errors */
  ret = mi2_parser_parse (mi2, "*stopped", &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "%s", "*stopped,");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,something-else='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,stopped-threads='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,stopped-threads='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,id='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,core='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,disp='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,bkptno='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={var='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,reason='some obscure reason'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-group-created,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "*stopped,stopped-threads='all',disp='del'"
	    "bkptno='3',reason='breakpoint-hit'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  /* Ok messages. */
  snprintf (buf, 512, "^done,%s", "bkpt={number='1',disp='del',enabled='y',"
	    "type='breakpoint',addr='42',func='main',file='foo.c',"
	    "fullname='bar/foo.c',line='3',times='43',ignore='9',"
	    "cond='hello'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "^done,%s", "bkpt={number='3',disp='keep',enabled='y',"
	    "type='breakpoint',addr='42',func='main',file='foo.c',"
	    "fullname='bar/foo.c',line='3',times='43',ignore='9',"
	    "cond='hello'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_bpt != NULL);
  fail_unless (g_bpt->breakpoints[1] != NULL);
  fail_unless (g_bpt->breakpoints[3] != NULL);
  g_bpt = NULL;
  cmd = -2;
  snprintf (buf, 512, "%s", "*stopped,stopped-threads='all',disp='del'"
	    "bkptno='1',reason='breakpoint-hit',"
	    "frame={file='foo.c',line='4'},thread-id='99'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (cmd == 0);
  fail_unless (g_bpt != NULL);
  fail_unless (g_bpt->breakpoints[1] == NULL);
  fail_unless (mi2_parser_get_thread (mi2) == 99);
  g_bpt = NULL;
  cmd = -2;
  snprintf (buf, 512, "%s", "*stopped,stopped-threads='all',disp='keep'"
	    "bkptno='3',reason='breakpoint-hit',"
	    "frame={file='foo.c',line='4'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (cmd == 0);
  fail_unless (g_bpt == NULL);
  mi2_parser_free (mi2);

  /* Test cmd != -1 */
  GLOB_RET = 1;
  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);
  GLOB_RET = 0;
  g_bpt = NULL;
  cmd = -2;
  snprintf (buf, 512, "%s", "*stopped,stopped-threads='all',disp='keep'"
	    "bkptno='3',reason='breakpoint-hit',"
	    "frame={file='foo.c',line='4'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (cmd == 1);
  fail_unless (g_bpt == NULL);
  mi2_parser_free (mi2);

  /* Test exit */
  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);
  GLOB_RET = 0;
  g_bpt = NULL;
  cmd = -2;
  snprintf (buf, 512, "%s", "*stopped,reason='exited-normally'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (cmd == 0);
  fail_unless (g_bpt == NULL);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_thread)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd = -1;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  /* thread group created. */
  ret = mi2_parser_parse (mi2, "=thread", &cmd, &regs);
  fail_unless (ret != 0);
  g_threads = NULL;
  snprintf (buf, 512, "%s", "=thread-group-created,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_threads != NULL);
  fail_unless (g_threads->id == 5);
  fail_unless (g_threads->first == NULL);
  fail_unless (g_threads->next == NULL);
  snprintf (buf, 512, "%s", "=thread-group-created,");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-group-created");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-group-created,id='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-group-created,id='3',somthing-else='3'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  mi2_parser_free (mi2);

  /* thread created. */
  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);
  snprintf (buf, 512, "%s", "=thread-group-created,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='5',id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_threads->first != NULL);
  fail_unless (g_threads->id != 42);
  snprintf (buf, 512, "%s", "=thread-created,group-id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-created,id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-created,");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-created");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-created,id='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-created,something-else='?'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='9',id='4'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  mi2_parser_free (mi2);

  /* thread exited. */
  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);
  snprintf (buf, 512, "%s", "=thread-group-created,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='5',id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-group-exited,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_threads == NULL);
  snprintf (buf, 512, "%s", "=thread-group-exited,id='7'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-group-exited,id='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-group-exited");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-group-exited,something-else='4'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-group-exited,{something-else}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  mi2_parser_free (mi2);

  /* thread exited. */
  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);
  snprintf (buf, 512, "%s", "=thread-group-created,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='5',id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  g_threads = NULL;
  snprintf (buf, 512, "%s", "=thread-exited,group-id='5',id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (g_threads != NULL);
  fail_unless (g_threads->first == NULL);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-exited,group-id='K',id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-exited,group-id='4',id='7'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-exited,group-id='4'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-exited,id='7'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-exited,id='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-exited");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-exited,something-else='4'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "=thread-exited,{something-else}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "=thread-something-else");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  mi2_parser_free (mi2);

}
END_TEST

START_TEST (test_mi2_parser_parse_library)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);

  /* Errors. */
  snprintf (buf, 512, "=library-something-else");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "=library-loaded-else");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "=library-loaded,id=''");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "=library-loaded,something-else='?'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "=library-loaded,id='book',host-name='page',"
	    "target-name='line',symbols-loaded='K'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "=library-loaded,id='book',host-name='page',"
	    "target-name='line',symbols-loaded='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  /* Ok library messages. */
  g_libraries = NULL;
  snprintf (buf, 512, "=library-loaded,id='book',host-name='page',"
	    "target-name='line',symbols-loaded='1'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_libraries != NULL);
  fail_unless (g_libraries->next == NULL);
  fail_unless (strcmp (g_libraries->id, "book") == 0);
  fail_unless (strcmp (g_libraries->target_name, "line") == 0);
  fail_unless (strcmp (g_libraries->host_name, "page") == 0);
  fail_unless (g_libraries->symbols_loaded == 1);
  snprintf (buf, 512, "=library-unloaded,id='book',host-name='page',"
	    "target-name='line'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  g_libraries = NULL;
  snprintf (buf, 512, "=library-loaded,id='book',host-name='page',"
	    "target-name='line',symbols-loaded='1'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_libraries != NULL);
  fail_unless (g_libraries->next == NULL);
  fail_unless (strcmp (g_libraries->id, "book") == 0);
  fail_unless (strcmp (g_libraries->target_name, "line") == 0);
  fail_unless (strcmp (g_libraries->host_name, "page") == 0);
  fail_unless (g_libraries->symbols_loaded == 1);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_parse_frame)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 1);
  fail_unless (mi2 != NULL);

  /* Errors */
  snprintf (buf, 512, "%s", "*stopped,frame={,}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={name=''}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={some='4'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={addr='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={line='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={level='K'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={func='main',func='foo'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={file='foo.c',file='bar.c'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s",
	    "*stopped,frame={fullname='main',fullname='foo'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  /* OK message. */
  g_frames = NULL;
  snprintf (buf, 512, "%s",
	    "*stopped,frame={level='0',addr='42',func='bar',file='foo.c',"
	    "fullname='bar/foo.c',line='99'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_stack->stack[0].variables == NULL);
  fail_unless (g_stack->stack[0].args == NULL);
  fail_unless (g_stack->stack[0].line == 99);
  fail_unless (g_stack->stack[0].addr == 42);
  fail_unless (strcmp (g_stack->stack[0].func, "bar") == 0);
  fail_unless (strcmp (g_stack->stack[0].file, "foo.c") == 0);
  fail_unless (strcmp (g_stack->stack[0].fullname, "bar/foo.c") == 0);

  /* Parse args. */
  snprintf (buf, 512, "%s", "*stopped,frame={fullname='main',file='foo',"
	    "args={some='?'}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={fullname='main',file='foo',"
	    "args={{some='?'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);
  snprintf (buf, 512, "%s", "*stopped,frame={fullname='main',file='foo',"
	    "args={{type='?',value='value'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  /* Ok args. */
  g_frames = NULL;
  snprintf (buf, 512, "%s", "*stopped,frame={fullname='main',file='foo',"
	    "args={{name='name',type='type',value='value'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_stack->stack[0].variables == NULL);
  fail_unless (g_stack->stack[0].args != NULL);
  fail_unless (strcmp (g_stack->stack[0].args->name, "name") == 0);
  fail_unless (strcmp (g_stack->stack[0].args->type, "type") == 0);
  fail_unless (strcmp (g_stack->stack[0].args->value, "value") == 0);
  fail_unless (g_stack->stack[0].addr == -1);
  fail_unless (strcmp (g_stack->stack[0].file, "foo") == 0);
  fail_unless (strcmp (g_stack->stack[0].fullname, "main") == 0);


  g_frames = NULL;
  snprintf (buf, 512, "%s", "*stopped,frame={fullname='main',file='foo',"
	    "args={{name='name',value='value'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_stack->stack[0].variables == NULL);
  fail_unless (g_stack->stack[0].args != NULL);
  fail_unless (strcmp (g_stack->stack[0].args->name, "name") == 0);
  fail_unless (g_stack->stack[0].args->type == NULL);
  fail_unless (strcmp (g_stack->stack[0].args->value, "value") == 0);
  fail_unless (g_stack->stack[0].addr == -1);
  fail_unless (strcmp (g_stack->stack[0].file, "foo") == 0);
  fail_unless (strcmp (g_stack->stack[0].fullname, "main") == 0);


  g_frames = NULL;
  snprintf (buf, 512, "%s", "*stopped,frame={fullname='main',file='foo',"
	    "args={{name='name',type='type'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_stack->stack[0].variables == NULL);
  fail_unless (g_stack->stack[0].args != NULL);
  fail_unless (strcmp (g_stack->stack[0].args->name, "name") == 0);
  fail_unless (strcmp (g_stack->stack[0].args->type, "type") == 0);
  fail_unless (g_stack->stack[0].args->value == NULL);
  fail_unless (g_stack->stack[0].addr == -1);
  fail_unless (strcmp (g_stack->stack[0].file, "foo") == 0);
  fail_unless (strcmp (g_stack->stack[0].fullname, "main") == 0);

  /* Test ^done,frame */
  g_frames = NULL;
  snprintf (buf, 512, "%s",
	    "^done,frame={level='0',fullname='main',file='foo',"
	    "args={{name='name',type='type'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_stack->stack[0].variables == NULL);
  fail_unless (g_stack->stack[0].args != NULL);
  fail_unless (strcmp (g_stack->stack[0].args->name, "name") == 0);
  fail_unless (strcmp (g_stack->stack[0].args->type, "type") == 0);
  fail_unless (g_stack->stack[0].args->value == NULL);
  fail_unless (g_stack->stack[0].addr == -1);
  fail_unless (strcmp (g_stack->stack[0].file, "foo") == 0);
  fail_unless (strcmp (g_stack->stack[0].fullname, "main") == 0);

  /* Missing level. */
  g_frames = NULL;
  snprintf (buf, 512, "%s", "^done,frame={fullname='main',file='foo',"
	    "args={{name='name',type='type'}}}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);


  if (g_file_name != NULL)
    {
      free (g_file_name);
      g_file_name = NULL;
    }

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_bp)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;
  breakpoint *bp;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);
  fail_unless (mi2 != NULL);
  snprintf (buf, 512, "^done,%s", "bkpt={number='1',disp='del',enabled='y',"
	    "type='breakpoint',addr='42',func='main',file='foo.c',"
	    "fullname='bar/foo.c',line='3',times='43',ignore='9',"
	    "cond='hello'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "^done,%s", "bkpt={number='2',disp='del',enabled='y',"
	    "type='breakpoint',addr='42',func='main',file='foo.c',"
	    "fullname='bar/foo.c',line='42',times='43',ignore='9',"
	    "cond='hello'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  /* Get BP. */
  bp = mi2_parser_get_bp (mi2, NULL, 2);
  fail_unless (bp == NULL);
  bp = mi2_parser_get_bp (mi2, "bar/SOMETHING", 2);
  fail_unless (bp == NULL);
  bp = mi2_parser_get_bp (mi2, "bar/foo.c", 1);
  fail_unless (bp == NULL);
  bp = mi2_parser_get_bp (mi2, "bar/Something", 3);
  fail_unless (bp == NULL);
  bp = mi2_parser_get_bp (mi2, "bar/foo.c", 3);
  fail_unless (bp != NULL);
  fail_unless (bp->disp == 0);
  fail_unless (bp->addr == 42);
  bp = mi2_parser_get_bp (mi2, "bar/foo.c", 42);
  fail_unless (bp != NULL);

  /* Remove */
  mi2_parser_remove_bp (mi2, 1);
  bp = mi2_parser_get_bp (mi2, "bar/foo.c", 3);
  fail_unless (bp == NULL);
  mi2_parser_remove_bp (mi2, 2);
  bp = mi2_parser_get_bp (mi2, "bar/foo.c", 42);
  fail_unless (bp == NULL);
  mi2_parser_remove_bp (mi2, 150);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_parser_wp)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;
  breakpoint *bp;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);

  g_bpt = NULL;
  fail_unless (mi2 != NULL);
  snprintf (buf, 512, "^done,%s", "wpt={number='2',exp='one'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_bpt != NULL);
  fail_unless (g_bpt->breakpoints[2] != NULL);
  fail_unless (g_bpt->breakpoints[2]->number == 2);
  fail_unless (strcmp (g_bpt->breakpoints[2]->expression, "one") == 0);

  snprintf (buf, 512, "^done,%s", "hw-rwpt={number='2',exp='two'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_bpt != NULL);
  fail_unless (g_bpt->breakpoints[2] != NULL);
  fail_unless (g_bpt->breakpoints[2]->number == 2);
  fail_unless (strcmp (g_bpt->breakpoints[2]->expression, "two") == 0);

  snprintf (buf, 512, "^done,%s", "hw-awpt={number='2',exp='three'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  fail_unless (g_bpt != NULL);
  fail_unless (g_bpt->breakpoints[2] != NULL);
  fail_unless (g_bpt->breakpoints[2]->number == 2);
  fail_unless (strcmp (g_bpt->breakpoints[2]->expression, "three") == 0);

  snprintf (buf, 512, "^done,%s", "wpt={number='k',exp='three'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "wpt={exp='three'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s", "wpt={number='3'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "^done,%s",
	    "wpt={number='3',exp='three',something='else'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  snprintf (buf, 512, "*stopped,%s",
	    "hw-awpt={number='2',exp='three'},value={old='5',new='6'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "*stopped,%s",
	    "hw-awpt={number='2',exp='three'},value={old='6',new='7'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  snprintf (buf, 512, "*stopped,%s",
	    "hw-awpt={number='2',exp='three'},value={old='5',new='6',k2='?'}");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret < 0);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_set_get_thread)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;
  breakpoint *bp;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);

  ret = mi2_parser_set_thread (mi2, 9);
  fail_unless (ret < 0);

  fail_unless (mi2 != NULL);
  snprintf (buf, 512, "%s", "=thread-group-created,id='5'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='5',id='42'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='5',id='43'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-group-created,id='6'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);
  snprintf (buf, 512, "%s", "=thread-created,group-id='6',id='44'");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  ret = mi2_parser_get_thread (mi2);
  fail_unless (ret == 44);
  ret = mi2_parser_set_thread (mi2, 42);
  fail_unless (ret == 0);
  ret = mi2_parser_get_thread (mi2);
  fail_unless (ret == 42);
  ret = mi2_parser_set_thread (mi2, 43);
  fail_unless (ret == 0);
  ret = mi2_parser_get_thread (mi2);
  fail_unless (ret == 43);
  ret = mi2_parser_set_thread (mi2, 44);
  fail_unless (ret == 0);
  ret = mi2_parser_get_thread (mi2);
  fail_unless (ret == 44);

  mi2_parser_free (mi2);
}
END_TEST

START_TEST (test_mi2_get_location)
{
  int ret;
  mi2_parser *mi2;
  char buf[512];
  int cmd;
  char *regs = NULL;
  breakpoint *bp;
  char *line;
  int nr;

  mi2 = mi2_parser_create ((view *) 1, (configuration *) 21);

  /* Set up stack */
  g_frames = NULL;
  snprintf (buf, 512, "%s",
	    "^done,stack=[frame={level='0',addr='42',func='bar',file='foo.c',"
	    "fullname='bar/foo.c',line='99'},"
	    "frame={level='1',addr='43',func='bar2',file='foo2.c',"
	    "fullname='bar2/foo2.c',line='100'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  ret = mi2_parser_get_location (mi2, &line, &nr);
  fail_unless (ret == 0);
  fail_unless (nr == 99);
  fail_unless (strcmp (line, "bar/foo.c") == 0);

  g_frames = NULL;
  snprintf (buf, 512, "%s",
	    "^done,stack=[frame={level='0',addr='42',func='bar',file='foo.c',"
	    "line='99'},"
	    "frame={level='1',addr='43',func='bar2',file='foo2.c',"
	    "fullname='bar2/foo2.c',line='100'}]");
  ret = mi2_parser_parse (mi2, buf, &cmd, &regs);
  fail_unless (ret == 0);

  ret = mi2_parser_get_location (mi2, &line, &nr);
  fail_unless (ret < 0);

  mi2_parser_free (mi2);
}
END_TEST

/**
 * @test Test mi2_parser.c functions.
 *
 * Tests for validiation of the parset. Tests that the parser
 * reports invalid messages.
 *
 * - Parse done,threads.
 * - Parse done,files.
 * - Parse done,asm*
 * - Parse done,registers.
 * - Parse bkpt.
 * - Parse variables.
 * - Parse stack.
 * - Parse frame.
 * - Parse ^done,frame
 * - Parse ^running.
 * - Parse ^connected.
 * - Parse ^error.
 * - Parse ^exit.
 * - Parse =library.
 * - Parse =thread.
 * - Parse *stopped.
 * - Parse *running.
 * - Retrieve/remove breakpoints.
 * - wp: Watchpoints.
 * - Set/get thread.
 * - Get location.
 */
  Suite * mi2_parser_suite (void)
{
  Suite *s = suite_create ("mi2_parser");

  TCase *tc_mi2_parser_create = tcase_create ("mi2_parser_create");
  tcase_add_test (tc_mi2_parser_create, test_mi2_parser_create);
  suite_add_tcase (s, tc_mi2_parser_create);

  TCase *tc_mi2_parser_parse = tcase_create ("mi2_parser_parse");
  tcase_add_test (tc_mi2_parser_parse, test_mi2_parser_parse);
  suite_add_tcase (s, tc_mi2_parser_parse);

  TCase *tc_mi2_parser_parse_threads =
    tcase_create ("mi2_parser_parse_threads");
  tcase_add_test (tc_mi2_parser_parse_threads, test_mi2_parser_parse_threads);
  suite_add_tcase (s, tc_mi2_parser_parse_threads);

  TCase *tc_mi2_parser_parse_files = tcase_create ("mi2_parser_parse_files");
  tcase_add_test (tc_mi2_parser_parse_files, test_mi2_parser_parse_files);
  suite_add_tcase (s, tc_mi2_parser_parse_files);

  TCase *tc_mi2_parser_parse_asm = tcase_create ("mi2_parser_parse_asm");
  tcase_add_test (tc_mi2_parser_parse_asm, test_mi2_parser_parse_asm);
  suite_add_tcase (s, tc_mi2_parser_parse_asm);

  TCase *tc_mi2_parser_parse_regs = tcase_create ("mi2_parser_parse_regs");
  tcase_add_test (tc_mi2_parser_parse_regs, test_mi2_parser_parse_regs);
  suite_add_tcase (s, tc_mi2_parser_parse_regs);

  TCase *tc_mi2_parser_parse_bkpt = tcase_create ("mi2_parser_parse_bkpt");
  tcase_add_test (tc_mi2_parser_parse_bkpt, test_mi2_parser_parse_bkpt);
  suite_add_tcase (s, tc_mi2_parser_parse_bkpt);

  TCase *tc_mi2_parser_parse_variables =
    tcase_create ("mi2_parser_parse_variables");
  tcase_add_test (tc_mi2_parser_parse_variables,
		  test_mi2_parser_parse_variables);
  suite_add_tcase (s, tc_mi2_parser_parse_variables);

  TCase *tc_mi2_parser_parse_stack = tcase_create ("mi2_parser_parse_stack");
  tcase_add_test (tc_mi2_parser_parse_stack, test_mi2_parser_parse_stack);
  suite_add_tcase (s, tc_mi2_parser_parse_stack);

  TCase *tc_mi2_parser_parse_running =
    tcase_create ("mi2_parser_parse_running");
  tcase_add_test (tc_mi2_parser_parse_running, test_mi2_parser_parse_running);
  suite_add_tcase (s, tc_mi2_parser_parse_running);

  TCase *tc_mi2_parser_parse_connected =
    tcase_create ("mi2_parser_parse_connected");
  tcase_add_test (tc_mi2_parser_parse_connected,
		  test_mi2_parser_parse_connected);
  suite_add_tcase (s, tc_mi2_parser_parse_connected);

  TCase *tc_mi2_parser_parse_error = tcase_create ("mi2_parser_parse_error");
  tcase_add_test (tc_mi2_parser_parse_error, test_mi2_parser_parse_error);
  suite_add_tcase (s, tc_mi2_parser_parse_error);

  TCase *tc_mi2_parser_parse_exit = tcase_create ("mi2_parser_parse_exit");
  tcase_add_test (tc_mi2_parser_parse_exit, test_mi2_parser_parse_exit);
  suite_add_tcase (s, tc_mi2_parser_parse_exit);

  TCase *tc_mi2_parser_parse_thread =
    tcase_create ("mi2_parser_parse_thread");
  tcase_add_test (tc_mi2_parser_parse_thread, test_mi2_parser_parse_thread);
  suite_add_tcase (s, tc_mi2_parser_parse_thread);

  TCase *tc_mi2_parser_parse_library =
    tcase_create ("mi2_parser_parse_library");
  tcase_add_test (tc_mi2_parser_parse_library, test_mi2_parser_parse_library);
  suite_add_tcase (s, tc_mi2_parser_parse_library);

  TCase *tc_mi2_parser_parse_async_running =
    tcase_create ("mi2_parser_parse_async_running");
  tcase_add_test (tc_mi2_parser_parse_async_running,
		  test_mi2_parser_parse_async_running);
  suite_add_tcase (s, tc_mi2_parser_parse_async_running);

  TCase *tc_mi2_parser_parse_async_stopped =
    tcase_create ("mi2_parser_parse_async_stopped");
  tcase_add_test (tc_mi2_parser_parse_async_stopped,
		  test_mi2_parser_parse_async_stopped);
  suite_add_tcase (s, tc_mi2_parser_parse_async_stopped);

  TCase *tc_mi2_parser_parse_frame = tcase_create ("mi2_parser_parse_frame");
  tcase_add_test (tc_mi2_parser_parse_frame, test_mi2_parser_parse_frame);
  suite_add_tcase (s, tc_mi2_parser_parse_frame);

  TCase *tc_mi2_parser_bp = tcase_create ("mi2_parser_bp");
  tcase_add_test (tc_mi2_parser_bp, test_mi2_parser_bp);
  suite_add_tcase (s, tc_mi2_parser_bp);

  TCase *tc_mi2_parser_wp = tcase_create ("mi2_parser_wp");
  tcase_add_test (tc_mi2_parser_wp, test_mi2_parser_wp);
  suite_add_tcase (s, tc_mi2_parser_wp);

  TCase *tc_mi2_parser_set_get_thread = tcase_create ("mi2_set_get_thread");
  tcase_add_test (tc_mi2_parser_set_get_thread, test_mi2_set_get_thread);
  suite_add_tcase (s, tc_mi2_parser_set_get_thread);

  TCase *tc_mi2_parser_get_location = tcase_create ("mi2_get_location");
  tcase_add_test (tc_mi2_parser_get_location, test_mi2_get_location);
  suite_add_tcase (s, tc_mi2_parser_get_location);
  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = mi2_parser_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
