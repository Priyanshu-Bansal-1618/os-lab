#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  printf(1, "--- Testing Lazy Allocation ---\n");
  
  char *p = sbrk(32 * 4096);
  
  p[0] = 1;                 
  p[4096] = 2;              
  p[8192] = 3;              
  p[12288] = 4;             
  p[16384] = 5;             

  int faults = getfaults();
  printf(1, "Page faults handled: %d (expected 5)\n", faults);

  printf(1, "Triggering process kill...\n");
  p[33 * 4096] = 1; // Out of bounds

  exit();
}