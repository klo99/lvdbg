#include <stdio.h>

int
print_array (const char **array)
{
  const char **p;
  int i;

  p = array;
  while (*p)
    {
      printf ("Element %i - '%s'", i++, *p++);
    }
}
