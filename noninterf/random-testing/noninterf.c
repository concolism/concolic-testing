//#include <klee/klee.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef REPLAY
#include <stdio.h>
#endif

#define MEM_LENGTH 5
#define STK_LENGTH 10
#ifndef PRG_LENGTH
#define PRG_LENGTH 4
#endif

enum Tag { L, H };
typedef enum Tag Tag;

Tag lub(Tag t1, Tag t2) {
#ifdef BRANCHFREE_TAG
  return (t1 | t2);
#else
  if (t1 == L && t2 == L)
    return L;
  else
    return H;
#endif
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
#ifdef COMPACT
void print_machine_insns(Machine *m) {
  int halt = 0;
  for (int i = 0; i < PRG_LENGTH && !halt; i++) {
    char c;
    switch (m->insns[i].t) {
      case PUSH:
        c = 'P';
        break;
      case POP:
        c = 'Q';
        break;
      case LOAD:
        c = 'L';
        break;
      case STORE:
        c = 'S';
        break;
      case ADD:
        c = 'A';
        break;
      case HALT:
        c = 'H';
        halt = 1;
        break;
      case NOOP:
        c = 'O';
        break;
      default:
        c = 'U';
        halt = 1;
    }
    putchar(c);
  }
  putchar('\n');
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
#ifndef BUG_LOAD_TAG
      Tag t = addr->tag;
#endif
#ifndef BUG_LOAD_OOB
      if (addr->value >= MEM_LENGTH) {
        return ERRORED;
      }
#endif
      *addr = machine->memory[addr->value];
#ifndef BUG_LOAD_TAG
      addr->tag = lub(t, addr->tag);
#endif
      break;
    case STORE:
#ifndef BUG_STORE_UNDERFLOW
      if (machine->sp < 2) {
        return ERRORED;
      }
#endif
      addr = &machine->stack[machine->sp-1];
      Atom data = machine->stack[machine->sp-2];
#ifndef BUG_STORE_OOB
      if (addr->value >= MEM_LENGTH) {
        return ERRORED;
      }
#endif
#ifndef BUG_STORE_TAG
      data.tag = lub(data.tag, addr->tag);
#endif
#ifndef BUG_STORE_TAG_2
      if (addr->tag > machine->memory[addr->value].tag) {
        return ERRORED;
      }
#endif
      machine->memory[addr->value] = data;
      machine->sp -= 2;
      break;
    case ADD:
#ifndef BUG_ADD_UNDERFLOW
      if (machine->sp < 2) {
        return ERRORED;
      }
#endif
      Atom data1 = machine->stack[machine->sp-1];
      Atom data2 = machine->stack[machine->sp-2];
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
      machine->stack[machine->sp-2] = data3;
      machine->sp--;
      break;
    case HALT:
      return HALTED;
    default:
      return ERRORED;
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

#ifdef BRANCHFREE_CHECK
int indist_machine(Machine *m1, Machine *m2) {
  int indist = 1;
  for (int i = 0; i < MEM_LENGTH; i++) {
    indist &= (a1.tag == H) | (a1.value == a2.value);
  }
  return indist;
}
#else
#define ASSERT(x) if (!(x)) { return 0; }

int indist_atom(Atom a1, Atom a2) {
  ASSERT(a1.tag == a2.tag);
#ifdef BRANCHFREE_TAG
  return (a1.tag == H) | (a1.value == a2.value);
#else
  return a1.tag == H || a1.value == a2.value;
#endif
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
#endif

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
printf("Specify Number of Runs:\n");
int number_of_runs,nr;
scanf("%d",&nr);
number_of_runs = nr;
freopen("bugs.txt","w", stdout);
//srand(time(NULL));
while(number_of_runs--){
  Machine machine1, machine1_, machine2,machine2_;
  MemAtom
    memory1[MEM_LENGTH], memory1_[MEM_LENGTH],
    memory2[MEM_LENGTH], memory2_[MEM_LENGTH];
  StkAtom
    stack1[STK_LENGTH], stack1_[STK_LENGTH],
    stack2[STK_LENGTH], stack2_[STK_LENGTH];
  Insn insns1[PRG_LENGTH], insns2[PRG_LENGTH];


for (int i = 0; i < MEM_LENGTH; i++) {
    memory1[i].tag = L;
    memory1[i].value = 0;
    memory2[i].tag = L;
    memory2[i].value = 0;
}

machine1.pc = 0;
machine1.memory = memory1;
machine1.stack = stack1;
machine1.insns = insns1;

machine2.pc = 0;
machine2.memory = memory2;
machine2.stack = stack2;
machine2.insns = insns2;

machine1.sp = rand()%STK_LENGTH;
machine2.sp = machine1.sp;
for (int i = 0; i < machine1.sp;i++){
  int tagid = rand()%2;
  int stkval = rand()%MEM_LENGTH;
  if(tagid){
    stack1[i].tag = L;
    stack2[i].tag = L;
    stack1[i].value = stkval;
    stack2[i].value = stkval;
  }
  else{
    stack1[i].tag = H;
    stack2[i].tag = H;
    stack1[i].value = stkval;
    stack2[i].value = rand()%MEM_LENGTH;
  }
}
for (int i = 0;i < PRG_LENGTH;i++){
  int ins = rand()%7;
  Atom a;
  a.tag = L;
  a.value = 0;
  switch (ins) {
    case 0:
      insns1[i].t = NOOP;
      insns2[i].t = NOOP;
      insns1[i].immediate = a;
      insns2[i].immediate = a;
      break;
    case 1:
      insns1[i].t = PUSH;
      insns2[i].t = PUSH;
      int tagid = rand()%2;
      int pushval = rand()%MEM_LENGTH;
      if(tagid){
        insns1[i].immediate.tag = L;
        insns2[i].immediate.tag = L;
        insns1[i].immediate.value = pushval;
        insns2[i].immediate.value = pushval;
      }
      else{
        insns1[i].immediate.tag = H;
        insns2[i].immediate.tag = H;
        insns1[i].immediate.value = pushval;
        insns2[i].immediate.value = rand()%MEM_LENGTH;
      }
      break;
    case 2:
      insns1[i].t = POP;
      insns2[i].t = POP;
      insns1[i].immediate = a;
      insns2[i].immediate = a;
      break;
    case 3:
      insns1[i].t = LOAD;
      insns2[i].t = LOAD;
      insns1[i].immediate = a;
      insns2[i].immediate = a;
      break;
    case 4:
      insns1[i].t = STORE;
      insns2[i].t = STORE;
      insns1[i].immediate = a;
      insns2[i].immediate = a;
      break;
    case 5:
      insns1[i].t = ADD;
      insns2[i].t = ADD;
      insns1[i].immediate = a;
      insns2[i].immediate = a;
      break;
    case 6:
      insns1[i].t = HALT;
      insns2[i].t = HALT;
      insns1[i].immediate = a;
      insns2[i].immediate = a;
      break;
    default:
      insns1[i].t = NOOP;
      insns2[i].t = NOOP;
      insns1[i].immediate = a;
      insns2[i].immediate = a;
      break;
  }
}

#ifdef REPLAY_MANUAL
  read_machine(&machine1);
  read_machine(&machine2);
#endif

#ifdef REPLAY
#ifndef COMPACT
  printf("*** Initial\n");
  print_machine_pair(&machine1, &machine2);
#else
  print_machine_insns(&machine1);
  return 0;
#endif
#endif

machine1_.memory = memory1_;
machine1_.stack = stack1_;
machine1_.insns = insns1;
copy_machine(&machine1, &machine1_);

machine2_.memory = memory2_;
machine2_.stack = stack2_;
machine2_.insns = insns2;
copy_machine(&machine2, &machine2_);

printf("Run number: %d\n",nr-number_of_runs);
  if (run(&machine1) == ERRORED) {
    printf("Machine 1 error\n\n");
    continue;
}

  if (run(&machine2) == ERRORED) {
    printf("Machine 2 error\n\n");
    continue;
  }

  //klee_stack_trace();
  if (!indist_machine(&machine1, &machine2)) {
    printf("**************BUG***************\n");
  }
  printf("Initial\n");
  print_machine_pair(&machine1_,&machine2_);
  printf("Final\n");
  print_machine_pair(&machine1,&machine2);
  printf("\n");
}
  return 0;
}
