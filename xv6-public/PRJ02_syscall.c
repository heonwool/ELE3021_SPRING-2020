#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "elf.h"

// Newly designed system calls
// NOTE: getadmin(), setmemorylimit(), and printproclist() are in the proc.c
int
exec2(char *path, char **argv, int stacksize)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  begin_op();

  if(curproc->mode != ADMINISTRATOR)
  {
    end_op();
    cprintf("exec2: fail (privilege error)\n");
    return -1;
  }

  if(stacksize < 1 || stacksize > 100)
  {
    end_op();
    cprintf("exec2: fail (stacksize out of boundary)\n");
    return -1;
  }

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec2: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate (1+stacksize) pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + (1+stacksize)*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - (1+stacksize)*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}

/*
char *
getshmem(int pid)
{
  // NOT IMPLEMENTED	
}
*/

// Wrapper functions for newly designed system calls
int 
sys_getadmin(void)
{
  char *password;
  if(argstr(0, &password) < 0)
    return -1;

  return getadmin(password);
}

int
sys_exec2(void)
{
  char *path, *argv[MAXARG];
  int stacksize;

  uint uargv, uarg;
  int i;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0 || argint(2, &stacksize) < 0)
    return -1;

  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){ 
    if(i >= NELEM(argv))
     return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
     return -1;
    if(uarg == 0){
     argv[i] = 0;
     break;
    } 
    if(fetchstr(uarg, &argv[i]) < 0)
     return -1;
  } 

  return exec2(path, argv, stacksize);
}

int 
sys_setmemorylimit(void)
{
  int pid, limit;
  if(argint(0, &pid) < 0 || argint(1, &limit) < 0)
    return -1;

  return setmemorylimit(pid, limit);
}

/*
int
sys_checkmemorylimit(void)
{
  int pid;
  uint nbytes;

  if(argint(0, (int*)&pid) < 0 || argint(1, (int*)&nbytes) < 0)
    return -1;

  return checkmemorylimit(pid, nbytes);
}
*/

int
sys_printproclist(void)
{
  printproclist();
  return 0;
}

/*
int
sys_isalive(void)
{
  int pid;
  int(argint(0, &pid) < 0)
    return -1;

  return isalive(pid);
}*/

int
sys_getshmem(void)
{
  int pid;
  if(argint(0, &pid) < 0)
    return -1;

  exit();
  return -1;
}