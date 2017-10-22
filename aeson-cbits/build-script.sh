#!/bin/sh

KLEE_HEADERS=../../klee/include/
BUGS=""
# BUGS+=" -DBUG_DEST_TOO_SMALL"

clang -I$KLEE_HEADERS $BUGS -emit-llvm -c -g unescape_string.c
