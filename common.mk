CC=clang
CCOPTS=-g -c -emit-llvm -I../klee/include
KLEE=../klee/bin/klee

$(ARTIFACT).bc: $(ARTIFACT).c
	$(CC) $(CCOPTS) $<

klee: $(ARTIFACT).bc
	$(KLEE) -only-output-states-covering-new --timeout 60 $<
