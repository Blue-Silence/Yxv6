#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char b;
  int a[32];
  for(int i=0;i<32;i++)
    printf("%d\n",a[i]);
  read(0,&b,1);
  return 1;
  /*for(int i=0;i<32;i++)
    printf("%d\n",a[i]);

  //return 123;
  exit(0);*/
}
