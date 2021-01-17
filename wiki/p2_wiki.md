### Project 2: Advanced Process Management

<br>

#### 1. 디자인 Design

<br>

###### 1) Project Specification

- Administrator mode

  - 프로세스의 실행 모드를 user mode / administrator mode로 구분한다. 프로세스가 실행되면 기본으로 user mode로 시작하며, 시스템 콜을 통해 관리자 권한을 획득할 수 있다.

  - 관리자 권한을 획득한 프로세스는 일부 시스템콜을 사용할 수 있게 된다. (getadmin, setmemorylimit, etc.)

  - 위 기능을 위해 구현해야할 시스템 콜은 다음과 같다:

    > int getadmin(char *password);
    >
    > 올바른 암호는 자신의 학번이고, 입력받은 password가 학번과 일치할 시 이 시스템 콜을 호출한 프로세스는 관리자 권한을 획득하고 getadmin 함수는 0을 반환한다. 일치하지 않을 경우는 -1을 반환한다.

- Custom stack size

  - 기존의 xv6 프로세스는 exec시 하나의 스택용 페이지와 하나의 가드용 페이지를 할당받는다. 이 기능에서는 스택용 페이지를 처음 실행시 원하는 개수만큼 할당받을 수 있게 하는 시스템콜을 구현한다.

    > int exec2(char *path, char **argv, int stacksize);
    >
    > 이 시스템 콜은 administrator mode에서만 호출할 수 있다.
    >
    > 첫 번째와 두 번째 인자의 의미는 기존 exec 시스템 콜의 첫 번째, 두 번째 인자와 동일하며, stacksize에는 스택용 페이지의 개수를 전달받는다. (stacksize는 1 이상 100 이하의 정수여야 하며, 가드용 페이지는 이 크기에 관계없이 반드시 1개를 할당해야 한다.)

- Per-process memory limit

  - 이 기능에서는 특정 프로세스에 대해 할당받을 수 있는 메모리의 최대치를 지정할 수 있게 한다. 프로세스가 처음 생성될 때에는 제한이 없으며, 이후 시스템 콜을 통해 이 제한을 설정할 수 있다. 

  - 유저 프로세스가 메모리를 추가로 할당받으려 할 때 그 프로세스의 memory limit을 넘으려는 경우 변경하지 않도록 해야한다. 단, 이 과제에서는 프로세스가 사용한 메모리 양을 프로세스 구조체의 sz의 크기로 한정한다.

    > int setmemorylimit(int pid, int limit);
    >
    > pid는 memory limit을 지정할 프로세스의 pid이며, limit은 그 프로세스가 사용할 수 있는 memory limit을 바이트 단위로 나타낸다. 이미 그 프로세스가 할당받은 메모리보다 제한을 작게 설정하려는 경우 제한을 변경하지 않고 -1을 반환한다. pid가 존재하지 않는 경우, limit이 음수인 경우 등에도 -1을 반환한다. 정상적으로 동작했다면 0을 반환한다.

- Shared memory

  - 여러 프로세스가 동시에 접근할 수 있는 shared memory를 만든다. 각 프로세스가 생성될 때 그 프로세스 소유의 shared memory를 위한 페이지가 하나 할당된다. 이 페이지는 어떤 프로세스든 read는 가능하지만, write는 그 페이지의 소유 프로세스만 가능하다. (소유자가 아닌 프로세스가 해당 페이지에 write를 시도할 경우, page fault exception이 발생해야 한다.)

  - 프로세스가 종료될 때 shared memory 또한 올바르게 회수가 되어야 하며, shared memory는 프로세스 구조체의 sz 크기에 반영하지 않는다.

    > char *getshmem(int pid);
    >
    > pid가 소유한 shared memory 페이지의 시작 주소를 반환한다.

- Process manager

  - 현재 실행중인 프로세스들의 세부 정보를 확인하고 쉽게 관리할 수 있는 유저 프로그램을 만든다. 프로그램의 이름은 pmanager이며, pmanager는 시작할 때 getadmin 시스템 콜을 사용하여 관리자 권한을 획득해야 한다. 

  - pmanager는 다음과 같은 명령어들을 지원해야 한다.

    > list
    >
    > ​	현재 실행중인 프로세스들의 정보를 출력한다.
    >
    > kill <pid>
    >
    > ​	pid를 가진 프로세스를 kill한다.
    >
    > execute [path] [stacksize]
    >
    > ​	path의 경로에 위치한 프로그램을 stacksize 개수만큼의 스택용 페이지와 함께 실행하게 한다. 
    >
    > ​	프로그램에 넘겨주는 인자는 하나로, 0번 인자에 path를 넣어준다.
    >
    > ​	실행한 프로그램의 종료를 기다리지 않고, pmanager는 이어서 실행되어야 한다.
    >
    > memlim [pid] [limit]
    >
    > ​	입력받은 pid를 가진 프로세스의 메모리 제한을 limit으로 설정한다.
    >
    > exit
    >
    > ​	pmanager를 종료한다.

