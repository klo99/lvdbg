#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/input.h"

/*
 * NOTE:
 * ~~~~
 *
 * These definitions comes from ncurses.h!
 */
#define KEY_DOWN	0402	/* down-arrow key */
#define KEY_UP		0403	/* up-arrow key */
#define KEY_LEFT	0404	/* left-arrow key */
#define KEY_RIGHT	0405	/* right-arrow key */
#define KEY_HOME	0406	/* home key */
#define KEY_BACKSPACE	0407	/* backspace key */
#define KEY_F0		0410	/* Function keys.  Space for 64 */
#define KEY_F(n)	(KEY_F0+(n))	/* Value of function key n */
#define KEY_DL		0510	/* delete-line key */
#define KEY_IL		0511	/* insert-line key */
#define KEY_DC		0512	/* delete-character key */
#define KEY_IC		0513	/* insert-character key */
#define KEY_EIC		0514	/* sent by rmir or smir in insert mode */
#define KEY_CLEAR	0515	/* clear-screen or erase key */
#define KEY_EOS		0516	/* clear-to-end-of-screen key */
#define KEY_EOL		0517	/* clear-to-end-of-line key */
#define KEY_SF		0520	/* scroll-forward key */
#define KEY_SR		0521	/* scroll-backward key */
#define KEY_NPAGE	0522	/* next-page key */
#define KEY_PPAGE	0523	/* previous-page key */
#define KEY_STAB	0524	/* set-tab key */
#define KEY_CTAB	0525	/* clear-tab key */
#define KEY_CATAB	0526	/* clear-all-tabs key */
#define KEY_ENTER	0527	/* enter/send key */
#define KEY_PRINT	0532	/* print key */
#define KEY_LL		0533	/* lower-left key (home down) */
#define KEY_A1		0534	/* upper left of keypad */
#define KEY_A3		0535	/* upper right of keypad */
#define KEY_B2		0536	/* center of keypad */
#define KEY_C1		0537	/* lower left of keypad */
#define KEY_C3		0540	/* lower right of keypad */
#define KEY_BTAB	0541	/* back-tab key */
#define KEY_BEG		0542	/* begin key */
#define KEY_CANCEL	0543	/* cancel key */
#define KEY_CLOSE	0544	/* close key */
#define KEY_COMMAND	0545	/* command key */
#define KEY_COPY	0546	/* copy key */
#define KEY_CREATE	0547	/* create key */
#define KEY_END		0550	/* end key */
#define KEY_EXIT	0551	/* exit key */
#define KEY_FIND	0552	/* find key */
#define KEY_HELP	0553	/* help key */
#define KEY_MARK	0554	/* mark key */
#define KEY_MESSAGE	0555	/* message key */
#define KEY_MOVE	0556	/* move key */
#define KEY_NEXT	0557	/* next key */
#define KEY_OPEN	0560	/* open key */
#define KEY_OPTIONS	0561	/* options key */
#define KEY_PREVIOUS	0562	/* previous key */
#define KEY_REDO	0563	/* redo key */
#define KEY_REFERENCE	0564	/* reference key */
#define KEY_REFRESH	0565	/* refresh key */
#define KEY_REPLACE	0566	/* replace key */
#define KEY_RESTART	0567	/* restart key */
#define KEY_RESUME	0570	/* resume key */
#define KEY_SAVE	0571	/* save key */
#define KEY_SBEG	0572	/* shifted begin key */
#define KEY_SCANCEL	0573	/* shifted cancel key */
#define KEY_SCOMMAND	0574	/* shifted command key */
#define KEY_SCOPY	0575	/* shifted copy key */
#define KEY_SCREATE	0576	/* shifted create key */
#define KEY_SDC		0577	/* shifted delete-character key */
#define KEY_SDL		0600	/* shifted delete-line key */
#define KEY_SELECT	0601	/* select key */
#define KEY_SEND	0602	/* shifted end key */
#define KEY_SEOL	0603	/* shifted clear-to-end-of-line key */
#define KEY_SEXIT	0604	/* shifted exit key */
#define KEY_SFIND	0605	/* shifted find key */
#define KEY_SHELP	0606	/* shifted help key */
#define KEY_SHOME	0607	/* shifted home key */
#define KEY_SIC		0610	/* shifted insert-character key */
#define KEY_SLEFT	0611	/* shifted left-arrow key */
#define KEY_SMESSAGE	0612	/* shifted message key */
#define KEY_SMOVE	0613	/* shifted move key */
#define KEY_SNEXT	0614	/* shifted next key */
#define KEY_SOPTIONS	0615	/* shifted options key */
#define KEY_SPREVIOUS	0616	/* shifted previous key */
#define KEY_SPRINT	0617	/* shifted print key */
#define KEY_SREDO	0620	/* shifted redo key */
#define KEY_SREPLACE	0621	/* shifted replace key */
#define KEY_SRIGHT	0622	/* shifted right-arrow key */
#define KEY_SRSUME	0623	/* shifted resume key */
#define KEY_SSAVE	0624	/* shifted save key */
#define KEY_SSUSPEND	0625	/* shifted suspend key */
#define KEY_SUNDO	0626	/* shifted undo key */
#define KEY_SUSPEND	0627	/* suspend key */
#define KEY_UNDO	0630	/* undo key */
#define KEY_MOUSE	0631	/* Mouse event has occurred */
#define KEY_RESIZE	0632	/* Terminal resize event */
#define KEY_EVENT	0633	/* We were interrupted by an event */

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 7;
int GLOB_RETURN = 0;
int stdscr = 0;
int g_buf[64];
int g_index;
int g_index_max;
int g_function;
int g_action;
int g_param;
int g_group;
int g_diss;
char *g_string = NULL;
int g_got_tag;
int g_win_type;
view *g_view;
mi2_interface *g_mi2;
view *g_view;

