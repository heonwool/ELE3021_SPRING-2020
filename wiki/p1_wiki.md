### Project 1: Simple Schedulers on xv6

<br>

#### 1. 디자인 Design

<br>

##### 1) What is MLQ Scheduling?
In computer science, there is a common division between foreground (interactive) proccesses and background (batch) processes. These two types of processes have different response-time requirements and so may have different scheduling needs. 
Thus, a multilevel queue (MLQ) scheduling algorithm created to meet this needs. It partitions the ready queue several separate queues. The processes are permanently assigned to one queue, generally based on some property of the process, such as memory size, process priority, or process type. Each queue has its own scheduling algorithm. In addition, there must be scheduling among the queues, which is commonly implemented as fixed-priority preemptive scheduling.

<br>
###### 2) What is MLFQ Scheduling?

The multilevel feedback queue (MLFQ) scheduling algorithm is similar to MLQ scheduing algorithm. However, it allows a process to move between queues. The idea is to separate processes according to the characteristics of their CPU bursts. If a process uses too much CPU time, it will be moved to a lower-priority queues. In addition, a process that waits too long in a lower-priority queue may be moved to a higher-priority queue and this prevents starvation problem in MLQ scheduling algorithm.

<br>

###### 3) Project Specification: General

- 모든 preemption 은 10ms(1tick) 안으로 일어나면 된다.
- Coding Convention 은 xv6 코드의 기본적인 형태를 따른다.
- 구현의 편의성을 위해 cpu 의 개수는 하나임을 가정한다.

<br>

###### 4) Project Specification: Required system calls

- void yield(void): 다음 프로세스에게 cpu 를 양보한다.

- int getlev(void): MLFQ 스케줄러일 때 프로세스가 속한 큐의 레벨을 반환한다.

- int setpriority(int pid, int priority):  MLFQ 스케줄러일 때, pid 를 가지는 프로세스의 우선순위를 priority로 설정한다.


<br>

###### 5) Project Specification: MLQ Scheduler

- 스케줄러는 2개의 큐로 구성된다. 
- PID가 짝수인 프로세스들은 RR로, 홀수인 프로세스들은 FCFS로 스케줄링 된다.
- RR queue가 FCFS queue보다 우선순위가 항상 높으며, FCFS queue 내에서는 먼저 생성된 프로세스를 우선적으로 처리해야 한다.
- Sleeping 상태에 있는 프로세스는 무시해야 한다.

<br>
###### 6) Project Specification: MLFQ Scheduler

- K-level feedback queue

- L0 ~ Lk 1 의 큐 k 개로 이루어져 있고 , 수가 작을수록 우선순위가 높은 큐이다.

- i번 큐는 2i+4 ticks의 time quantum을 가진다.

- k는 고정된 값이 아니라, 빌드시에 make에 옵션으로 넣어주는 유동적인 값이다. (k는 2 이상 5 이하의 정수)

- 처음 실행된 프로세스는 모두 가장 높은 레벨의 큐(L0)에 들어간다.

- 같은 큐 내에서는 priority가 높은 프로세스가 먼저 스케줄링 되어야 한다.

- Priorty는 프로세스마다 별개로 존재하며, 0이상 10이하의 정수 값을 가진다. Priority 값이 클수록 우선순위가 높으며, 처음 프로세스가 생성되면 priority는 0이다. 또한 priority가 같은 프로세스끼리는 어느 것을 먼저 스케줄링해도 괜찮다.

- 타이머 인터럽트나 yield, sleep 등을 통해 스케줄러에게 실행 흐름이 넘어올 때마다 , 스케줄러는 RUNNABLE 한 프로세스가 존재하는 큐 중 가장 레벨이 높은 큐를 선택해야 한다.

- 각 큐는 독립적으로 다음을 수행해야 한다: (1) 그 큐 내에서 이전에 마지막으로 실행한 프로세스가 아직 time quantum 이 남아있다면 그 프로세스를 이어서 스케줄링 한다. (2) 그 큐 내에서 이전에 마지막으로 실행한 프로세스가 자신의 time quantum 을 모두 사용했다면 , priority 가 가장 높은 프로세스를 선택하여 스케줄링 한다.

- i < k - 1 일 때 , Li 큐에서 실행된 프로세스가 quantum 을 모두 사용한 경우 Li+1 큐로 내려가고 , 실행 시간을 초기화한다.

- Lk - 1 큐에서 실행된 프로세스가 quantum 을 모두 사용한 경우 해당 프로세스는 이후 priority boosting 이 되기 전까지 다시 스케줄링되지 않는다.