<br>

###### 2) Initial Idea

- Administrator mode

  To implement Administrator mode, I implemented an additional variable mode to the struct proc. In the struct proc, there is a variable which indicates the process state (such as ZOMBIE). Like this, mode indicates the execution mode of the process.

- Custom stack size

  I focused on understanding and modifying the existing system call exec().

- Per-process memory limit

  I started implementing this by declaring an additional variable limit inside the struct proc, as in administrator mode. 

- Process manager

  I had a lot of trouble in parsing the input command and implementing the execute function. For parsing the command, I decided to implement a new function by referring to a standard C function, such as the strtok() function. Also, in the case of execute, I referred to how sh.c executes the BACK command in runcmd() function.

<br>

#### 2. 구현 Implementation    

<br>

###### 1) Structure of codes

- The following files have been modified and added during this assignment: PRJ02_syscall.c, pmanager.c, proc.c, proc.h.
  - Since getadmin, setmemorylimit, and printproclist require access to the process table. They are defined and implemented in proc.c.
  - In PRJ02_syscall.c, exec2 function and wrapper function of all newly implemented system calls are implemented.
  - To implement the required function, two variables have been added to the struct proc existing in proc.h.
  - pmanager.c has been added to implement pmanager.

<br>

###### 2) Process initialization

Since I declared new variables in the process structure, I added a task to initialize new variables in the allocproc() function.

```c
// proc.c

static struct proc*
allocproc(void)
{
  // (...)
found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  // (...)

  // For Project #2
  p->mode = USER;
  p->limit = 0;
    
  // (...)
```

<br>

###### 3) Administrator mode

The getadmin function is implemented as follows. I used strcmp() in ulib.c, and through this, the password entered can be compared with the correct password, my student number.

As mentioned earlier, since a variable representing the execution mode was added to the process structure, the code could be easily written as follows.

```c
// proc.c

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
```

<br>

###### 4) Custom stack size

The implementation of exec2() started based on the exec() function existing in exec.c. In order to comply with the provisions in the given specification, an additional exception handling sequence was implemented in front of the function.

In the case of the normal exec() function in xv6, exec() allocates two pages for every process. However, in the case of exec2(), n stack pages must be allocated. Hence, the code was modified as follows.

```c
// PRJ02_syscall.c

int
exec2(char *path, char **argv, int stacksize)
{
  // (...)

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

  // (...)

  // Allocate (1+stacksize) pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + (1+stacksize)*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - (1+stacksize)*PGSIZE));
  sp = sz;

  // (...)
```

<br>

###### 5) Per-process memory limit

In the case of the setmemorylimit function, the variable representing the memory limit is declared in the struct proc, so it can be easily implemented as follows.

```c
// proc.c

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
```

<br>

The key to the implementation of this feature is to limit the memory of the process. Therefore, I traced up the process of allocating new memory to the process. The sequence that I found is as follows: malloc -> morecore -> sbrk -> growproc -> allocuvm.

Thus, memory allocation could be limited by checking an additional condition in the growproc() function.

```c
// proc.c

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
```

<br>

###### 6) Process manager

Implementing the list command in pmanager was solved with one system call below. At first, I considered getting the entire process information by a system call, and then print that data in the pmanager. However, it was very inefficient. Hence, I made a system call that prints all of the process data within that system call.

```c
// proc.c

void
printproclist(void)
{
  struct proc *p;
  char *pstate;

  acquire(&ptable.lock);
  cprintf("NAME | PID | State | Elapsed Time (ticks) | Memory Size (bytes) | Limit (bytes)\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != UNUSED){
      pstate = getprocstate(p->state);
      cprintf("%s | %d | %s | %d | %d | %d\n", p->name, p->pid, pstate, ticks - p->time_c, p->sz, p->limit);
    }
  }
  cprintf("\n");
  release(&ptable.lock);
}
```

<br>

For background execution, I referred to the sh.c's BACK command processing method. In the case of the BACK command, unlike the EXEC command, when it exec() the process, the parent process is continuously executed without doing wait(). Therefore, when I made the execute command, I also executed exec2() without wait(). After implementation like this, when exiting, I implemented wait() function n times (n = # of child process that pmanager generated.)



```c
int
runcmd(struct cmd *cmd)
{
  // (...)

  case EXIT:
    // if it gets exit commend, it should wait all the child process to terminate.
    // store the child process pid and kill all of them.
    
    for(int i = 0; i <= child.head; i++){
      kill(child.pid[i]);
      wait();
    }
    
  // (...)
}
```

<br>

