#include <stdio.h>

int
main (int argc, char *argv[])
{
  int ch;

  fprintf (stdout, "Hello word!\nHit '9' key to continue.\n");

  do {
    ch = getc (stdin);
    fprintf (stdout, "You pressed '%c'\n", ch);
  } while (ch != '9');

  return 0;
}
