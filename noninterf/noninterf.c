#include <klee/klee.h>
#include <limits.h>
#include <stdlib.h>

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
void print_int_pair(int x, int y) {
  if (x == y) {
    printf("%d", x);
  } else {
    printf("[%d/%d]", x, y);
  }
}

char tag2char(Tag t) {
  return t == L ? 'L' : 'H';
}

void print_tag_pair(Tag t, Tag u) {
  if (t == u) {
    printf("%c", tag2char(t));
  } else {
    printf("[%c/%c]", tag2char(t), tag2char(u));
  }
}

void print_atom_pair(Atom a, Atom b) {
  print_int_pair(a.value, b.value);
  print_tag_pair(a.tag, b.tag);
}

const char *insn_type(InsnType it) {
  switch (it) {
    case NOOP:
      return "NOOP";
    case PUSH:
      return "PUSH";
    case POP:
      return "POP";
    case LOAD:
      return "LOAD";
    case STORE:
      return "STORE";
    case ADD:
      return "ADD";
    case HALT:
      return "HALT";
    default:
      return "UNK";
  }
}

void print_atom(Atom a) {
  print_atom_pair(a, a);
}

void print_insn(Insn i) {
  printf("%s", insn_type(i.t));
  if (i.t == PUSH) {
    printf("(");
    print_atom(i.immediate);
    printf(")");
  }
}

void print_insn_pair(Insn i, Insn j) {
  if (i.t == j.t) {
    printf("%s", insn_type(i.t));
    if (i.t == PUSH) {
      printf("(");
      print_atom_pair(i.immediate, j.immediate);
      printf(")");
    }
  } else {
    printf("[");
    print_insn(i);
    printf("/");
    print_insn(j);
    printf("]");
  }
}

