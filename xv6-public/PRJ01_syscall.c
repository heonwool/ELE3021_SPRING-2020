#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"


#ifdef MLFQ_SCHED
int sys_yield(void) {
	yield2();
	return 0;
}
#else
// wrapper for yield()
int sys_yield(void) {
	yield();
	return 0;
}
#endif

int
sys_getlev(void)
{
  return getlev();
}

int
sys_setpriority(void)
{
  int pid, priority;

  if(argint(0, &pid) < 0)
    return -1;

  if(argint(1, &priority) < 0)
    return -1;

  return setpriority(pid, priority);
}