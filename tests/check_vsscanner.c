#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/vsscanner.h"

FILE *OUT_FILE = NULL;
int VERBOSE_LEVEL = 10;

START_TEST (test_vsscanner_create)
{
  vsscanner *scanner;

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  vsscanner_free (scanner);
}
END_TEST

START_TEST (test_vsscanner_add_rule)
{
  vsscanner *scanner;
  int ret;
  int i;
  char *tab[] = { ":lower:", ":upper:", ":alpha:", ":digit:",
    ":alnum:", ":graph:", ":punct:", ":xdigit:"
  };
  char buf[128];

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  ret = vsscanner_add_rule (scanner, "hello", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_add_rule (scanner, "^hello", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_add_rule (scanner, "a[a-z]", 1, 0, 0);
  fail_unless (ret == 0);
  for (i = 0; i < 8; i++)
    {
      sprintf (buf, "a[%s]", tab[i]);
      ret = vsscanner_add_rule (scanner, buf, 1, 0, 0);
      fail_unless (ret == 0);
    }

  vsscanner_free (scanner);
}
END_TEST

START_TEST (test_vsscanner_scan)
{
  id_table ids;
  int ret;
  vsscanner *scanner;

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "hello", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "hello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "?hello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "?hello?", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_free (scanner);

  /* Test multiplier \? \+ * */
  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "he\\?llo", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "hello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "hello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "heello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_free (scanner);

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "mr\\. .", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "mr. X", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);
  vsscanner_restart (scanner);

  ret = vsscanner_scan (scanner, "mr. ?", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);
  vsscanner_restart (scanner);


  vsscanner_free (scanner);


}
END_TEST

START_TEST (test_vsscanner_scan_bracket)
{
  id_table ids;
  int ret;
  vsscanner *scanner;

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "[Hh]ello", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "hello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "Hello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "   Hello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 3);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "?ello", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_free (scanner);

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "ab[cC]*de", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "abcde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abCde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abCcCde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 7);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 4);

  vsscanner_free (scanner);

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "ab[cC]\\+de", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "abcde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abCde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abCcCde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 7);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_free (scanner);

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "ab[cC]\\?de", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "abcde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abCde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abCcCde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 4);

  vsscanner_free (scanner);
}
END_TEST

START_TEST (test_vsscanner_scan_bracket_none)
{
  id_table ids;
  int ret;
  vsscanner *scanner;

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "ab[^cC]*de", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "abcde", &ids);
  fail_unless (ret == 0);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abCde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "ab?de", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 4);

  vsscanner_free (scanner);

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "ab[^cC]\\+de", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "abcde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abCde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abOde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_free (scanner);

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "ab[^cCd]\\?de", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "abcde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "ab?de", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 5);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "ab??de", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "abde", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 0);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 4);

  vsscanner_free (scanner);
}
END_TEST

START_TEST (test_vsscanner_scan_groups)
{
  id_table ids;
  int ret;
  vsscanner *scanner;
  int i;
  char *tab[] = { ":lower:", ":upper:", ":alpha:", ":digit:",
    ":alnum:", ":graph:", ":punct:", ":xdigit:"
  };
  char buf[128];

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  for (i = 0; i < 8; i++)
    {
      sprintf (buf, "%d[[%s]]", i, tab[i]);
      ret = vsscanner_add_rule (scanner, buf, i, 0, 0);
      fail_unless (ret == 0);
    }

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?0a", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 0);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?0A", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?1A", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 1);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?1a", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?2a", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 2);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?2B", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 2);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?22", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?33", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 3);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?3a", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q4A", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 4);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q4a", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 4);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q49", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 4);
  fail_unless (ids.id[0].len == 2);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q4<", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q5A", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 5);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q5a", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 5);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q59", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 5);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q5:", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 5);
  fail_unless (ids.id[0].len == 2);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "Q5 ", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?6:", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 6);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?3a", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?76", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 7);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?7f", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 7);
  fail_unless (ids.id[0].len == 2);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?7F", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 1);
  fail_unless (ids.id[0].id == 7);
  fail_unless (ids.id[0].len == 2);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "?3g", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0);

  vsscanner_free (scanner);

}
END_TEST