int
wgetch (int scr)
{
  if (g_index >= g_index_max)
    {
      return -1;
    }

  return g_buf[g_index++];
}

int
mi2_do_action (mi2_interface * mi2, int action, int param)
{
  g_function = 1;
  g_action = action;
  g_mi2 = mi2;
  g_param = param;
  return GLOB_RETURN;
}

void
mi2_toggle_disassemble (mi2_interface * mi2)
{
  g_diss = 1;
}

int
view_next_window (view * view, int dir, int type)
{
  g_function = 2;
  g_param = dir;
  g_view = view;
  g_group = type;

  return GLOB_RETURN;
}

int
view_scroll_up (view * view)
{
  g_function = 3;
  g_view = view;

  return GLOB_RETURN;
}

int
view_scroll_down (view * view)
{
  g_function = 4;
  g_view = view;

  return GLOB_RETURN;
}

int
view_get_tag (view * view, int *win)
{
  g_got_tag = *win;
  *win = g_win_type;
  return GLOB_RETURN;
}

int
view_add_message (view * view, int level, const char *msg, ...)
{
  g_function = 6;
  return GLOB_RETURN;
}

int
view_show_file (view * view, const char *file_name, int line, int mark_stop)
{
  g_function = 7;
  return GLOB_RETURN;
}

void
view_toggle_view_mode (view * view)
{
}

int
form_selection (char **list, const char *header)
{
  g_function = 8;
  if (g_string)
    *list = g_string;
  return GLOB_RETURN;
}

