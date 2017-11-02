#include <klee/klee.h>
#include <limits.h>

#ifdef REPLAY
#include <stdio.h>
#endif

#define MEM_LENGTH 5
#define STK_LENGTH 10
#define PRG_LENGTH 10

enum Tag { L, H };
typedef enum Tag Tag;

Tag lub(Tag t1, Tag t2) {
  return (t1 == L && t2 == L) ? L : H;
}

enum InsnType {
  NOOP,
  PUSH,
  POP,
  LOAD,
  STORE,
  ADD,
  HALT,
};
typedef enum InsnType InsnType;

struct Atom {
  Tag tag;
  int value;
};
typedef struct Atom Atom;

struct Insn {
  InsnType t;
  Atom immediate;
};
typedef struct Insn Insn;

typedef Atom MemAtom;
typedef Atom StkAtom;

struct Machine {
  int pc;  // program counter
  int sp;  // stack pointer
  MemAtom *memory;
  StkAtom *stack;
  Insn *insns;
};
typedef struct Machine Machine;

#ifdef REPLAY
void print_machine(Machine *machine) {
  printf("PC: %d\tSP: %d\n", machine->pc, machine->sp);
  printf("STK: ");
  for (int i = 0; i < machine->sp; i++) {
    printf("%d%c",
        machine->stack[i].value,
        machine->stack[i].tag == L ? 'L' : 'H');
    if (i < machine->sp-1)
      printf("\t");
  }
  printf("\n");
  printf("MEM: ");
  for (int i = 0; i < MEM_LENGTH; i++) {
    printf("%d%c",
        machine->memory[i].value,
        machine->memory[i].tag == L ? 'L' : 'H');
    if (i < MEM_LENGTH-1)
      printf("\t");
  }
  printf("\n");
  printf("PRG: ");
  for (int i = 0; i < PRG_LENGTH; i++) {
    Atom immediate;
    switch (machine->insns[i].t) {
      case NOOP:
        printf("NOOP");
        break;
      case PUSH:
        immediate = machine->insns[i].immediate;
        printf("PUSH(%d%c)", immediate.value, immediate.tag == L ? 'L' : 'H');
        break;
      case POP:
        printf("POP");
        break;
      case LOAD:
        printf("LOAD");
        break;
      case STORE:
        printf("STORE");
        break;
      case ADD:
        printf("ADD");
        break;
      case HALT:
        printf("HALT");
        break;
    }
    if (i == PRG_LENGTH-1)
      printf("\n");
    else
      printf("\t");
  }
}
#endif

enum Outcome {
  STEPPED,
  ERRORED,
  HALTED,
  EXITED,
};
typedef enum Outcome Outcome;

Outcome step(Machine *machine) {
  if (machine->pc >= PRG_LENGTH)
    return EXITED;

  Insn current_insn = machine->insns[machine->pc];

  switch (current_insn.t) {
    case NOOP:
      break;
    case PUSH:
#ifndef BUG_PUSH_OVERFLOW
      if (machine->sp == STK_LENGTH) {
        return ERRORED;
      }
#endif
      machine->stack[machine->sp++] = current_insn.immediate;
      break;
    case POP:
#ifndef BUG_POP_UNDERFLOW
      if (machine->sp < 1) {
        return ERRORED;
      }
#endif
      machine->sp--;
      break;
    case LOAD:
#ifndef BUG_LOAD_UNDERFLOW
      if (machine->sp < 1) {
        return ERRORED;
      }
#endif
      Atom *addr = &machine->stack[machine->sp-1];
#ifndef BUG_LOAD_OOB
      if (addr->value >= MEM_LENGTH) {
        return ERRORED;
      }
#endif
      *addr = machine->memory[addr->value];
      break;
    case STORE:
#ifndef BUG_STORE_UNDERFLOW
      if (machine->sp < 2) {
        return ERRORED;
      }
#endif
      addr = &machine->stack[--machine->sp];
      Atom data = machine->stack[--machine->sp];
#ifndef BUG_STORE_OOB
      if (addr->value >= MEM_LENGTH) {
        return ERRORED;
      }
#endif
      machine->memory[addr->value] = data;
      break;
    case ADD:
#ifndef BUG_ADD_UNDERFLOW
      if (machine->sp < 2) {
        return ERRORED;
      }
#endif
      Atom data1 = machine->stack[--machine->sp];
      Atom data2 = machine->stack[--machine->sp];
#ifndef BUG_ADD_INT_OVERFLOW
      if (data1.value > INT_MAX - data2.value) {
        return ERRORED;
      }
#endif
#ifdef BUG_ADD_TAG
      Atom data3 = {L, data1.value + data2.value};
#else
      Atom data3 = {lub(data1.tag, data2.tag), data1.value + data2.value};
#endif
      machine->stack[machine->sp++] = data3;
      break;
    case HALT:
      return HALTED;
  }

  machine->pc++;
  return STEPPED;
}