- yield 나 sleep 시스템 콜을 사용한 경우 해당 프로세스는 작업을 마친 것으로 간주하여 L0 큐로 올라가며 실행 시간도 초기화된다.

- 100 ticks 가 지날 때마다 모든 프로세스들을 L0 큐로 올리는 priority boosting 을 구현해야 한다. 여기서 boost된 프로세스의 실행시간은 초기화되나 priority값은 변하지 않는다.

- 스케줄링될 수 있는 프로세스가 존재하지 않는 경우에도 곧바로 priority boosting 을 수행한다.

<br>

#### 2. 구현 Implementation    

<br>

###### ※ Main Idea

<br>

The main idea for this project was not to make actual queue structure. I though that if I create an actual queue structure in this project, it will increase the development difficulty since there would be much more things to consider.

<br>

###### 1) General Implementations

<br>
To diverge the source code in compile time, I used *gcc CFLAG options* and *#ifdef~#endif* in source code. In addition, to check the current CPU scheduling algorithm, I added *cprintf()* in *init.c*.

```c
// init.c (in main() function)

  #ifdef DEFAULT
  printf(1, "policy: DEFAULT\n");
  #elif MULTILEVEL_SCHED
  printf(1, "policy: MULTILEVEL_SCHED\n");
  #elif MLFQ_SCHED
  printf(1, "policy: MLFQ_SCHED\n");
  printf(1, "MLFQ_K: %d", MLFQ_K);
  #else
  printf(1, "policy: Unknown\n");
  #endif
```

<br>
To implement MLQ and MLFQ Schedulers, I revised the *struct proc* in *proc.h*. 

```c
// proc.h

struct proc {
  // (...)

  // For MLQ and MLFQ schedulers
  uint time_c;                 // Process creation time
  int priority;                // Process priority
  int level;                   // Queue level of the process
  int hold;                    // # of times that process can hold CPU
};
```

<br>

###### 2) System calls

<br>

System calls for this project are defined in *proc.c* and *PRJ01_syscall.c*.

In this project, I implemented three system calls to meet the project specification. Among the newly written system calls, the most important thing is the *yield2()* function.

The yield () system call is a function that can be called by both the kernel and the user. Therefore, in the MLFQ scheduler, it is necessary to distinguish between the *yield()* function called by the user and the *yield()* function called by the kernel.

If we do not divide these two, whenever kernal call the *yield()*, it will reset the process level and hold value into 0 and 4. This cause the malfunction of *scheduler()* function.

Thus, I implemented new function *yield2()* to divide user-called yield function and kernel-called yield function. Although *yield2()* function does the same job as *yield()*, *yield2()* additionally does the following: reset the process *level* and *hold* value into 0 and 4.

By writing the code shown below, I was able to satisfy the project specifications.

```c
// proc.c

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
```

```c
// PRJ01_syscall.c
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
```

<br>

###### 3) MLQ Scheduler

<br>
To implement the MLQ scheduler, I mainly focused on *proc.c* and *proc.h*. And, the main idea was to create *scheduer()* function without the actual queue itself to reduce the difficulty.
For FCFS scheduling scheme, I added variable *time_c* at *struct proc*. By setting up the process creation time in *allocproc()* function, we can compare the process creation time between the odd pid proceeses.

```c
// proc.c
// in allocproc() function

found:
  // (...)
  #ifdef MULTILEVEL_SCHED
  p->time_c = ticks;
  // (...)
```

<br>
Inside the scheduler function, I used multiple for loops for traversing *ptable.proc*. By this approach, I can get same effect without creating actual queues for odd and even process pids.

First, traverse *ptable.proc* to see if there is a process that has even pid. If present, process the scheduling by Round Robin algorithm. 

```c
   // proc.c
   // in scheduler() function
   
   // (...)
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
```

After the end of traversal, check *ptable.proc* one more time whether there are processes that has even pid and the state is *RUNNABLE*.

If exists, goto *EXIST_RUNNABLE* and deal with it.

```c
   // proc.c
   // in scheduler() function
   
   // (...)
       for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
         if(p->state == RUNNABLE && p->pid % 2 == 0)
           goto EXIST_RUNNABLE;
       }
   // (...)
```

If not, we need to schedule processes that has odd pid.

In odd pid queue, we need to follow FCFS algorithm. Thus, an additional *ptable.proc* traversal is needed to find out the first-come process.

