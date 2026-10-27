#ifndef _MEMINFO_H_
#define _MEMINFO_H_
struct meminfo {
  int total_pages;   // physical pages managed by kalloc
  int free_pages;    // currently on the free list
  int user_pages;    // calling process user space (sz / PGSIZE)
  int pt_pages;      // pages used by its page tables
};
#endif
