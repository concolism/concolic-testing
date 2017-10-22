#!/bin/sh

KLEE_HEADERS=../../klee/include/

clang -I$KLEE_HEADERS -emit-llvm -c -g unescape_string.c
