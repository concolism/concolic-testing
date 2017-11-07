/* Dumb implementation of Klee interface. */

#include <klee/klee.h>
#include <stdio.h>
#include <stdlib.h>

void klee_assume(uintptr_t condition) {
  if (!condition) {
    printf("Invalid assumption.\n");
    exit(1);
  }
}

void klee_prefer_cex(void *object, uintptr_t condition) { }

void klee_make_symbolic(void *object, size_t sz, const char *name) { }

void klee_silent_exit(int status) {
  exit(status);
}

void klee_abort() {
  printf("Abort.\n");
  exit(1);
}
