// Copyright (c) 2008-2009 Bjoern Hoehrmann
// Copyright (c) 2015, Ondrej Palkovsky
// Copyright (c) 2016, Winterland

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <klee/klee.h>


#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

#ifndef ASCII_SOURCE
#ifndef FLOW_MACHINE
static const uint8_t utf8d[] = {
  // The first part of the table maps bytes to character classes that
  // to reduce the size of the transition table and create bitmasks.
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

  // The second part is a transition table that maps a combination
  // of a state of the automaton and a character class to a state.
   0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12,
};
#endif

static inline uint32_t decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
#ifdef FLOW_MACHINE
  // Machine transitions are reflected in the control flow.

  uint32_t type;
  if (byte < 128) {
    type = 0;
  } else if (byte < 144) {
    type = 1;
  } else if (byte < 160) {
    type = 9;
  } else if (byte < 192) {
    type = 7;
  } else if (byte < 194) {
    type = 8;
  } else if (byte < 224) {
    type = 2;
  } else if (byte < 225) {
    type = 10;
  } else if (byte == 237) {
    type = 4;
  } else if (byte < 240) {
    type = 3;
  } else if (byte < 241) {
    type = 11;
  } else if (byte < 244) {
    type = 6;
  } else if (byte < 245) {
    type = 5;
  } else {
    type = 8;
  }

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  type += *state;

  if (type == 0 || type == 25 || type == 31 || type == 33) {
    *state = 0;
  } else if (type == 2
      || type == 37
      || type == 43
      || type == 45
      || type == 55
      || type == 61
      || type == 69) {
    *state = 24;
  } else if (type == 3
      || type == 79
      || type == 81
      || type == 85
      || type == 91
      || type == 93
      || type == 97) {
    *state = 36;
  } else if (type == 4) {
    *state = 60;
  } else if (type == 5) {
    *state = 96;
  } else if (type == 6) {
    *state = 84;
  } else if (type == 10) {
    *state = 48;
  } else if (type == 11) {
    *state = 72;
  } else {
    *state = 12;
  }
#else
  uint32_t type = utf8d[byte];

#if defined(COVERAGE) || defined(SM_COV_1)
  // Hack to see state machine coverage
  switch (type) {
    case 0: break;
    case 1: break;
    case 2: break;
    case 3: break;
    case 4: break;
    case 5: break;
    case 6: break;
    case 7: break;
    case 8: break;
    case 9: break;
    case 10: break;
    case 11: break;
    default: break;
  }
#endif

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state + type];

#if defined(COVERAGE) || defined(SM_COV_2)
  switch (*state) {
    case 0: break;
    case 12: break;
    case 24: break;
    case 36: break;
    case 60: break;
    case 96: break;
    case 84: break;
    default: break;
  }
#endif
#endif

  return *state;
}
#endif

static inline uint16_t decode_hex(uint32_t c)
{
  if (c >= '0' && c <= '9')      return c - '0';
  else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0xFFFF; // Should not happen
}

// Decode, return non-zero value on error
int _js_decode_string(uint16_t *const dest, size_t *destoff,
                  const uint8_t *s, const uint8_t *const srcend)
{
  uint16_t *d = dest + *destoff;
  uint32_t state = 0;
  uint32_t codepoint;

  uint8_t surrogate = 0;
  uint16_t temp_hex = 0;
  uint16_t unidata;

  // Optimized version of dispatch when just an ASCII char is expected
  #define DISPATCH_ASCII(label) {\
    if (s >= srcend) {\
      return -1;\
    }\
    codepoint = *s++;\
    goto label;\
  }

  standard:
    // Test end of stream
    while (s < srcend) {
#ifdef ASCII_SOURCE
        // Assume s is ASCII
        // state remains constant = UTF8_ACCEPT
        codepoint = (uint8_t) *s++;
        if (codepoint >= 0x80) { return -1; }
#else
        if (decode(&state, &codepoint, *s++) != UTF8_ACCEPT) {
          if (state == UTF8_REJECT) { return -1; }
          continue;
        }
#endif

        if (codepoint == '\\')
          DISPATCH_ASCII(backslash)
        else if (codepoint <= 0xffff) {
          *d++ = (uint16_t) codepoint;
        } else {
          *d++ = (uint16_t) (0xD7C0 + (codepoint >> 10));
          *d++ = (uint16_t) (0xDC00 + (codepoint & 0x3FF));
        }
    }
    *destoff = d - dest;
    // Exit point
    return (state != UTF8_ACCEPT);
  backslash:
    switch (codepoint) {
      case '"':
      case '\\':
      case '/':
        *d++ = (uint16_t) codepoint;
        goto standard;
      case 'b':
        *d++ = '\b';
        goto standard;
      case 'f':
        *d++ = '\f';
        goto standard;
      case 'n':
        *d++ = '\n';
        goto standard;
      case 'r':
        *d++ = '\r';
        goto standard;
      case 't':
        *d++ = '\t';
        goto standard;
      case 'u':
        DISPATCH_ASCII(unicode1);
      default:
        return -1;
    }
  unicode1:
    temp_hex = decode_hex(codepoint);
    if (temp_hex == 0xFFFF) { return -1; }
    else unidata = temp_hex << 12;
    DISPATCH_ASCII(unicode2);
  unicode2:
    temp_hex = decode_hex(codepoint);
    if (temp_hex == 0xFFFF) { return -1; }
    else unidata |= temp_hex << 8;
    DISPATCH_ASCII(unicode3);
  unicode3:
    temp_hex = decode_hex(codepoint);
    if (temp_hex == 0xFFFF) { return -1; }
    else unidata |= temp_hex << 4;
    DISPATCH_ASCII(unicode4);
  unicode4:
    temp_hex = decode_hex(codepoint);
    if (temp_hex == 0xFFFF) { return -1; }
    else unidata |= temp_hex;
    *d++ = (uint16_t) unidata;

    if (surrogate) {
      if (unidata < 0xDC00 || unidata > 0xDFFF) // is not low surrogate
        return -1;
      surrogate = 0;
    } else if (unidata >= 0xD800 && unidata <= 0xDBFF ) { // is high surrogate
        surrogate = 1;
        DISPATCH_ASCII(surrogate1);
    } else if (unidata >= 0xDC00 && unidata <= 0xDFFF) { // is low surrogate
        return -1;
    }
    goto standard;
  surrogate1:
    if (codepoint != '\\') { return -1; }
    DISPATCH_ASCII(surrogate2)
  surrogate2:
    if (codepoint != 'u') { return -1; }
    DISPATCH_ASCII(unicode1)
}

#define SIZE 12

#ifdef BUG_DEST_TOO_SMALL
#define DSIZE 2
#elif defined(BUG_DEST_TOO_SMALL_BIS)
#define DSIZE (SIZE-1)
#else
#define DSIZE SIZE
#endif

int main(void) {
  uint8_t s[SIZE];
  uint16_t d[DSIZE];
  size_t ofs = 0;

  klee_make_symbolic(s, sizeof s, "s");
  // klee_assume(s[0] == 'a');
  // klee_assume(s[1] == 'b');
  // klee_assume(s[2] == 'c');

  if (-1 == _js_decode_string(d, &ofs, s, s+SIZE))
    return 1;

  return 0;
}
