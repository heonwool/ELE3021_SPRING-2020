#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
  char username[50], password[50];
  printf(1, "[Add user]\n");

  printf(1, "Username: ");
  gets(username, sizeof username);
  username[strlen(username) - 1] = 0;
  
  printf(1, "Password: ");
  gets(password, sizeof password);
  password[strlen(password) - 1] = 0;

  if (useradd(username, password) == 0)
    printf(1, "Useradd successful\n");
  else
    printf(1, "Useradd failed!\n");
  exit();
}

/*
int main(int argc, char *argv[])
{
  useradd("user1", "1234");
  useradd("user2", "1234");
  useradd("user3", "1234");
  useradd("user4", "1234");
  useradd("user5", "1234");
  useradd("user6", "1234");
  useradd("user7", "1234");
  useradd("user8", "1234");
  useradd("user9", "1234");

  return 0;
}*/