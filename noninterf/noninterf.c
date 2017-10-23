#include <klee/klee.h>

enum Tag { L, H };
enum InsnType {
  NOOP,
  PUSH,
  POP,
  LOAD,
  STORE,
  ADD,
};

struct Atom {
  enum Tag tag;
  int value;
};

struct Insn {
  enum InsnType t;
  int immediate;
};

typedef enum Tag Tag;
typedef enum InsnType InsnType;
typedef struct Atom Atom;
typedef struct Insn Insn;

int main() {
  
  return 0;
}
