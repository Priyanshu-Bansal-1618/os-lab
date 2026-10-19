#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  char *p;

  printf(1, "=== vmprint at startup ===\n");
  vmprint();

  p = sbrk(8 * 4096);
  if(p == (char*)-1){ printf(1, "sbrk grow failed\n"); exit(); }
  printf(1, "=== after sbrk(+8 pages) ===\n");
  vmprint();

  if(sbrk(-4096) == (char*)-1){ printf(1, "sbrk shrink failed\n"); exit(); }
  printf(1, "=== after sbrk(-4096) ===\n");
  vmprint();

  exit();
}
