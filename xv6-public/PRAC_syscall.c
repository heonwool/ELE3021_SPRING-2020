#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

// simple system call
int myfunction(char *str) {
	cprintf("%s\n", str);
	return 0xABCDABCD;
}

// wrapper for my_syscall
int sys_myfunction(void) {
	char *str;

	// decode argument using argstr
	if (argstr(0, &str) < 0)
		return -1;

	return myfunction(str);
}

// getppid()
int getppid(void) {
	return myproc()->parent->pid;
}

// wrapper for getppid()
int sys_getppid(void) {
	return getppid();
}
