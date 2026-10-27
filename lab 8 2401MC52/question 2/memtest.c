#include "types.h"
#include "stat.h"
#include "user.h"
#include "meminfo.h"

static void
show(char *tag)
{
  struct meminfo mi;
  getmeminfo(&mi);
  printf(1, "[%s] total=%d free=%d user=%d pt=%d\n",
         tag, mi.total_pages, mi.free_pages, mi.user_pages, mi.pt_pages);
}

int
main(void)
{
  struct meminfo before, after;
  char *p;
  int pid;

  show("initial");

  p = sbrk(4096 * 8);            /* grow user space by 8 pages */
  if(p == (char*)-1){ printf(1, "sbrk failed\n"); exit(); }
  show("after sbrk +8 pages");

  getmeminfo(&before);           /* snapshot just before fork */
  pid = fork();
  if(pid == 0){
    show("child alive");
    exit();
  }
  show("parent, child alive");
  wait();                        /* reap child -> freevm returns pages */
  getmeminfo(&after);
  show("after child reaped");
  printf(1, "free before fork=%d, after reap=%d -> %s\n",
         before.free_pages, after.free_pages,
         before.free_pages == after.free_pages ? "NO LEAK (equal)" : "LEAK?");
  exit();
}
