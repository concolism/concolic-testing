#!/bin/sh

KLEE=../klee

$KLEE --only-output-states-covering-new unescape_string.bc
