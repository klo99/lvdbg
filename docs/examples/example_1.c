#include <stdlib.h>

int foo(int *p);
int bar(int *p);

int
bar(int *p)
{
  return foo(p);
}

int
foo(int *p)
{
  int n = 0xdead;

  *p = n;

  return *p;
}

int
main(int argc, char *argv[])
{
  int *p = NULL;

  bar(p);

  return 0;
}