Outcome run(Machine *machine) {
  while (step(machine) == STEPPED) {
    ;
  }
  return step(machine);
}

#define ASSERT(x) if (!(x)) { return 0; }

int indist_atom(Atom a1, Atom a2) {
  ASSERT(a1.tag == a2.tag);
  if (a1.tag == L)
    ASSERT(a1.value == a2.value);
  return 1;
}

int indist_machine(Machine *m1, Machine *m2) {
    ASSERT(m1->pc == m2->pc);
    ASSERT(m1->sp == m2->sp);
    for (int i = 0; i < m1->sp; i++) {
      ASSERT(indist_atom(m1->stack[i], m2->stack[i]));
    }
    for (int i = 0; i < MEM_LENGTH; i++) {
      ASSERT(indist_atom(m1->memory[i], m2->memory[i]));
    }
    for (int i = 0; i < PRG_LENGTH; i++) {
      ASSERT(m1->insns[i].t == m2->insns[i].t);
      if (m1->insns[i].t == PUSH)
        ASSERT(indist_atom(m1->insns[i].immediate, m2->insns[i].immediate));
    }
    return 1;
}

void assume_valid_machine(Machine *machine) {
  // Why can't we use &&
  klee_assume(0 <= machine->pc);
  klee_assume(machine->pc < PRG_LENGTH);
  klee_assume(0 <= machine->sp);
  klee_assume(machine->sp < STK_LENGTH);

  for (int i = 0 ; i < machine->sp && i < STK_LENGTH ; i++) {
    klee_assume(machine->stack[i].value >= 0);
    klee_prefer_cex(machine->stack, machine->stack[i].value == 0);
  }

  for (int i = 0 ; i < MEM_LENGTH ; i++) {
    klee_assume(machine->memory[i].value >= 0);
    klee_prefer_cex(machine->memory, machine->memory[i].value == 0);
  }

  for (int i = 0 ; i < PRG_LENGTH ; i++) {
    klee_assume(machine->insns[i].immediate.value >= 0);
    klee_prefer_cex(machine->insns, machine->insns[i].t == NOOP);
  }
}

void copy_machine(Machine* from, Machine *to) {
  to->pc = from->pc;
  to->sp = from->sp;
  for (int i = 0; i < from->sp; i++) {
    to->stack[i] = from->stack[i];
  }
  for (int i = 0; i < MEM_LENGTH; i++) {
    to->memory[i] = from->memory[i];
  }
}

int main() {
  Machine machine1, machine1_, machine2;
  MemAtom
    memory1[MEM_LENGTH], memory1_[MEM_LENGTH],
    memory2[MEM_LENGTH];
  StkAtom
    stack1[STK_LENGTH], stack1_[STK_LENGTH],
    stack2[STK_LENGTH];
  Insn insns1[PRG_LENGTH], insns2[PRG_LENGTH];

  klee_make_symbolic(&machine1, sizeof machine1, "machine1");
  klee_make_symbolic(&memory1, sizeof memory1, "memory1");
  klee_make_symbolic(&stack1, sizeof stack1, "stack1");
  klee_make_symbolic(&insns1, sizeof insns1, "insns1");

  klee_make_symbolic(&machine2, sizeof machine2, "machine2");
  klee_make_symbolic(&memory2, sizeof memory2, "memory2");
  klee_make_symbolic(&stack2, sizeof stack2, "stack2");
  klee_make_symbolic(&insns2, sizeof insns2, "insns2");

#ifdef REPLAY
  machine1.memory = memory1;
  machine1.stack = stack1;
  machine1.insns = insns1;

  print_machine(&machine1);

  machine2.memory = memory2;
  machine2.stack = stack2;
  machine2.insns = insns2;

  print_machine(&machine2);
#endif

  klee_assume(machine1.pc == 0);
  klee_assume(machine1.memory == memory1);
  klee_assume(machine1.stack == stack1);
  klee_assume(machine1.insns == insns1);

  klee_assume(machine2.memory == memory2);
  klee_assume(machine2.stack == stack2);
  klee_assume(machine2.insns == insns2);

  assume_valid_machine(&machine1);

  machine1_.memory = memory1_;
  machine1_.stack = stack1_;
  machine1_.insns = insns1;
  copy_machine(&machine1, &machine1_);

  // machine2_.memory = memory2_;
  // machine2_.stack = stack2_;
  // machine2_.insns = insns2;

  if (run(&machine1) == ERRORED) {
    klee_silent_exit(0);
  }

  assume_valid_machine(&machine2);
  if (!indist_machine(&machine1_, &machine2)) {
    klee_silent_exit(0);
  }

  if (run(&machine2) == ERRORED) {
    klee_silent_exit(0);
  }

  if (!indist_machine(&machine1, &machine2)) {
    klee_abort();
  }

  return 0;
}
