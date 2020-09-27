#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char blob[4096] = {0};

static void scan(void) {
  for (int i = 0; i < 4096; ++i)
    if (blob[i] != 0) {
      printf("Found \"unexpected\" bit-flip at 0x%p\n", &blob[i]);
      exit(EXIT_SUCCESS);
    }
}

int main(void) {
  unsigned count = 0;
  for (;;) {
    printf("%u -- pid: %d (Try running: `cme -p %d -n 10`)\n", ++count,
           getpid(), getpid());
    scan();
    sleep(1);
  }
  return 0;
}
