#include "types.h"
#include "user.h"
#include "fcntl.h"

#define LIST 1 
#define KILL 2 
#define EXEC 3 
#define MLIM 4
#define BACK 5 
#define EXIT 6

#define MAXARGS 3

char *password = "2016024875";

struct {
  int pid[100];
  int head;
} child;

struct cmd {
  int type;
};

struct killcmd {
  int type;
  int pid;
};

struct execcmd {
  int type;
  char *argv[10];
  int stacksize;
};

struct mlimcmd {
  int type;
  int pid;
  uint limit;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

int
runcmd(struct cmd *cmd)
{
  struct killcmd *kcmd = 0;
  struct execcmd *ecmd = 0;
  struct mlimcmd *mcmd = 0;

  switch(cmd->type){
  case LIST:
    printproclist();
    break;

  case KILL:
    kcmd = (struct killcmd*)cmd;
    if(kill(kcmd->pid) == 0 && kcmd->pid > 0)
      printf(1, "Successfully killed PID %d.\n", kcmd->pid);
    else
      printf(1, "Failed to kill PID %d.\n", kcmd->pid);
    break;

  case EXEC:
    ecmd = (struct execcmd*)cmd;

    int ret = fork1();
    if(ret == 0){
      if(getadmin(password) != 0)
        panic("getadmin in execute cmd");
      if(exec2(ecmd->argv[0], ecmd->argv, ecmd->stacksize) == -1)
        printf(2, "Failed to execute '%s'.\n", ecmd->argv[0]);
    }

    return ret;
    break;

  case MLIM:
    mcmd = (struct mlimcmd*)cmd;
    if(setmemorylimit(mcmd->pid, mcmd->limit) == 0)
      printf(1, "Successfully set the memory limit of PID %d.\n", mcmd->pid);
    else
      printf(1, "Failed to set the memory limit of PID %d.\n", mcmd->pid);
    break;

  case EXIT:
    // if it gets exit commend, it should wait all the child process to terminate.
    // store the child process pid and kill all of them.
    
    for(int i = 0; i <= child.head; i++){
      kill(child.pid[i]);
      wait();
    }
    
    exit();
    break;

  default:
    panic("runcmd");
  }

  return 0;
}

int
getcmd(char *buf, int nbuf)
{
  printf(2, "> ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  int fd;

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  if(getadmin(password) != 0){
    close(fd);
    panic("getadmin");
  }

  
  for(int i = 0; i < 100; i++)
    child.pid[i] = 0;
  child.head = -1;
  

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    struct cmd* curcmd;
    curcmd = parsecmd(buf);

    if(curcmd != 0){
      //runcmd(curcmd);
      
      int ret = runcmd(curcmd);
      
      if(ret > 0){
        int flag = 0;
        for(int i = 0; i <= child.head; i++){
          if(child.pid[i] == ret)
            flag = 1;
        }

        if(!flag){
          child.head++;
          child.pid[child.head] = ret;
        }
      }

      // implement systemcall isDead and check whether the child process is alive or not
      // and resize the child.pid
      /*
      printf(1, "CURRENT CHILD STATUS: ");
      for(int i = 0; i <= child.head; i++)
        printf(1, "PID %d ", child.pid[i]);
      printf(1, "\n"); */
      
    }
    else
      printf(1, "Wrong input! Try again!\n");
  }

  exit();
}

void
panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

struct cmd*
cmd(int type)
{
  struct cmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  return (struct cmd*)cmd;
}

struct cmd*
killcmd(int pid)
{
  struct killcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = KILL;
  cmd->pid = pid;
  return (struct cmd*)cmd;
}

struct cmd*
execcmd(char *path, int stacksize)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  cmd->argv[0] = path;
  for(int i = 1; i < 10; i++)
    cmd->argv[i] = 0;

  cmd->stacksize = stacksize;
  return (struct cmd*)cmd;
}

struct cmd*
mlimcmd(int pid, uint limit)
{
  struct mlimcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = MLIM;
  cmd->pid = pid;
  cmd->limit = limit;
  return (struct cmd*)cmd;
}

// Credit: Feng Li
// Origin: https://fengl.org/2013/01/01/implement-strtok-in-c-2/
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
