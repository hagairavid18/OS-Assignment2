#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  int i;

  if(argc < 3){
    fprintf(2, "usage: kill pid signal...\n");
    exit(1);
  } else if (argc % 2 == 0) {
    fprintf(2, "kill: invalid number of arguments\n");
  }
  for(i=1; i<argc; i = i+2){
    int res = kill(atoi(argv[i]),atoi(argv[i+1]));
    printf("kill %s\n", res == 0? "SUCCESS" : "FAILED");
  }
  exit(0);
}