START_TEST (test_vsscanner_scan_complex)
{
  id_table ids;
  int ret;
  vsscanner *scanner;
  int i;

  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "\"[^\"]*\"", 1, 0, 0);
  fail_unless (ret == 0);
  ret = vsscanner_add_rule (scanner, "/\\*.*\\*/", 2, 1, 0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "hello \"World\"!", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1);
  fail_unless (ids.id[0].index == 6, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 7);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "hello \"World\"! /* Comment */ 123", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 2);
  fail_unless (ids.id[0].index == 6, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 7);
  fail_unless (ids.id[1].index == 15, "index %d", ids.id[1].index);
  fail_unless (ids.id[1].id == 2, "id %d", ids.id[1]);
  fail_unless (ids.id[1].len == 13);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "hello \"World\"! /** Comment bla ", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 2);
  fail_unless (ids.id[0].index == 6, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 7);
  fail_unless (ids.id[1].index == 15, "index %d", ids.id[1].index);
  fail_unless (ids.id[1].id == 2, "id %d", ids.id[1]);
  fail_unless (ids.id[1].len == 16, "%d != %d", ids.id[1].len, 16);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "  ... Comment bla */  ", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "%d", ids.len);
  fail_unless (ids.id[0].index == 0, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 2, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 20);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "hello \"World\"! /** Comment bla ", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 2);
  fail_unless (ids.id[0].index == 6, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 7);
  fail_unless (ids.id[1].index == 15, "index %d", ids.id[1].index);
  fail_unless (ids.id[1].id == 2, "id %d", ids.id[1]);
  fail_unless (ids.id[1].len == 16);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "  ... Comment bla ...   ", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "%d", ids.len);
  fail_unless (ids.id[0].index == 0, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 2, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 24);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "  ... Comment bla */  ", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "%d", ids.len);
  fail_unless (ids.id[0].index == 0, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 2, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 20);
  vsscanner_free (scanner);


  /* Numbers */
  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret =
    vsscanner_add_rule (scanner, "[[:digit:]]*[\\.,]\\?[[:digit:]]*", 1, 0,
			0);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "x = 123;", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "Len = %d", ids.len);
  fail_unless (ids.id[0].index == 4, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 3);
  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "x = .123;", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "%d", ids.len);
  fail_unless (ids.id[0].index == 4, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 4);
  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "x = .123", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "%d", ids.len);
  fail_unless (ids.id[0].index == 4, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 4);
  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "x = 123.123", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "%d", ids.len);
  fail_unless (ids.id[0].index == 4, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 7);
  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "x = 123,123", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "%d", ids.len);
  fail_unless (ids.id[0].index == 4, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 7);
  vsscanner_free (scanner);


  /* words */
  scanner = vsscanner_create ();
  fail_unless (scanner != NULL);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_add_rule (scanner, "car", 1, 0, 1);
  fail_unless (ret == 0);
  ret = vsscanner_scan (scanner, "car", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 1, "Len = %d", ids.len);
  fail_unless (ids.id[0].index == 0, "index %d", ids.id[1].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[0].len == 3);
  vsscanner_restart (scanner);
  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  ret = vsscanner_scan (scanner, "scar 1car car1 1car", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 0, "%d", ids.len);

  memset (&ids, 0, sizeof (ids));
  ids.size = DEF_IDT_LEN;
  vsscanner_restart (scanner);
  ret = vsscanner_scan (scanner, "car .car car. :car: !car! car", &ids);
  fail_unless (ret == 0);
  fail_unless (ids.len == 6, "Len = %d", ids.len);
  fail_unless (ids.id[0].index == 0, "index %d", ids.id[0].index);
  fail_unless (ids.id[0].id == 1, "id %d", ids.id[0]);
  fail_unless (ids.id[0].len == 3);
  fail_unless (ids.id[1].index == 5, "index %d", ids.id[1].index);
  fail_unless (ids.id[1].id == 1, "id %d", ids.id[1]);
  fail_unless (ids.id[1].len == 3);
  fail_unless (ids.id[2].index == 9, "index %d", ids.id[2].index);
  fail_unless (ids.id[2].id == 1, "id %d", ids.id[2]);
  fail_unless (ids.id[2].len == 3);
  fail_unless (ids.id[3].index == 15, "index %d", ids.id[3].index);
  fail_unless (ids.id[3].id == 1, "id %d", ids.id[3]);
  fail_unless (ids.id[3].len == 3);
  fail_unless (ids.id[4].index == 21, "index %d", ids.id[4].index);
  fail_unless (ids.id[4].id == 1, "id %d", ids.id[4]);
  fail_unless (ids.id[4].len == 3);
  fail_unless (ids.id[5].index == 26, "index %d", ids.id[5].index);
  fail_unless (ids.id[5].id == 1, "id %d", ids.id[5]);
  fail_unless (ids.id[5].len == 3);
  if (ids.size > DEF_IDT_LEN)
    {
      free (ids.extra_id);
    }

  vsscanner_free (scanner);
}
END_TEST

