#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  printf(1, "--- Before sbrk ---\n");
  vmprint();
  
  // Allocate 4 pages (4 * 4096 bytes)
  sbrk(4 * 4096);
  
  printf(1, "\n--- After sbrk ---\n");
  vmprint();
  
  exit();
}