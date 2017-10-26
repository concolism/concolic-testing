CC=clang
KLEE=../klee/bin/klee
KLEE_LIB=../klee/lib
CCOPTS=-Wall -I../klee/include
CCBUILDOPTS=-g -c -emit-llvm

build: $(TARGET).bc
replay: $(TARGET).replay
cpp: $(TARGET).c-prepro

ifndef TIMEOUT
TIMEOUT=60
endif

ifndef NOLIMIT
TIMEOUT_OPT:=-max-time $(TIMEOUT)
endif

$(TARGET).bc: $(ARTIFACT).c
	$(CC) $(CCOPTS) $(CCBUILDOPTS) $(BUGS) $< -o $@

$(TARGET).c-prepro: $(ARTIFACT).c
	$(CC) $(CCOPTS) -E $(BUGS) $< -o $@

$(TARGET).replay: $(ARTIFACT).c
	$(CC) $(CCOPTS) -L$(KLEE_LIB) -DREPLAY $(BUGS) $< -o $@ -lkleeRuntest

klee: $(TARGET).bc
	$(KLEE) -only-output-states-covering-new $(TIMEOUT_OPT) $<

clean:
	rm -f *.bc *.c-prepro *.replay

.PHONY: clean build cpp klee
