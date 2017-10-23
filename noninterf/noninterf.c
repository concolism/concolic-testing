#include <klee/klee.h>

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

int main() {
  Machine machine;
  MemAtom memory[MEM_LENGTH];
  StkAtom stack[STK_LENGTH];
  Insn insns[PRG_LENGTH];

  klee_make_symbolic(&machine, sizeof machine, "machine");
  klee_make_symbolic(&memory, sizeof memory, "memory");
  klee_make_symbolic(&stack, sizeof stack, "stack");
  klee_make_symbolic(&insns, sizeof insns, "insns");

  klee_assume(machine.pc >= 0);
  klee_assume(machine.sp >= 0);
  klee_assume(machine.memory == memory);
  klee_assume(machine.stack == stack);
  klee_assume(machine.insns == insns);


  for (int i = 0 ; i < machine.sp && i < STK_LENGTH ; i++) {
    klee_assume(machine.stack[i].value >= 0);
  }

  for (int i = 0 ; i < MEM_LENGTH ; i++) {
    klee_assume(machine.memory[i].value >= 0);
  }

  for (int i = 0 ; i < PRG_LENGTH ; i++) {
    klee_assume(machine.insns[i].immediate.value >= 0);
  }

  step(&machine);
  return 0;
}
