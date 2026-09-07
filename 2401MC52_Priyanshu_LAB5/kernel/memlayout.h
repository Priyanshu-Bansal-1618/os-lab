// Physical memory layout.

// QEMU virt machine memory map:
// 00001000 -- boot ROM
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- UART0
// 10001000 -- VirtIO disk
// 80000000 -- kernel load address
// After 80000000 -- available RAM.

// Kernel physical memory layout:
// 80000000 -- kernel entry, text, and data
// end      -- start of kernel heap
// PHYSTOP  -- end of kernel-managed RAM

// UART registers in physical memory.
#define UART0     0x10000000L
#define UART0_IRQ 10

// VirtIO MMIO interface.
#define VIRTIO0     0x10001000
#define VIRTIO0_IRQ 1

// Core-local interrupt controller (CLINT).
#define CLINT_BASE  0x02000000L
#define CLINT(hart) (CLINT_BASE + (hart) * 4)

// Platform-level interrupt controller (PLIC).
#define PLIC                 0x0c000000L
#define PLIC_PRIORITY        (PLIC + 0x0)
#define PLIC_PENDING         (PLIC + 0x1000)
#define PLIC_SENABLE(hart)   (PLIC + 0x2080 + (hart) * 0x100)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart) * 0x2000)
#define PLIC_SCLAIM(hart)    (PLIC + 0x201004 + (hart) * 0x2000)

// Kernel and user RAM range.
#define KERNBASE 0x80000000L
#define PHYSTOP  (KERNBASE + 128 * 1024 * 1024)

// Trampoline mapped at the highest virtual address.
#define TRAMPOLINE (MAXVA - PGSIZE)

// Kernel stacks below the trampoline with guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p) + 1) * 2 * PGSIZE)

// User virtual memory layout:
// text -> data/bss -> stack -> heap -> ... -> trapframe -> trampoline

#define TRAPFRAME (TRAMPOLINE - PGSIZE)

// Fixed virtual address for the shared-memory page.
#define SHMVA (TRAPFRAME - 2*PGSIZE)  // VA used by shm_get()
