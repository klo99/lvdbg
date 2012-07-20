#include <stdio.h>

int foo (int n);
int bar (int n);

int
bar (int n)
{
  int i;
  int ret = 0;

  for (i = 1; i <= n; i++)
    {
      ret += foo (i);
    }
  return ret;
}

int
foo (int n)
{
  return n*n;
}

int
main (int argc, char *argv[])
{
  int sum;

  sum = bar (100);

  return 0;
}
