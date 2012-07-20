#include <stdio.h>

int
sum (int n)
{
  int i;
  int sum = 0;

  for (i = 1; i <= n; i++)
    sum += i;

  return sum;
}

int
main (int argc, char * argv[])
{
  int ret;
  int N;

  N = 1000;
  ret = sum (N);

  printf ("1 + 2 + ... + %d = %d\n", N, ret);

  return 0;
}
