#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define MIN_PRIORITY 0
#define MAX_PRIORITY 10

// Time quantum = 2i + 4 ticks
// L0 =  4 ticks
// L1 =  6 ticks
// L2 =  8 ticks
// L3 = 10 ticks
// L4 = 12 ticks

const int TIME_QUANTUM[5] = {4, 6, 8, 10, 12};

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);
extern char *strcpy(char *s, const char *t);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  // For Project #1
  p->time_c = ticks;

  #ifdef MLFQ_SCHED
  p->priority = MIN_PRIORITY;
  p->level = 0;
  p->hold = TIME_QUANTUM[0];
  #endif

  // For Project #2
  p->mode = USER;
  p->limit = 0;

  // For Project #3
  strcpy(p->userid, "root");

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz, limit;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  limit = curproc->limit;
  if(n > 0){
    if(sz + n <= limit || limit == 0){
      if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
        return -1;
    } else{
      return -1;
    }
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  
  //cprintf("CURRENT: PID %d -> sz %d limit %d nbytes %d\n", curproc->pid, sz, limit, n);

  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // For Project #2
  np->mode = curproc->mode;
  np->limit = curproc->limit;

  // For Project #3
  strcpy(np->userid, curproc->userid);

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

int
fork2(char *username)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // For Project #2
  np->mode = curproc->mode;
  np->limit = curproc->limit;

  // For Project #3
  strcpy(np->userid, username);

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

int
sys_fork2(void)
{
  char *username;
  if(argstr(0, &username) < 0)
    return -1;

  return fork2(username);
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

#ifdef MULTILEVEL_SCHED 
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    // Check processes that has even pids
EXIST_RUNNABLE:
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      if(p->pid % 2 == 0){
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        swtch(&(c->scheduler), p->context);
        switchkvm();

        c->proc = 0;
      }
    }

    // After the end of traversal, re-check the ptable.proc 
    // whether it has runnable even pid processes or not.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE && p->pid % 2 == 0)
        goto EXIST_RUNNABLE;
    }

    // Check processes that has odd pids
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // ptable.proc traversal for finding minimum odd pid
      struct proc *temp_p;
      for(temp_p = ptable.proc; temp_p < &ptable.proc[NPROC]; temp_p++){
        // If there is an even pid process that is RUNNABLE, jump to EXIST_RUNNABLE!
        if(p->state == RUNNABLE && p->pid % 2 == 0)
          goto EXIST_RUNNABLE;

        if(temp_p->state == RUNNABLE && temp_p->pid % 2 != 0 && p->time_c > temp_p->time_c)
          p = temp_p;
      }

      if(p->pid % 2 != 0){
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        swtch(&(c->scheduler), p->context);
        switchkvm();

        c->proc = 0;
      }
    }
    release(&ptable.lock);
  }
}

#elif MLFQ_SCHED
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  struct proc *new_p, *temp_p;
  int now_level;
  int flag = 1;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    // Boost the priority every 100 ticks
    if(ticks % 100 == 0 && flag){
      flag = 0;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        p->level = 0;
        p->hold = 4;
      }
    }

    if(ticks % 100 != 0)
      flag = 1;

    // If there is a process who used all of ther time quantum, move to the next queue.
    // Also, if there is a SLEEPING process, put it in L0 queue.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->level < MLFQ_K - 1 && p->hold <= 0){
        p->level++;
        p->hold = TIME_QUANTUM[p->level];
      }
      if(p->state == SLEEPING){
        p->level = 0;
        p->hold = TIME_QUANTUM[0];
      }
    }

    // Check the queue level that has RUNNABLE process
    now_level = MLFQ_K;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE && p->hold > 0 && p->level < now_level) {
        now_level = p->level;
      }
    }

    // If there are no such process that satisfy the above condition,
    // boost the priority and continue;
    if(now_level == MLFQ_K){
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        p->level = 0;
        p->hold = 4;
      }
      release(&ptable.lock);
      continue;
    }

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if(p->state != RUNNABLE || p->level != now_level || p->hold <= 0)
        continue;

      new_p = p;
      for(temp_p = ptable.proc; temp_p < &ptable.proc[NPROC]; temp_p++){
        if(temp_p->state != RUNNABLE || temp_p->level != now_level || temp_p->hold <= 0)
          continue;
        if(temp_p->priority > new_p->priority)
          new_p = temp_p;
      }

      p = new_p;

      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      if(p->hold > 0)
        p->hold--;
      c->proc = 0;
    }
    release(&ptable.lock);
  }
}

