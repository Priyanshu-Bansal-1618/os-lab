// clang-format off

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// bio.c — Buffer cache management and disk block I/O
void            binit(void);
struct buf*     bread(uint, uint);
void            brelse(struct buf*);
void            bwrite(struct buf*);
void            bpin(struct buf*);
void            bunpin(struct buf*);

// console.c — Kernel console initialization, input, and output handling
void            consoleinit(void);
void            consoleintr(int);
void            consputc(int);

// exec.c — Loading and executing a new user program
int             kexec(char*, char**);

// file.c — File allocation, reference management, and file I/O
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, uint64, int n);
int             filestat(struct file*, uint64 addr);
int             filewrite(struct file*, uint64, int n);

// fs.c — Filesystem, inode, directory, and file data operations
void            fsinit(int);
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);
void            iinit();
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, int, uint64, uint, uint);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, int, uint64, uint, uint);
void            itrunc(struct inode*);
void            ireclaim(int);

// kalloc.c — Physical memory allocation and deallocation
void*           kalloc(void);
void            kfree(void *);
void            kinit(void);

// log.c — Filesystem journaling and transaction management
void            initlog(int, struct superblock*);
void            log_write(struct buf*);
void            begin_op(void);
void            end_op(void);

// pipe.c — Kernel pipe creation, reading, writing, and closing
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
int             piperead(struct pipe*, uint64, int);
int             pipewrite(struct pipe*, uint64, int);

// printk.c — Kernel formatted output, logging, and panic handling
int             printk(char*, ...) __attribute__ ((format (printf, 1, 2)));
void            panic(char*) __attribute__((noreturn));
void            printkinit(void);

// proc.c — Process management, scheduling, synchronization, and memory
int             cpuid(void);
void            kexit(int);
int             kfork(void);
int             growproc(int);
void            proc_mapstacks(pagetable_t);
pagetable_t     proc_pagetable(struct proc *);
void            proc_freepagetable(pagetable_t, uint64);
int             kkill(int);
int             killed(struct proc*);
void            setkilled(struct proc*);
struct cpu*     mycpu(void);
struct proc*    myproc();
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            sleep_prepare(void*);
void            sleep(void);
void            userinit(void);
int             kwait(uint64);
void            wakeup(void*);
void            yield(void);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);
void            procdump(void);

// swtch.S — Low-level CPU context switching between execution contexts
void            swtch(struct context*, struct context*);

// spinlock.c — Spinlock initialization, acquisition, and release
void            acquire(struct spinlock*);
int             holding(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);
void            push_off(void);
void            pop_off(void);

// sleeplock.c — Sleep-lock initialization and blocking synchronization
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);
void            initsleeplock(struct sleeplock*, char*);

// string.c — Kernel memory and string manipulation utilities
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// syscall.c — System call argument fetching and system call dispatch
void            argint(int, int*);
int             argstr(int, char*, int);
void            argaddr(int, uint64 *);
int             fetchstr(uint64, char*, int);
int             fetchaddr(uint64, uint64*);
void            syscall();

// trap.c — Trap handling, timer interrupts, and kernel return preparation
extern uint     ticks;
void            trapinit(void);
void            trapinithart(void);
extern struct spinlock tickslock;
void            prepare_return(void);

// uart.c — UART initialization, interrupts, and serial communication
void            uartinit(void);
void            uartintr(void);
void            uartwrite(char [], int);
void            uartputc_sync(int);

// vm.c — Virtual memory management, page tables, and memory copying
void            kvminit(void);
void            kvminithart(void);
void            kvmmap(pagetable_t, uint64, uint64, uint64, int);
int             mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t     uvmcreate(void);
uint64          uvmalloc(pagetable_t, uint64, uint64, int);
uint64          uvmdealloc(pagetable_t, uint64, uint64);
int             uvmcopy(pagetable_t, pagetable_t, uint64);
void            uvmfree(pagetable_t, uint64);
void            uvmunmap(pagetable_t, uint64, uint64, int);
void            uvmclear(pagetable_t, uint64);
pte_t *         walk(pagetable_t, uint64, int);
uint64          walkaddr(pagetable_t, uint64);
int             copyout(pagetable_t, uint64, uint64, char *, uint64);
int             copyin(pagetable_t, uint64, char *, uint64, uint64);
int             copyinstr(pagetable_t, uint64, char *, uint64, uint64);
int             ismapped(pagetable_t, uint64);
uint64          vmfault(pagetable_t, uint64, uint64, int);

// plic.c — Platform-Level Interrupt Controller initialization and handling
void            plicinit(void);
void            plicinithart(void);
int             plic_claim(void);
void            plic_complete(int);

// virtio_disk.c — VirtIO disk initialization, I/O, and interrupt handling
void            virtio_disk_init(void);
void            virtio_disk_rw(struct buf *, int);
void            virtio_disk_intr(void);

// shm.c — Shared memory allocation and process address-space cleanup
void            shminit(void);
uint64          shm_get(void);
void            shm_unmap(struct proc*);

// sem.c — Kernel semaphore initialization and synchronization operations
void            seminit(void);
int             sem_init(int, int);
int             sem_wait(int);
int             sem_signal(int);

// Number of elements in a fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