void print_machine_pair(Machine *m1, Machine *m2) {
  printf("PC: ");
  print_int_pair(m1->pc, m2->pc);
  printf("\tSP: ");
  print_int_pair(m1->sp, m2->sp);
  printf("\nSTK: ");
  int max_sp = m1->sp < m2->sp ? m2->sp : m1->sp;
  for (int i = 0; i < max_sp; i++) {
    if (i >= m1->sp) {
      printf("_/");
      print_atom(m2->stack[i]);
    } else if (i >= m2->sp) {
      print_atom(m1->stack[i]);
      printf("/_");
    } else {
      print_atom_pair(m1->stack[i], m2->stack[i]);
    }
    if (i < max_sp-1)
      printf("\t");
  }
  printf("\nMEM: ");
  for (int i = 0; i < MEM_LENGTH; i++) {
    print_atom_pair(m1->memory[i], m2->memory[i]);
    if (i < MEM_LENGTH-1)
      printf("\t");
  }
  printf("\nPRG: ");
  for (int i = 0; i < PRG_LENGTH; i++) {
    print_insn_pair(m1->insns[i], m2->insns[i]);
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
    //ASSERT(m1->pc == m2->pc);
    //ASSERT(m1->sp == m2->sp);
    // for (int i = 0; i < m1->sp; i++) {
    //   ASSERT(indist_atom(m1->stack[i], m2->stack[i]));
    // }
    for (int i = 0; i < MEM_LENGTH; i++) {
      ASSERT(indist_atom(m1->memory[i], m2->memory[i]));
    }
    // for (int i = 0; i < PRG_LENGTH; i++) {
    //   ASSERT(m1->insns[i].t == m2->insns[i].t);
    //   if (m1->insns[i].t == PUSH)
    //     ASSERT(indist_atom(m1->insns[i].immediate, m2->insns[i].immediate));
    // }
    return 1;
}

void assume_indist_atom(Atom a, Atom b) {
  klee_assume(a.tag == b.tag);
  if (a.tag == L)
    klee_assume(a.value == b.value);
}

void assume_indist_insn(Insn i1, Insn i2) {
  klee_assume(i1.t == i2.t);
  assume_indist_atom(i1.immediate, i2.immediate);
}

void assume_indist_machine(Machine *m1, Machine *m2) {
    klee_assume(m1->pc == m2->pc);
    klee_assume(m1->sp == m2->sp);
    for (int i = 0; i < m1->sp; i++) {
      assume_indist_atom(m1->stack[i], m2->stack[i]);
    }
    for (int i = 0; i < MEM_LENGTH; i++) {
      assume_indist_atom(m1->memory[i], m2->memory[i]);
    }
    for (int i = 0; i < PRG_LENGTH; i++) {
      assume_indist_insn(m1->insns[i], m2->insns[i]);
    }
}

void assume_valid_machine(Machine *machine) {
  // Why can't we use &&
  klee_assume(0 == machine->pc);
  //klee_assume(machine->pc < PRG_LENGTH);
  klee_assume(0 == machine->sp);
  //klee_assume(machine->sp < STK_LENGTH);

  for (int i = 0 ; i < machine->sp && i < STK_LENGTH ; i++) {
    klee_assume(machine->stack[i].value == 0);
    //klee_prefer_cex(machine->stack, machine->stack[i].value == 0);
    klee_assume(machine->stack[i].tag == 0);
  }

  for (int i = 0 ; i < MEM_LENGTH ; i++) {
    klee_assume(machine->memory[i].value == 0);
    //klee_prefer_cex(machine->memory, machine->memory[i].value == 0);
    klee_assume(machine->memory[i].tag == L);
  }

  for (int i = 0 ; i < PRG_LENGTH ; i++) {
    klee_assume(machine->insns[i].immediate.value >= 0);
    //klee_prefer_cex(machine->insns, machine->insns[i].t == NOOP);
    klee_assume(machine->insns[i].immediate.value < MEM_LENGTH);
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

#ifdef REPLAY_MANUAL
void read_machine(Machine* to) {
  to->pc = 0;
  to->sp = 0;
  for (int i = 0; i < MEM_LENGTH; i++) {
    to->memory[i].tag = L;
    to->memory[i].value = 0;
  }
  char src_[80];
  char *src = fgets(src_, 80, stdin);
  for (int i = 0; i < PRG_LENGTH; i++) {
    Insn *next_insn = &to->insns[i];
    next_insn->immediate.value = 0;
    next_insn->immediate.tag = L;
    switch (*src++) {
      case 'P':
        next_insn->t = PUSH;
        next_insn->immediate.value = (int) (*src++ - '0');
        next_insn->immediate.tag = (*src++ == 'l') ? L : H;
        break;
      case 'Q':
        next_insn->t = POP;
        break;
      case 'L':
        next_insn->t = LOAD;
        break;
      case 'S':
        next_insn->t = STORE;
        break;
      case 'A':
        next_insn->t = ADD;
        break;
      case 'H':
        next_insn->t = HALT;
        break;
      case 'O':
        next_insn->t = NOOP;
        break;
      default:
        do {
          to->insns[i].t = NOOP;
          to->insns[i].immediate.value = 0;
          to->insns[i].immediate.tag = L;
        } while (++i < PRG_LENGTH);
        return;
    }
  }
}
#endif

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

#if defined(REPLAY) || defined(COVERAGE)
  machine1.memory = memory1;
  machine1.stack = stack1;
  machine1.insns = insns1;

  machine2.memory = memory2;
  machine2.stack = stack2;
  machine2.insns = insns2;
#endif

#ifdef REPLAY_MANUAL
  read_machine(&machine1);
  read_machine(&machine2);
#endif

#ifdef REPLAY
  printf("*** Initial\n");
  print_machine_pair(&machine1, &machine2);
#endif

  //klee_assume(machine1.pc == 0);
  klee_assume(machine1.memory == memory1);
  klee_assume(machine1.stack == stack1);
  klee_assume(machine1.insns == insns1);

  klee_assume(machine2.memory == memory2);
  klee_assume(machine2.stack == stack2);
  klee_assume(machine2.insns == insns2);

  klee_assume(machine1.insns[0].t == PUSH);
  // klee_assume(machine1.insns[0].immediate.value == 1);
  // klee_assume(machine1.insns[0].immediate.tag == L);
  klee_assume(machine1.insns[1].t == PUSH);
  // klee_assume(machine1.insns[1].immediate.value == 0);
  // klee_assume(machine1.insns[1].immediate.tag == H);
  klee_assume(machine1.insns[2].t == STORE);
  klee_assume(machine1.insns[3].t == HALT);

  assume_valid_machine(&machine1);
  // assume_valid_machine(&machine2);
  // assume_indist_machine(&machine1, &machine2);

  machine1_.memory = memory1_;
  machine1_.stack = stack1_;
  machine1_.insns = insns1;
  copy_machine(&machine1, &machine1_);

  // machine2_.memory = memory2_;
  // machine2_.stack = stack2_;
  // machine2_.insns = insns2;

  if (run(&machine1) == ERRORED) {
#ifdef REPLAY
    printf("Machine 1 error\n");
#endif
    exit(1);
  }

  assume_indist_machine(&machine1_, &machine2);
  assume_valid_machine(&machine2);
  //klee_assume(machine2.insns[1].immediate.value == 1);

  if (run(&machine2) == ERRORED) {
#ifdef REPLAY
    printf("Machine 2 error\n");
#endif
    exit(1);
  }

#ifdef REPLAY
  printf("*** Final\n");
  print_machine_pair(&machine1, &machine2);
#endif
  //klee_stack_trace();
  if (!indist_machine(&machine1, &machine2)) {
    abort();
  }

  return 0;
}