START_TEST (test_input_create)
{
  input *input;
  int ret;

  input =
    input_create ((view *) 42, (mi2_interface *) 43, (configuration *) 44, 0);
  fail_unless (input != NULL);

  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'q';
  ret = input_get_input (input);
  fail_unless (ret < 0);

  /* Key down */
  g_view = NULL;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_DOWN;
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 3);
  fail_unless (g_view == (view *) 42);

  /* Key up */
  g_view = NULL;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_UP;
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 4);
  fail_unless (g_view == (view *) 42);

  /* Key right */
  g_view = NULL;
  g_group = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_RIGHT;
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 2);
  fail_unless (g_param == 1);
  fail_unless (g_group == 0);
  fail_unless (g_view == (view *) 42);

  /* Key left */
  g_view = NULL;
  g_group = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_LEFT;
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 2);
  fail_unless (g_param == -1);
  fail_unless (g_group == 0);
  fail_unless (g_view == (view *) 42);

  /* Key \t */
  g_view = NULL;
  g_group = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = '\t';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 2);
  fail_unless (g_param == 1);
  fail_unless (g_group == 1);
  fail_unless (g_view == (view *) 42);

  /* Key Shift \t */
  g_view = NULL;
  g_group = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_BTAB;
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 2);
  fail_unless (g_param == -1);
  fail_unless (g_group == 1);
  fail_unless (g_view == (view *) 42);

  /* Key Ctrl right */
  g_view = NULL;
  g_group = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 6;
  g_buf[0] = 0x1b;
  g_buf[1] = 0x5b;
  g_buf[2] = 0x31;
  g_buf[3] = 0x3b;
  g_buf[4] = 0x35;
  g_buf[5] = 0x44;
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 2);
  fail_unless (g_param == 1);
  fail_unless (g_group == 2);
  fail_unless (g_view == (view *) 42);

  /* Key Ctrl left */
  g_view = NULL;
  g_group = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 6;
  g_buf[0] = 0x1b;
  g_buf[1] = 0x5b;
  g_buf[2] = 0x31;
  g_buf[3] = 0x3b;
  g_buf[4] = 0x35;
  g_buf[5] = 0x43;
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 2);
  fail_unless (g_param == -1);
  fail_unless (g_group == 2);
  fail_unless (g_view == (view *) 42);

  /* Key '\r' */
  g_mi2 = 0;
  g_param = -1;
  g_action = -1;
  g_got_tag = -2;
  GLOB_RETURN = 0;
  g_win_type = WIN_STACK;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = '\r';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_param == 0);
  fail_unless (g_got_tag == -1);
  fail_unless (g_mi2 == (mi2_interface *) 43);
  fail_unless (g_action == ACTION_STACK_LIST_VARIABLES);

  /* '\r' Wrong window type when view_get_tag. */
  g_function = -2;
  g_param = -1;
  g_mi2 = 0;
  g_action = -1;
  g_got_tag = -2;
  GLOB_RETURN = 0;
  g_win_type = 99;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = '\r';
  ret = input_get_input (input);
  fail_unless (ret == 0);	/* Still return 0 as -1 is quitting. */
  fail_unless (g_function == -2);
  fail_unless (g_param == -1);
  fail_unless (g_got_tag == -1);
  fail_unless (g_action == -1);

  /* '\r' In main window. */
  g_function = -2;
  g_param = -1;
  g_mi2 = 0;
  g_action = -1;
  g_got_tag = -2;
  GLOB_RETURN = 0;
  g_win_type = WIN_MAIN;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = '\r';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == -2);
  fail_unless (g_param == -1);
  fail_unless (g_got_tag == -1);
  fail_unless (g_action == -1);

  /* '\r' In thread window. */
  g_function = -2;
  g_param = -1;
  g_mi2 = 0;
  g_action = -1;
  g_got_tag = -2;
  GLOB_RETURN = 0;
  g_win_type = WIN_THREADS;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = '\r';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_param == 0);
  fail_unless (g_got_tag == -1);
  fail_unless (g_action == ACTION_INT_UPDATE);

  /* Test F3 - start */
  g_param = -1;
  g_mi2 = 0;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_F (3);
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == 0);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test F4 - start */
  g_string = strdup ("Something");
  g_param = -1;
  g_mi2 = 0;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_F (4);
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 7);
  fail_unless (g_action == 0);

  /* Test F4 - bad */
  g_string = strdup ("Something");
  g_param = -1;
  g_mi2 = 0;
  GLOB_RETURN = -1;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_F (4);
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 8);
  fail_unless (g_action == 0);

  /* Test F4 - really bad */
  g_string = strdup ("Something");
  g_param = -1;
  g_mi2 = 0;
  GLOB_RETURN = -2;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_F (4);
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 6);

  /* Test F5 - symfiles */
  g_param = -1;
  g_mi2 = 0;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = KEY_F (5);
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_FILE_LIST_EXEC_SORCES);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'd' */
  g_param = -1;
  g_diss = 0;
  g_mi2 = 0;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'd';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_diss == 1);
  fail_unless (g_action == ACTION_DATA_DISASSEMBLE);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 255;
  ret = input_get_input (input);
  fail_unless (ret == 0);

  g_index = 0;
  g_index_max = 2;
  g_buf[0] = 255;
  g_buf[1] = 255;
  ret = input_get_input (input);
  fail_unless (ret == 0);

  input_free (input);
}
END_TEST