/**
 * \test Test vsscanner.c functions.
 *
 * Test for the simple scanner.
 * - _create: Test the creation.
 * - _add_rule: Test to add rules.
 * - _scan*: Test scanning.
 * - _scan_bracket Test brackets, [abc].
 * - _scan_bracket Test brackets, [^abc].
 * - _scan_complex: Test scanning.
 */
  Suite * vsscanner_suite (void)
{
  Suite *s = suite_create ("vsscanner");

  TCase *tc_vsscanner_create = tcase_create ("vsscanner_create");
  tcase_add_test (tc_vsscanner_create, test_vsscanner_create);
  suite_add_tcase (s, tc_vsscanner_create);

  TCase *tc_vsscanner_add_rule = tcase_create ("vsscanner_add_rule");
  tcase_add_test (tc_vsscanner_add_rule, test_vsscanner_add_rule);
  suite_add_tcase (s, tc_vsscanner_add_rule);

  TCase *tc_vsscanner_scan = tcase_create ("vsscanner_scan");
  tcase_add_test (tc_vsscanner_scan, test_vsscanner_scan);
  suite_add_tcase (s, tc_vsscanner_scan);

  TCase *tc_vsscanner_scan_bracket = tcase_create ("vsscanner_scan_bracket");
  tcase_add_test (tc_vsscanner_scan_bracket, test_vsscanner_scan_bracket);
  suite_add_tcase (s, tc_vsscanner_scan_bracket);

  TCase *tc_vsscanner_scan_bracket_none =
    tcase_create ("vsscanner_scan_bracket_none");
  tcase_add_test (tc_vsscanner_scan_bracket_none,
		  test_vsscanner_scan_bracket_none);
  suite_add_tcase (s, tc_vsscanner_scan_bracket_none);

  TCase *tc_vsscanner_scan_groups = tcase_create ("vsscanner_scan_groups");
  tcase_add_test (tc_vsscanner_scan_groups, test_vsscanner_scan_groups);
  suite_add_tcase (s, tc_vsscanner_scan_groups);

  TCase *tc_vsscanner_scan_complex = tcase_create ("vsscanner_scan_complex");
  tcase_add_test (tc_vsscanner_scan_complex, test_vsscanner_scan_complex);
  suite_add_tcase (s, tc_vsscanner_scan_complex);
  return s;
}

int
main (void)
{
  int number_failed;

  OUT_FILE = stdout;
  Suite *s = vsscanner_suite ();
  SRunner *sr = srunner_create (s);

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
