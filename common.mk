CC=clang
GCC=gcc
KLEE=../klee/bin/klee
KLEE_LIB=../klee/lib
CCOPTS=-Wall -I../klee/include
CCBUILDOPTS=-g -c -emit-llvm
GCCCOVOPTS=-fprofile-arcs -ftest-coverage

build: $(TARGET).bc
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
	$(GCC) $(CCOPTS) -L$(KLEE_LIB) -DREPLAY $(BUGS) $< -o $@ -lkleeRuntest

$(TARGET).replay-c: $(ARTIFACT).c
	$(GCC) $(CCOPTS) $(GCCCOVOPTS) -L$(KLEE_LIB) -DCOVERAGE $(BUGS) $< -o $@ -lkleeRuntest

klee: $(TARGET).bc
	$(KLEE) -only-output-states-covering-new $(TIMEOUT_OPT) $<

replay: $(TARGET).replay
	@if [[ -f "$(TEST_FILE)" ]] ; then \
	  LD_LIBRARY_PATH=$(KLEE_LIB) KTEST_FILE=$(TEST_FILE) ./$< ; \
	fi

coverage: $(TARGET).replay-c
	@test $(KLEE_OUT) || (echo "make coverage: KLEE_OUT is undefined" ; exit 1)
	rm -f *.gcda
	for t in $(KLEE_OUT)/*.ktest ; do \
	  LD_LIBRARY_PATH=$(KLEE_LIB) KTEST_FILE=$$t ./$< ; \
	done
	gcov $(TARGET).c

clean:
	rm -f *.bc *.c-prepro *.replay *.replay-c *.gcov *.gcda *.gcno

.PHONY: clean build cpp coverage klee