Command parsing in pmanager was solved by implementing the following functions. Cut the sentence input from parseline() through strtok1() and put it as char *arg implemented in parsecmd() function. parsecmd() creates and returns each command structure using the parsed result obtained above.

The important thing here is that if a command receives a number such as kill or memlim, it is necessary to check whether it is integer. In the case of the provided atoi() function, the input such as '19a' is also converted to integer '19' and returned, so it is not good for accurate exception handling.

```c
char* strtok1(char *str, const char* delim) {
  static char* _buffer;

  if(str != 0) 
  	_buffer = str;
  
  if(_buffer[0] == '\0') 
  	return 0;
 
  char *ret = _buffer, *b;
  const char *d;
 
  for(b = _buffer; *b !='\0'; b++) {
    for(d = delim; *d != '\0'; d++) {
      if(*b == *d) {
        *b = '\0';
        _buffer = b+1;
 
        // skip the beginning delimiters
        if(b == ret) { 
          ret++; 
          continue; 
        }

        return ret;
      }
    }
  }
 
  return ret;
}

void
parseline(char *s, char **arg)
{
  char *ptr = strtok1(s, " ");
  for(int i = 0; i < MAXARGS + 1 && ptr != 0; i++){
    arg[i] = ptr;
    ptr = strtok1(0, " ");
  }
}

struct cmd*
parsecmd(char *s)
{
  struct cmd* result = 0;
  //int type, pid;
  //uint limit;

  char *arg[MAXARGS + 1] = {0, };

  // here you have to parse the command!

  s[strlen(s) - 1] = ' ';
  parseline(s, arg);

  if(strcmp(arg[0], "list") == 0) {
    if(arg[1] != 0)
      return 0;

    result = cmd(LIST);
  }

  else if(strcmp(arg[0], "kill") == 0){
    if(arg[2] != 0)
      return 0;

    for(int i = 0; i < strlen(arg[1]); i++){
      if(arg[1][i] < 48 || arg[1][i] > 57)
        return 0;
    }

    int pid = atoi(arg[1]);
    result = killcmd(pid);
  }

  else if(strcmp(arg[0], "execute") == 0){
    if(arg[3] != 0)
      return 0;

    for(int i = 0; i < strlen(arg[2]); i++){
      if(arg[2][i] < 48 || arg[2][i] > 57)
        return 0;
    } 

    int stacksize = atoi(arg[2]);
    result = execcmd(arg[1], stacksize);

    // do something
  }

  else if(strcmp(arg[0], "memlim") == 0){
    if(arg[3] != 0)
      return 0;

    for(int i = 0; i < strlen(arg[1]); i++){
      if(arg[1][i] < 48 || arg[1][i] > 57)
        return 0;
    }

    for(int i = 0; i < strlen(arg[2]); i++){
      if(arg[2][i] < 48 || arg[2][i] > 57)
        return 0;
    } 

    int pid = atoi(arg[1]);
    uint limit = (uint)atoi(arg[2]);
    result = mlimcmd(pid, limit);
  }

  else if(strcmp(arg[0], "exit") == 0){
    if(arg[1] != 0)
      return 0;

    result = cmd(EXIT);
  }

  return (struct cmd*) result;
}
```

<br>

#### 3. 실행결과 Test Results

<br>

###### ※ Test Environment

Ubuntu 16.04.4 on  VMware Workstation 15.5.2

- Allocated # of cores: 2

- Allocated memory: 2GB

<br>

|       | admin_test                                                   | memory_test                                                  |
| ----- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| Video | ![PRJ02_admin_test](/uploads/7c9f4e28b611b32fb85c871f3ddc9d81/PRJ02_admin_test.mp4) | ![PRJ02_memory_test](/uploads/8537ec38a5cf89b462921efc6027918c/PRJ02_memory_test.mp4) |

|       | stack_test                                                   | pmanager_wrong_input                                         |
| ----- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| Video | ![PRJ02_stack_test](/uploads/49cf26e1ab608c5bf156cc65da44e74a/PRJ02_stack_test.mp4) | ![PRJ02_pmanager_wrong_input](/uploads/e4a1c3753ed69240c9b2554c5c61640e/PRJ02_pmanager_wrong_input.mp4) |

From these testings, I could conclude that the functions that I've made works as the given specification. 

<br>

#### 4. 트러블슈팅 Trouble Shooting

<br>

- Multiple zombie processes occurred when exiting the pmanager, causing multiple 'zombie!' messages as output.
  - This error was caused by not wait() child processes when pmanager was closed. Therefore, it could be solved by wait() on exit as much as the spawned child process.

<br>

#### 5. Acknowledgement

<br>

- I couldn't implement getshmem() function. Thus, it is designed to exit() when called.

- I referred the link below to implement the strtok1() function used in pmanager.
  - Original credit to Feng Li
  - Link: https://fengl.org/2013/01/01/implement-strtok-in-c-2/

