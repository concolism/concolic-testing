#include <klee/klee.h>

#define MEM_LENGTH 5
#define STK_LENGTH 10
#define PRG_LENGTH 10

enum Tag { L, H };
typedef enum Tag Tag;

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
  MemAtom memory[MEM_LENGTH];
  StkAtom stack[STK_LENGTH];
  Insn insns[PRG_LENGTH];
};
typedef struct Machine Machine

int main() {
  
  return 0;
}