#else
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}
#endif

// Newly defined system calls for Project #1
void
yield2(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  myproc()->level = 0;
  myproc()->hold = TIME_QUANTUM[0];
  sched();
  release(&ptable.lock);
}

int 
getlev(void)
{
  return myproc()->level;
}

int
setpriority(int pid, int priority)
{
  struct proc *p;
  struct proc *parent = myproc();

  int flag = 0;
  int retval = -1;

  acquire(&ptable.lock);

  // Checking invalid priority   
  if(priority < MIN_PRIORITY || priority > MAX_PRIORITY)
    retval = -2;

  else{
    // Check (1) whether the given pid is child or not 
    //       (2) existence of the given pid
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->pid == pid && p->parent->pid == parent->pid){
        p->priority = priority;
        flag = 1;
      }
    }
  }

  if(flag)
    retval = 0;

  release(&ptable.lock);
  return retval;
}

// Newly defined system calls for Project #2

// temporarily copied this function from ulib.c
int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

int 
getadmin(char *password)
{
  char *correct = "2016024875";
  int flag;

  acquire(&ptable.lock);

  if(strcmp(password, correct) == 0)
  {
    struct proc *p;

    p = myproc();
    p->mode = ADMINISTRATOR;
  
    flag = 0;
  }

  else
    flag = -1;

  release(&ptable.lock);
  return flag;
}

int 
setmemorylimit(int pid, int limit)
{
  struct proc *p = myproc();

  // Check whether the input value is negative or it has ADMINISTRATOR privilege
  if(limit < 0 || p->mode != ADMINISTRATOR){
    cprintf("setmemorylimit: fail (limit < 0 or privilege error)\n");
    return -1;
  }

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid && p->state > UNUSED) {
      acquire(&ptable.lock);

      // If limit < p->sz, return -1
      if(p->sz > limit && limit != 0){
        release(&ptable.lock);
        cprintf("setmemorylimit: fail (designated limit is smaller than current sz)\n");
        return -1;
      }

      else{
        p->limit = limit;
        release(&ptable.lock);
        return 0;
      }

    }
  }

  // If there is no such PID in ptable.proc, return -1
  cprintf("setmemorylimit: fail (no such pid)\n");
  return -1;
}

/*
int
checkmemorylimit(int pid, uint nbytes)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid && p->pid != UNUSED){
      if(p->limit == 0 || (p->sz + nbytes) <= p->limit)
        return 1;
    }
  }

  return 0;
}
*/

char*
getprocstate(int num)
{
  char *result;

  switch(num){
  case EMBRYO:
    result = "EMBRYO";
    break;

  case SLEEPING:
    result = "SLEEPING";
    break;

  case RUNNABLE:
    result = "RUNNABLE";
    break;

  case RUNNING:
    result = "RUNNING";
    break;

  case ZOMBIE:
    result = "ZOMBIE";
    break;

  default:
    result = "NULL";
  }

  return result;  
}

void
printproclist(void)
{
  struct proc *p;
  char *pstate;

  acquire(&ptable.lock);
  cprintf("NAME | PID | UID | State | Elapsed Time (ticks) | Memory Size (bytes) | Limit (bytes)\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != UNUSED){
      pstate = getprocstate(p->state);
      cprintf("%s | %d | %s | %s | %d | %d | %d\n", p->name, p->pid, p->userid, pstate, ticks - p->time_c, p->sz, p->limit);
    }
  }
  cprintf("\n");
  release(&ptable.lock);
}

int
isalive(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->killed == 0 && p->pid == pid){
      return 1;
    }
  }
  release(&ptable.lock);
  return 0;
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
