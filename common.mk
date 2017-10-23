CC=clang
CCOPTS=-g -c -emit-llvm -I../klee/include
KLEE=../klee/bin/klee

$(TARGET).bc: $(ARTIFACT).c
	$(CC) $(CCOPTS) $(BUGS) $< -o $@

klee: $(ARTIFACT).bc
	$(KLEE) -only-output-states-covering-new --timeout 60 $<

clean:
	rm -f *.bc
