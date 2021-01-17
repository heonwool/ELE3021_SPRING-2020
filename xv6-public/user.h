struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// system calls defined for LAB classes
int myfunction(char*);
int getppid(void);

// system calls defined for Project #1
int yield(void);
int getlev(void);
int setpriority(int pid, int priority);

// system calls defined for Project #2
int getadmin(char *password);
int exec2(char *path, char **argv, int stacksize);
int setmemorylimit(int pid, int limit);
void printproclist(void);
char* getshmem(int pid);

// system calls defined for Project #3
int inituser(void);
int usercheck(char *username, char *password);
int useradd(char *username, char *password);
int userdel(char *username);
int fork2(char *username);
int chmod(char *pathname, int mode);
int whoami(char *username);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

char* strcat(char*, const char*);