If there is an even pid process that is *RUNNABLE* during the additional *ptable.proc* traversal, goto *EXIST_RUNNABLE* to deal with it first.

```c
   // proc.c
   // in scheduler() function
   
   // (...)
       for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
         if(p->state != RUNNABLE)
           continue;
   
         struct proc *temp_p;
         for(temp_p = ptable.proc; temp_p < &ptable.proc[NPROC]; temp_p++){
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
   // (...)
```

<br>

###### 4) MLFQ Scheduler

<br>

To implement the MLFQ scheduler, I mainly focused on *proc.c* and *proc.h*. And, the main idea was to create *scheduer()* function without the actual queue itself to reduce the difficulty.
For MFLQ scheduling scheme, I added variable *priority*, *level*, and *hold* at *struct proc* and initialize these values in allocproc() function.

```c
// proc.c
// in allocproc() function

found:
  // (...)
  #elif MLFQ_SCHED
  p->priority = MIN_PRIORITY;
  p->level = 0;
  p->hold = TIME_QUANTUM[0];
  #endif
  // (...)
```

<br>

Inside the *scheduler()* function, I followed these steps to create the MLFQ scheduling algorithm.

1. Boost the priority every 100 ticks
2. If there is a process who used all of ther time quantum, move to the next queue. Also, if there is a *SLEEPING* process, put it in L0 queue.
3. Check the queue level that has *RUNNABLE* process. If there are no such process that satisfy this condition, boost the priority!
4. *ptable.proc* traversal in the queue level found in step #3.
5. Done!

The result of the steps shown above are:

```c
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
    sti();
    acquire(&ptable.lock);

    // Step 1
    if(ticks % 100 == 0 && flag){
      flag = 0;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        p->level = 0;
        p->hold = 4;
      }
    }

    if(ticks % 100 != 0)
      flag = 1;

    // Step 2
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

    // Step 3
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE && p->hold > 0 && p->level < now_level) {
        now_level = p->level;
      }
    }
    
    if(now_level == MLFQ_K){
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        p->level = 0;
        p->hold = 4;
      }
      release(&ptable.lock);
      continue;
    }

    // Step 4
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
```

<br>

#### 3. 실행결과 Test Results

<br>

###### ※ Test Environment

Ubuntu 16.04.4 on  VMware Workstation 15.5.2

- Allocated # of cores: 2

- Allocated memory: 2GB

<br>

###### 1) MLQ Scheduler

<br>

```c
// TEST PROGRAM: PRJ01_ml_test.c
// CONSTANT VALUE USED IN THE TEST

#define NUM_LOOP 20
#define NUM_YIELD 50000
#define NUM_SLEEP 10

#define NUM_THREAD 8
```
![PRJ01_mlq](/uploads/dca7de8dad3febd80460adcdab3a055f/PRJ01_mlq.mp4)
[PRJ01_mlq_result.txt](/uploads/c7e4cf5597a6edbe23e757cdeb2e8c84/PRJ01_mlq_result.txt)

<br>
In *Test 1*, 8 processes print out itself without doing sleep() or yield(). As shown in the result, processes with even pid comes first. Whereas processes with even pid change their turn, processes with odd pid comes in ascending order.

In *Test 2*, each process keeps doing *yield()*. Processes with even pid are Round Robin, so they run alternately each time when yield() occurs. Thus, they end almost simultaneously. However, processes with odd pid are FCFS, so they finish their job in ascending order.

In *Test 3*, each process keeps doing *sleep()*. In the result, we can see processes with even pid run first, and after that, processes with odd pid run. Processes with even pid do change their order, however, processes with odd pid does not change their order.

By this test results, I could confirm that the MLQ scheduler that I made satisfy the given project specifications.

<br>

###### 2) MLFQ Scheduler

<br>

```c
// TEST PROGRAM: PRJ01_mlfq_test.c
// CONSTANT VALUE USED IN THE TEST

#define NUM_LOOP 100000
#define NUM_YIELD 20000
#define NUM_SLEEP 500

#define NUM_THREAD 4
#define MAX_LEVEL 5
```

| MLFQ_K = 2 | MLFQ_K = 5 |
| ---------- | ---------- |
| (결과)     | (결과)     |

In *Test 1*, each process counts its queue level (0 ~ k-1). Since there is no priority between processes, it ends almost simultaniously. Thus, the result of *Test 1* satisfy the project specifications.

