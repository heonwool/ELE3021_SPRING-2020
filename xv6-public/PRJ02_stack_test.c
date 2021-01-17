#include "types.h"
#include "stat.h"
#include "user.h"

void f(int depth, int pid)
{
  volatile char arr[1000] = {0};
  sleep(100);
  printf(1, "[PID %d]Recursion depth: %d\n", pid, depth + arr[0]);
  f(depth + 1, pid);
}

int main(int argc, char *argv[])
{
  sleep(1000);


  fork();
  int pid = getpid();

  f(1, pid);
  exit();
  return 0;
}