START_TEST (test_input_break)
{
  input *input;
  int ret;

  input =
    input_create ((view *) 42, (mi2_interface *) 43, (configuration *) 44, 0);
  fail_unless (input != NULL);

  /* breakpoints 'b' and 'B' */
  /* Test 'b' - break insert/remove */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'b';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_BP_SIMPLE);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'B' - Next */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'B';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_BP_ADVANCED);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'w' - Next */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'w';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_BP_WATCHPOINT);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  input_free (input);
}
END_TEST

START_TEST (test_input_exec)
{
  input *input;
  int ret;

  input =
    input_create ((view *) 42, (mi2_interface *) 43, (configuration *) 44, 0);
  fail_unless (input != NULL);

  /* Test 'c' - continue */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'c';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_CONT);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'C' - Continue advanced */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'C';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_CONT_OPT);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'f' - finish */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'f';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_FINISH);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* test 'i' - interrupt */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'i';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_INTR);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'I' - Interrupt advanced */
  g_mi2 = 0;
  g_param = -1;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'I';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_param == 1);
  fail_unless (g_action == ACTION_EXEC_INTR);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'J' - Jump */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'J';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_JUMP);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'n' - Next */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'n';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_NEXT);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'N' - Next instruction */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'N';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_NEXTI);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'step' - Step */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 's';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_STEP);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'step' - Step Instruction */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'S';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_STEPI);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'r' - Next */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'r';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_RUN);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'R' - Next */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'R';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_RETURN);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  /* Test 'u' - Next */
  g_mi2 = 0;
  g_action = -1;
  GLOB_RETURN = 0;
  g_index = 0;
  g_index_max = 1;
  g_buf[0] = 'U';
  ret = input_get_input (input);
  fail_unless (ret == 0);
  fail_unless (g_function == 1);
  fail_unless (g_action == ACTION_EXEC_UNTIL);
  fail_unless (g_mi2 == (mi2_interface *) 43);

  input_free (input);
}
END_TEST

/**
 * @test Test input.c functions.
 *
 * Test that input's exeutes the default actions. Test non valid
 * inputs.
 *
 * - _break: Test break groups commands.
 * - _exec: Test exec groups commands.
 */
  Suite * input_suite (void)
{
  Suite *s = suite_create ("input");

  TCase *tc_input_create = tcase_create ("input_create");
  tcase_add_test (tc_input_create, test_input_create);
  suite_add_tcase (s, tc_input_create);

  TCase *tc_input_break = tcase_create ("input_break");
  tcase_add_test (tc_input_break, test_input_break);
  suite_add_tcase (s, tc_input_break);

  TCase *tc_input_exec = tcase_create ("input_exec");
  tcase_add_test (tc_input_exec, test_input_exec);
  suite_add_tcase (s, tc_input_exec);

  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = input_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