In *Test 2*, although the overall usage of times of each process eventually ends up being similar, a process with a large pid is more likely to end a little earlier. Hence, the result of *Test 2* satisfy the project specifications.

In the project specification, calling the system call yield() and sleep() resets the level and usage time. 

Thus, every process in *Test 3* will stay at L0 queue which will finish the process in descending order. Hence, the result of *Test 3* satisfy the project specifications.

In addition, every process in *Test 4* will also stay at L0 queue. However, if a process is in the sleep state, it will not be scheduled which allows other processes can be executed. Assuming the results of *Test 4* according to the project specifications, all processes must be completed simultaneously. Thus, we can say that the result of *Test 4* satisfy the project since it ends simultaneously.

In *Test 5*, processes with smaller pids that want to stay at a higher level are scheduled more preferentially, so finish the job first. Hence, the result of *Test 5* satisfy the expected result.

In *Test 6*, there weren't a message that starts with 'wrong:~'.

By this test results, I could confirm that the MLFQ scheduler that I made satisfy the given project specifications.

<br>

#### 4. 트러블슈팅 Trouble Shooting

<br>

###### 1) General Problems

<br>

- Cannot define SCHED_POLICY value properly in Makefile

   The CFLAG -D option has the same effect as declaring #define in the source code to be compiled. I had a problem writing the Makefile as shown below.

   ```makefile
   # Wrong Makefile
   CFLAG = (...) -D SCHED_POLICY=$(SCHED_POLICY)
   ```

   Declaring as above has the same effect as writing the code in the .c file as shown below.

   ```
   #define SCHED_POLICY SCHED_POLICY
   ```

   Thus, by revising the Makefile shown below, I can solve this issue.

   ```makefile
   # Revised Makefile
   CFLAG = (...) -D $(SCHED_POLICY)
   ```

   <br>

###### 2) MLQ Scheduler

<br>

- Cannot pass the TEST2 in ml_test.c

   During the project, I implemented that the processes with even pid execute first and the rest execute next. However, the result of [Test 2] was weird: Processes with even pid did not ended simultaneously. 

   The main reason for the problem was that FCFS was applied equally to processes with even pid. Thus, I resolved this problem by dividing the source code and creating an additional routine to check whether the process wthi even pid existed or not.

<br>

###### 3) MLFQ Scheduler

- Duplicated priority boosting problem when boosting the priority every 100 ticks.

At first, I did priority boosting for every 100 ticks by using the following pseudo code.

```c
   if(ticks % 100 ==0 ){
     // DO priority_boosting;
     continue;
   }
```

   However, *scheduler()* function is executed within 1 tick. Thus, using the above pseudo code can do priority boosting several times in every 100 ticks, which may cause an incorrect scheduling result.

   I can solve this problem by adding one more condition to check.

```c
   // proc.c
   // in scheduler() function
   
   // (...)
     int flag = 1;
     
     for(;;){
       // (..)
       if(ticks % 100 == 0 && flag){
         flag = 0;
         
         for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
           p->level = 0;
           p->hold = 4;
         }
       }
   
       if(ticks % 100 != 0)
         flag = 1;
         
       // (...)
```

   I declared a variable flag that checks whether the priority boosting is executed or not.

   Initialize flag to 1 and for ticks % 2 != 0, reset flag to 1.

   If *ticks % 2 == 0 and flag == 1*, decrease *flag* to 0 and do priority boosting. After the priority boosting, it will not be repeated even if the *scheduler()* is executed at the same ticks because flag is not 1.

   <br>

- Cannot guarantee the atomicity when *priority_boost()* function that I designed is executed in *scheduler()* function.

   At first, I made an additional function that does priority boosting like below and used in *scheduler()* function.
   
   ```c
   void
   priority_boost(void)
   {
     struct proc *p;
   
     acquire(&ptable.lock);
     
     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
       p->level = 0;
       p->hold = 4;
     }
   
     release(&ptable.lock);
   }
   ```
   
   ```c
   // in scheduler() function
   	// (...)
       if(ticks % 100 == 0){
         release(&ptable.lock);
         priority_boost();
         acquire(&ptable.lock);
       }
       // (...)
       if(now_level == MLFQ_K){
         release(&ptable.lock);
         priority_boost();
         continue;
       }
       // (...)
   ```
   
   However, I found that writing code as above does not guarantee atomicity. To execute *priority_boost()* function, the *lock* must be released but when the *lock* is released, a context switch can occur while the *scheduler()* function is being executed.
   
   Thus I removed priority_boost() function and implemented code in *scheduler()* function.