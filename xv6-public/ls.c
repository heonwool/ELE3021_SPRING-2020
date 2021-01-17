#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

char*
auth(char *buf, short type, uint mode)
{
  for(int i = 0; i < 7; i++)
    buf[i] = '-';
  buf[7] = '\0';

  switch(type){
  case T_DIR:
    buf[0] = 'd';
    break;
  case T_FILE:
    buf[0] = '-';
    break;
  case T_DEV:
    buf[0] = 'c';
    break;
  default:
    printf(2, "ls: unknown file type\n");
    exit();
  }

  if(mode & MODE_RUSR) buf[1] = 'r';
  if(mode & MODE_WUSR) buf[2] = 'w';
  if(mode & MODE_XUSR) buf[3] = 'x';
  if(mode & MODE_ROTH) buf[4] = 'r';
  if(mode & MODE_WOTH) buf[5] = 'w';
  if(mode & MODE_XOTH) buf[6] = 'x';

  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  char res[8];

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "%s %s %s %d %d\n", fmtname(path), st.userid, auth(res, st.type, st.mode), st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %s %s %d %d\n", fmtname(buf), st.userid, auth(res, st.type, st.mode), st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit();
}
