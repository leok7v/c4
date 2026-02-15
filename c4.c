// c4.c - C in four functions

// char, int, and pointer types
// if, while, return, and expression statements
// just enough features to allow self-compilation and a bit more

// Written by Robert Swierczek

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

char *p, *lp, // current position in source code
     *data;   // data/bss pointer

int64_t *e, *le,  // current position in emitted code
    *id,      // currently parsed identifier
    *sym,     // symbol table (simple list of identifiers)
    *struct_syms, // struct type table (ID -> symbol entry)
    tk,       // current token
    ival,     // current token value
    ty,       // current expression type
    loc,      // local variable offset
    line,     // current line number
    src,      // print source and assembly flag
    debug,    // print executed instructions
    num_structs; // number of defined structs

// tokens and classes (operators last and in precedence order)
enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Int32_t, Int64_t, Return, Sizeof, Struct, Typedef, Union, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak, Arrow
};

// opcodes
enum { LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,LI32,SI  ,SC  ,SI32,PSH ,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,WRIT,SYST,POPN,PCLS,FRED };

// types
enum { CHAR, INT32, INT64, PTR = 256 };

// identifier offsets (since we can't create an ident struct)
enum { Tk, Hash, Name, Class, Type, Val, HClass, HType, HVal, Utyedef, Extent, Sline, Idsz };

void next()
{
  char *pp;

  while ((tk = *p)) {
    ++p;
    if (tk == '\n') {
      if (src) {
        printf("%d: %.*s", (int)line, (int)(p - lp), lp);
        lp = p;
        while (le < e) {
          printf("%8.4s", &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,LI32,SI  ,SC  ,SI32,PSH ,"
                           "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                           "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,WRIT,SYST,POPN,PCLS,FRED,"[*++le * 5]);
          if (*le <= ADJ) printf(" %lld\n", *++le); else printf("\n");
        }
      }
      ++line;
    }
    else if (tk == '#') {
      while (*p != 0 && *p != '\n') ++p;
    }
    else if ((tk >= 'a' && tk <= 'z') || (tk >= 'A' && tk <= 'Z') || tk == '_') {
      pp = p - 1;
      while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')
        tk = tk * 147 + *p++;
      tk = (tk << 6) + (p - pp);
      id = sym;
      while (id[Tk]) {
        if (tk == id[Hash] && !memcmp((char *)id[Name], pp, p - pp)) { tk = id[Tk]; return; }
        id = id + Idsz;
      }
      id[Name] = (int64_t)pp;
      id[Hash] = tk;
      tk = id[Tk] = Id;
      return;
    }
    else if (tk >= '0' && tk <= '9') {
      if ((ival = tk - '0')) { while (*p >= '0' && *p <= '9') ival = ival * 10 + *p++ - '0'; }
      else if (*p == 'x' || *p == 'X') {
        while ((tk = *++p) && ((tk >= '0' && tk <= '9') || (tk >= 'a' && tk <= 'f') || (tk >= 'A' && tk <= 'F')))
          ival = ival * 16 + (tk & 15) + (tk >= 'A' ? 9 : 0);
      }
      else { while (*p >= '0' && *p <= '7') ival = ival * 8 + *p++ - '0'; }
      tk = Num;
      return;
    }
    else if (tk == '/') {
      if (*p == '/') {
        ++p;
        while (*p != 0 && *p != '\n') ++p;
      }
      else {
        tk = Div;
        return;
      }
    }
    else if (tk == '\'' || tk == '"') {
      pp = data;
      while (*p != 0 && *p != tk) {
        if ((ival = *p++) == '\\') {
          if ((ival = *p++) == 'n') ival = '\n';
        }
        if (tk == '"') *data++ = ival;
      }
      ++p;
      if (tk == '"') ival = (int64_t)pp; else tk = Num;
      return;
    }
    else if (tk == '=') { if (*p == '=') { ++p; tk = Eq; } else tk = Assign; return; }
    else if (tk == '+') { if (*p == '+') { ++p; tk = Inc; } else tk = Add; return; }
    else if (tk == '-') { if (*p == '-') { ++p; tk = Dec; } else if (*p == '>') { ++p; tk = Arrow; } else tk = Sub; return; }
    else if (tk == '!') { if (*p == '=') { ++p; tk = Ne; } return; }
    else if (tk == '<') { if (*p == '=') { ++p; tk = Le; } else if (*p == '<') { ++p; tk = Shl; } else tk = Lt; return; }
    else if (tk == '>') { if (*p == '=') { ++p; tk = Ge; } else if (*p == '>') { ++p; tk = Shr; } else tk = Gt; return; }
    else if (tk == '|') { if (*p == '|') { ++p; tk = Lor; } else tk = Or; return; }
    else if (tk == '&') { if (*p == '&') { ++p; tk = Lan; } else tk = And; return; }
    else if (tk == '^') { tk = Xor; return; }
    else if (tk == '%') { tk = Mod; return; }
    else if (tk == '*') { tk = Mul; return; }
    else if (tk == '[') { tk = Brak; return; }
    else if (tk == '?') { tk = Cond; return; }
    else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || tk == ')' || tk == ']' || tk == ',' || tk == ':' || tk == '.') return;
  }
}

void memacc()
{
  int64_t *s, *m, t;
  if (tk != Id) { printf("%d: bad struct member\n", (int)line); exit(-1); }
  s = (int64_t *)struct_syms[ty - INT64 - 1];
  m = (int64_t *)s[Sline];
  t = 0;
  while (m && !t) {
    if (m[0] == id[Hash]) {
      *++e = PSH; *++e = IMM; *++e = m[2]; *++e = ADD;
      ty = m[1];
      if (ty == CHAR) *++e = LC;
      else if (ty == INT32) *++e = LI32;
      else if (ty > INT64 && ty < PTR) ; // nested struct, keep address
      else *++e = LI;
      t = 1;
    }
    else m = (int64_t *)m[3];
  }
  if (!t) { printf("%d: member not found\n", (int)line); exit(-1); }
  next();
}

void expr(int64_t lev)
{
  int64_t t, *d, base_ty, elem_ty;

  if (!tk) { printf("%d: unexpected eof in expression\n", (int)line); exit(-1); }
  else if (tk == Num) { *++e = IMM; *++e = ival; next(); ty = INT64; }
  else if (tk == '"') {
    *++e = IMM; *++e = ival; next();
    while (tk == '"') next();
    data = (char *)((int64_t)data + sizeof(int64_t) & -sizeof(int64_t)); ty = PTR;
  }
  else if (tk == Sizeof) {
    next(); if (tk == '(') next(); else { printf("%d: open paren expected in sizeof\n", (int)line); exit(-1); }
    ty = INT64; 
    if (tk == Int) next(); 
    else if (tk == Int32_t) { next(); ty = INT32; }
    else if (tk == Int64_t) { next(); ty = INT64; }
    else if (tk == Char) { next(); ty = CHAR; }
    while (tk == Mul) { next(); ty = ty + PTR; }
    if (tk == ')') next(); else { printf("%d: close paren expected in sizeof\n", (int)line); exit(-1); }
    *++e = IMM;
    if (ty == CHAR) *++e = sizeof(char);
    else if (ty == INT32) *++e = 4;
    else *++e = sizeof(int64_t); // INT64 and pointers
    ty = INT64;
  }
  else if (tk == Id) {
    d = id; next();
    if (tk == '(') {
      next();
      t = 0;
      while (tk != ')') { expr(Assign); *++e = PSH; ++t; if (tk == ',') next(); }
      next();
      if (d[Class] == Sys) *++e = d[Val];
      else if (d[Class] == Fun) { *++e = JSR; *++e = d[Val]; }
      else { printf("%d: bad function call\n", (int)line); exit(-1); }
      if (t) { *++e = ADJ; *++e = t; }
      ty = d[Type];
    }
    else if (d[Class] == Num) { *++e = IMM; *++e = d[Val]; ty = INT64; }
    else {
      if (d[Class] == Loc) { *++e = LEA; *++e = loc - d[Val]; }
      else if (d[Class] == Glo) { *++e = IMM; *++e = d[Val]; }
      else { printf("%d: undefined variable\n", (int)line); exit(-1); }
      ty = d[Type];
      
      if (ty == CHAR) *++e = LC;
      else if (ty == INT32) *++e = LI32;
      else if (ty > INT64 && ty < PTR) ; // struct value, keep address
      else *++e = LI;
    }
  }
  else if (tk == '(') {
    next();
    if (tk == Int || tk == Int32_t || tk == Int64_t || tk == Char) {
      t = INT64;
      if (tk == Int) next();
      else if (tk == Int32_t) { next(); t = INT32; }
      else if (tk == Int64_t) { next(); t = INT64; }
      else if (tk == Char) { next(); t = CHAR; }
      while (tk == Mul) { next(); t = t + PTR; }
      if (tk == ')') next(); else { printf("%d: bad cast\n", (int)line); exit(-1); }
      expr(Inc);
      ty = t;
    }
    else {
      expr(Assign);
      if (tk == ')') next(); else { printf("%d: close paren expected\n", (int)line); exit(-1); }
    }
  }
  else if (tk == Mul) {
    next(); expr(Inc);
    if (ty > INT64) ty = ty - PTR; else { printf("%d: bad dereference\n", (int)line); exit(-1); }
    if (ty == CHAR) *++e = LC;
    else if (ty == INT32) *++e = LI32;
    else if (ty > INT64 && ty < PTR) ; // struct value, keep address
    else *++e = LI;
  }
  else if (tk == And) {
    next(); expr(Inc);
    if (*e == LC || *e == LI32 || *e == LI) --e;
    else if (ty > INT64 && ty < PTR) ; // struct value, address already in accumulator
    else { printf("%d: bad address-of\n", (int)line); exit(-1); }
    ty = ty + PTR;
  }
  else if (tk == '!') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = 0; *++e = EQ; ty = INT64; }
  else if (tk == '~') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = -1; *++e = XOR; ty = INT64; }
  else if (tk == Add) { next(); expr(Inc); ty = INT64; }
  else if (tk == Sub) {
    next(); *++e = IMM;
    if (tk == Num) { *++e = -ival; next(); } else { *++e = -1; *++e = PSH; expr(Inc); *++e = MUL; }
    ty = INT64;
  }
  else if (tk == Inc || tk == Dec) {
    t = tk; next(); expr(Inc);
    if (*e == LC) { *e = PSH; *++e = LC; }
    else if (*e == LI32) { *e = PSH; *++e = LI32; }
    else if (*e == LI) { *e = PSH; *++e = LI; }
    else { printf("%d: bad lvalue in pre-increment\n", (int)line); exit(-1); }
    *++e = PSH;
    *++e = IMM;
    // For pointers, add/sub the size of pointed-to type
    // For scalars (CHAR, INT32, INT64), add/sub 1
    if (ty > PTR) {
      base_ty = ty - PTR;
      while (base_ty > PTR) base_ty = base_ty - PTR; // Remove all PTR layers
      if (base_ty == CHAR) *++e = sizeof(char);
      else if (base_ty == INT32) *++e = 4;
      else *++e = sizeof(int64_t); // INT64
    } else {
      *++e = 1; // Scalar increment is always 1
    }
    *++e = (t == Inc) ? ADD : SUB;
    if (ty == CHAR) *++e = SC;
    else if (ty == INT32) *++e = SI32;
    else *++e = SI;
  }
  else { printf("%d: bad expression\n", (int)line); exit(-1); }

  while (tk == '[' || tk == '.' || tk == Arrow) {
    if (tk == '[') {
      next();
      if (*e == LC || *e == LI32 || *e == LI) *e = PSH; else { printf("%d: bad index\n", (int)line); exit(-1); }
      expr(Assign);
      if (tk == ']') next(); else { printf("%d: close bracket expected\n", (int)line); exit(-1); }
      if (ty > INT64) {
        ty = ty - PTR;
        *++e = IMM;
        if (ty == CHAR) *++e = sizeof(char);
        else if (ty == INT32) *++e = 4;
        else *++e = sizeof(int64_t);
        *++e = MUL;
      }
      else { printf("%d: bad pointer in index\n", (int)line); exit(-1); }
      *++e = ADD;
      if (ty == CHAR) *++e = LC;
      else if (ty == INT32) *++e = LI32;
      else if (ty > INT64 && ty < PTR) ; // struct value in array
      else *++e = LI;
    }
    else if (tk == '.') {
      next();
      if (ty <= INT64 || ty >= PTR) { printf("%d: not a struct\n", (int)line); exit(-1); }
      memacc();
    }
    else if (tk == Arrow) {
      next();
      if (ty < PTR) { printf("%d: not a pointer\n", (int)line); exit(-1); }
      ty = ty - PTR;
      if (ty <= INT64 || ty >= PTR) { printf("%d: not a struct pointer\n", (int)line); exit(-1); }
      memacc();
    }
  }

  while (tk >= lev) { // "precedence climbing" or "Top Down Operator Precedence" method
    t = ty;
    if (tk == Assign) {
      next();
      if (*e == LC || *e == LI32 || *e == LI) *e = PSH;
      else { printf("%d: bad lvalue in assignment\n", (int)line); exit(-1); }
      expr(Assign); 
      if ((ty = t) == CHAR) *++e = SC;
      else if (ty == INT32) *++e = SI32;
      else *++e = SI;
    }
    else if (tk == Cond) {
      next();
      *++e = BZ; d = ++e;
      expr(Assign);
      if (tk == ':') next(); else { printf("%d: conditional missing colon\n", (int)line); exit(-1); }
      *d = (int64_t)(e + 3); *++e = JMP; d = ++e;
      expr(Cond);
      *d = (int64_t)(e + 1);
    }
    else if (tk == Lor) { next(); *++e = BNZ; d = ++e; expr(Lan); *d = (int64_t)(e + 1); ty = INT64; }
    else if (tk == Lan) { next(); *++e = BZ;  d = ++e; expr(Or);  *d = (int64_t)(e + 1); ty = INT64; }
    else if (tk == Or)  { next(); *++e = PSH; expr(Xor); *++e = OR;  ty = INT64; }
    else if (tk == Xor) { next(); *++e = PSH; expr(And); *++e = XOR; ty = INT64; }
    else if (tk == And) { next(); *++e = PSH; expr(Eq);  *++e = AND; ty = INT64; }
    else if (tk == Eq)  { next(); *++e = PSH; expr(Lt);  *++e = EQ;  ty = INT64; }
    else if (tk == Ne)  { next(); *++e = PSH; expr(Lt);  *++e = NE;  ty = INT64; }
    else if (tk == Lt)  { next(); *++e = PSH; expr(Shl); *++e = LT;  ty = INT64; }
    else if (tk == Gt)  { next(); *++e = PSH; expr(Shl); *++e = GT;  ty = INT64; }
    else if (tk == Le)  { next(); *++e = PSH; expr(Shl); *++e = LE;  ty = INT64; }
    else if (tk == Ge)  { next(); *++e = PSH; expr(Shl); *++e = GE;  ty = INT64; }
    else if (tk == Shl) { next(); *++e = PSH; expr(Add); *++e = SHL; ty = INT64; }
    else if (tk == Shr) { next(); *++e = PSH; expr(Add); *++e = SHR; ty = INT64; }
    else if (tk == Add) {
      next(); *++e = PSH; expr(Mul);
      if ((ty = t) > PTR) {
        *++e = PSH; *++e = IMM;
        ty = ty - PTR; // Get element type
        if (ty == CHAR) *++e = sizeof(char);
        else if (ty == INT32) *++e = 4;
        else *++e = sizeof(int64_t); // INT64 or pointers
        *++e = MUL;
        ty = t; // Restore pointer type
      }
      *++e = ADD;
    }
    else if (tk == Sub) {
      next(); *++e = PSH; expr(Mul);
      if (t > PTR && t == ty) {
        *++e = SUB; *++e = PSH; *++e = IMM;
        elem_ty = t - PTR;
        if (elem_ty == CHAR) *++e = sizeof(char);
        else if (elem_ty == INT32) *++e = 4;
        else *++e = sizeof(int64_t);
        *++e = DIV; ty = INT64;
      }
      else if ((ty = t) > PTR) {
        *++e = PSH; *++e = IMM;
        ty = ty - PTR;
        if (ty == CHAR) *++e = sizeof(char);
        else if (ty == INT32) *++e = 4;
        else *++e = sizeof(int64_t);
        *++e = MUL; *++e = SUB;
        ty = t;
      }
      else *++e = SUB;
    }
    else if (tk == Mul) { next(); *++e = PSH; expr(Inc); *++e = MUL; ty = INT64; }
    else if (tk == Div) { next(); *++e = PSH; expr(Inc); *++e = DIV; ty = INT64; }
    else if (tk == Mod) { next(); *++e = PSH; expr(Inc); *++e = MOD; ty = INT64; }
    else if (tk == Inc || tk == Dec) {
      if (*e == LC) { *e = PSH; *++e = LC; }
      else if (*e == LI32) { *e = PSH; *++e = LI32; }
      else if (*e == LI) { *e = PSH; *++e = LI; }
      else { printf("%d: bad lvalue in post-increment\n", (int)line); exit(-1); }
      *++e = PSH; *++e = IMM;
      // For pointers, add/sub the size of pointed-to type
      // For scalars (CHAR, INT32, INT64), add/sub 1
      if (ty > PTR) {
        base_ty = ty - PTR;
        while (base_ty > PTR) base_ty = base_ty - PTR; // Remove all PTR layers
        if (base_ty == CHAR) *++e = sizeof(char);
        else if (base_ty == INT32) *++e = 4;
        else *++e = sizeof(int64_t); // INT64
      } else {
        *++e = 1; // Scalar increment is always 1
      }
      *++e = (tk == Inc) ? ADD : SUB;
      if (ty == CHAR) *++e = SC;
      else if (ty == INT32) *++e = SI32;
      else *++e = SI;
      *++e = PSH; *++e = IMM;
      if (ty > PTR) {
        base_ty = ty - PTR;
        while (base_ty > PTR) base_ty = base_ty - PTR;
        if (base_ty == CHAR) *++e = sizeof(char);
        else if (base_ty == INT32) *++e = 4;
        else *++e = sizeof(int64_t);
      } else {
        *++e = 1;
      }
      *++e = (tk == Inc) ? SUB : ADD;
      next();
    }
    else if (tk == Brak) {
      next(); *++e = PSH; expr(Assign);
      if (tk == ']') next(); else { printf("%d: close bracket expected\n", (int)line); exit(-1); }
      if (t > PTR) {
        *++e = PSH; *++e = IMM;
        elem_ty = t - PTR;
        if (elem_ty == CHAR) *++e = sizeof(char);
        else if (elem_ty == INT32) *++e = 4;
        else *++e = sizeof(int64_t);
        *++e = MUL;
      }
      else if (t < PTR) { printf("%d: pointer type expected\n", (int)line); exit(-1); }
      *++e = ADD;
      ty = t - PTR;
      if (ty == CHAR) *++e = LC;
      else if (ty == INT32) *++e = LI32;
      else if (ty > INT64 && ty < PTR) ; // struct value
      else *++e = LI;
    }
    else { printf("%d: compiler error tk=%d\n", (int)line, (int)tk); exit(-1); }
  }
}


void stmt()
{
  int64_t *a, *b;

  if (tk == If) {
    next();
    if (tk == '(') next(); else { printf("%d: open paren expected\n", (int)line); exit(-1); }
    expr(Assign);
    if (tk == ')') next(); else { printf("%d: close paren expected\n", (int)line); exit(-1); }
    *++e = BZ; b = ++e;
    stmt();
    if (tk == Else) {
      *b = (int64_t)(e + 3); *++e = JMP; b = ++e;
      next();
      stmt();
    }
    *b = (int64_t)(e + 1);
  }
  else if (tk == While) {
    next();
    a = e + 1;
    if (tk == '(') next(); else { printf("%d: open paren expected\n", (int)line); exit(-1); }
    expr(Assign);
    if (tk == ')') next(); else { printf("%d: close paren expected\n", (int)line); exit(-1); }
    *++e = BZ; b = ++e;
    stmt();
    *++e = JMP; *++e = (int64_t)a;
    *b = (int64_t)(e + 1);
  }
  else if (tk == Return) {
    next();
    if (tk != ';') expr(Assign);
    *++e = LEV;
    if (tk == ';') next(); else { printf("%d: semicolon expected\n", (int)line); exit(-1); }
  }
  else if (tk == '{') {
    next();
    while (tk != '}') stmt();
    next();
  }
  else if (tk == ';') {
    next();
  }
  else {
    expr(Assign);
    if (tk == ';') next(); else { printf("%d: semicolon expected\n", (int)line); exit(-1); }
  }
}

int main(int argc, char **argv)
{
  int64_t fd, bt, ty, poolsz, *idmain, *s, *m;
  int64_t *pc, *sp, *bp, a, cycle;
  int64_t i, *t;

  --argc; ++argv;
  if (argc > 0 && **argv == '-' && (*argv)[1] == 's') { src = 1; --argc; ++argv; }
  if (argc > 0 && **argv == '-' && (*argv)[1] == 'd') { debug = 1; --argc; ++argv; }
  if (argc > 0 && **argv == '-' && (*argv)[1] == '-') { --argc; ++argv; }
  if (argc < 1) { printf("usage: c4 [-s] [-d] file ...\n"); return -1; }

  if ((fd = open(*argv, 0)) < 0) { printf("could not open(%s)\n", *argv); return -1; }

  poolsz = 256*1024; // arbitrary size
  if (!(sym = malloc(poolsz))) { printf("could not malloc(%d) symbol area\n", (int)poolsz); return -1; }
  if (!(le = e = malloc(poolsz))) { printf("could not malloc(%d) text area\n", (int)poolsz); return -1; }
  if (!(data = malloc(poolsz))) { printf("could not malloc(%d) data area\n", (int)poolsz); return -1; }
  if (!(sp = malloc(poolsz))) { printf("could not malloc(%d) stack area\n", (int)poolsz); return -1; }
  if (!(struct_syms = malloc(256 * sizeof(int64_t)))) { printf("could not malloc struct_syms\n"); return -1; }

  memset(sym,  0, poolsz);
  memset(e,    0, poolsz);
  memset(data, 0, poolsz);
  memset(struct_syms, 0, 256 * sizeof(int64_t));

  p = "char else enum if int int32_t int64_t return sizeof struct typedef union while "
      "open read close printf malloc free memset memcmp exit write system popen pclose fread void main";
  i = Char; while (i <= While) { next(); id[Tk] = i++; } // add keywords to symbol table
  i = OPEN; while (i <= FRED) { next(); id[Class] = Sys; id[Type] = INT64; id[Val] = i++; } // add library to symbol table
  next(); id[Tk] = Char; // handle void type
  next(); idmain = id; // keep track of main

  if (!(lp = p = malloc(poolsz))) { printf("could not malloc(%d) source area\n", (int)poolsz); return -1; }
  if ((i = read(fd, p, poolsz-1)) <= 0) { printf("read() returned %d\n", (int)i); return -1; }
  p[i] = 0;
  close(fd);

  // parse declarations
  line = 1;
  next();
  while (tk) {
    bt = INT64; // basetype (default to INT64 for backward compatibility)
    if (tk == Int) next();
    else if (tk == Int32_t) { next(); bt = INT32; }
    else if (tk == Int64_t) { next(); bt = INT64; }
    else if (tk == Char) { next(); bt = CHAR; }
    else if (tk == Enum) {
      next();
      if (tk != '{') next();
      if (tk == '{') {
        next();
        i = 0;
        while (tk != '}') {
          if (tk != Id) { printf("%d: bad enum identifier %d\n", (int)line, (int)tk); return -1; }
          next();
          if (tk == Assign) {
            next();
            if (tk != Num) { printf("%d: bad enum initializer\n", (int)line); return -1; }
            i = ival;
            next();
          }
          id[Class] = Num; id[Type] = INT64; id[Val] = i++;
          if (tk == ',') next();
        }
        next();
      }
    }

    else if (tk == Struct) {
      next();
      if (tk != '{') {
        s = id;
        next();
        if (s[Class] == Struct) bt = s[Type];

        // Restore id if we are defining it now
        if (tk == '{') id = s;
      }
      if (tk == '{') {
        s = id;
        s[Class] = Struct;
        if (num_structs >= PTR - INT64 - 1) { printf("%d: too many structs\n", (int)line); return -1; }
        s[Type] = INT64 + 1 + num_structs;
        struct_syms[num_structs] = (int64_t)s;
        num_structs = num_structs + 1;
        s[Sline] = 0;
        
        next();
        i = 0; 
        while (tk != '}') {
          bt = INT64;
          if (tk == Int) next();
          else if (tk == Int32_t) { next(); bt = INT32; }
          else if (tk == Int64_t) { next(); bt = INT64; }
          else if (tk == Char) { next(); bt = CHAR; }
          else if (tk == Struct) {
             next();
             if (tk == '{') { printf("%d: nested structs not supported\n", (int)line); return -1; }
             if (id[Class] == Struct) bt = id[Type];
             else { printf("%d: bad struct declaration\n", (int)line); return -1; }
             next();
          }
          while (tk != ';') {
            ty = bt;
            while (tk == Mul) { next(); ty = ty + PTR; }
            if (tk != Id) { printf("%d: bad struct member declaration\n", (int)line); return -1; }
            if (id[Class] == Loc) { printf("%d: duplicate struct member definition\n", (int)line); return -1; }
            
            m = malloc(4 * sizeof(int64_t));
            if (!m) { printf("%d: could not malloc member\n", (int)line); return -1; }
            m[0] = id[Hash];
            m[1] = ty;
            m[2] = i;
            m[3] = s[Sline];
            s[Sline] = (int64_t)m; 

            if (ty == CHAR) i = i + sizeof(char);
            else if (ty == INT32) i = i + 4;
            else if (ty > INT64 && ty < PTR) i = i + ((int64_t *)struct_syms[ty - INT64 - 1])[Val];
            else i = i + sizeof(int64_t);
 
            next();
            if (tk == ',') next();
          }
          next();
        }
        s[Val] = i; 
        next();
      }
    }

    while (tk != ';' && tk != '}') {
      ty = bt;
      while (tk == Mul) { next(); ty = ty + PTR; }
      if (tk != Id) { printf("%d: bad global declaration\n", (int)line); return -1; }
      if (id[Class]) { printf("%d: duplicate global definition\n", (int)line); return -1; }
      next();
      id[Type] = ty;
      if (tk == '(') { // function
        id[Class] = Fun;
        id[Val] = (int64_t)(e + 1);
        next(); i = 0;
        while (tk != ')') {
          ty = INT64;
          if (tk == Int) next();
          else if (tk == Int32_t) { next(); ty = INT32; }
          else if (tk == Int64_t) { next(); ty = INT64; }
          else if (tk == Char) { next(); ty = CHAR; }
          while (tk == Mul) { next(); ty = ty + PTR; }
          if (tk != Id) { printf("%d: bad parameter declaration\n", (int)line); return -1; }
          if (id[Class] == Loc) { printf("%d: duplicate parameter definition\n", (int)line); return -1; }
          id[HClass] = id[Class]; id[Class] = Loc;
          id[HType]  = id[Type];  id[Type] = ty;
          id[HVal]   = id[Val];   id[Val] = i++;
          next();
          if (tk == ',') next();
        }
        next();
        if (tk != '{') { printf("%d: bad function definition\n", (int)line); return -1; }
        loc = ++i;
        next();
        while (tk == Int || tk == Int32_t || tk == Int64_t || tk == Char || tk == Struct) {
          if (tk == Struct) {
            next();
            if (tk != Id) { printf("%d: bad struct declaration\n", (int)line); return -1; }
            if (id[Class] == Struct) {
              bt = id[Type]; // copy the Struct ID (address of symbol)
            } else {
              printf("%d: %s not a struct\n", (int)line, (char *)id[Name]); return -1;
            }
            next();
          } else {
            bt = INT64;
            if (tk == Int) next();
            else if (tk == Int32_t) { next(); bt = INT32; }
            else if (tk == Int64_t) { next(); bt = INT64; }
            else if (tk == Char) { next(); bt = CHAR; }
            else next();
          }
          while (tk != ';') {
            ty = bt;
            while (tk == Mul) { next(); ty = ty + PTR; }
            if (tk != Id) { printf("%d: bad local declaration\n", (int)line); return -1; }
            if (id[Class] == Loc) { printf("%d: duplicate local definition\n", (int)line); return -1; }
            id[HClass] = id[Class]; id[Class] = Loc;
            id[HType]  = id[Type];  id[Type] = ty;
            id[HVal]   = id[Val];
            if (ty > INT64 && ty < PTR) i = i + (((int64_t *)struct_syms[ty - INT64 - 1])[Val] + 7) / 8;
            else ++i;
            id[Val] = i;
            next();
            if (tk == ',') next();
          }
          next();
        }
        *++e = ENT; *++e = i - loc;
        while (tk != '}') stmt();
        *++e = LEV;
        id = sym; // unwind symbol table locals
        while (id[Tk]) {
          if (id[Class] == Loc) {
            id[Class] = id[HClass];
            id[Type] = id[HType];
            id[Val] = id[HVal];
          }
          id = id + Idsz;
        }
      }
      else {
        id[Class] = Glo;
        id[Val] = (int64_t)data;
        if (ty == CHAR) data = data + sizeof(char);
        else if (ty == INT32) data = data + 4;
        else if (ty > INT64 && ty < PTR) data = data + ((int64_t *)struct_syms[ty - INT64 - 1])[Val];
        else data = data + sizeof(int64_t);
      }
      if (tk == ',') next();
    }
    next();
  }

  if (!(pc = (int64_t *)idmain[Val])) { printf("main() not defined\n"); return -1; }
  if (src) return 0;

  // setup stack
  bp = sp = (int64_t *)((int64_t)sp + poolsz);
  *--sp = EXIT; // call exit if main returns
  *--sp = PSH; t = sp;
  *--sp = argc;
  *--sp = (int64_t)argv;
  *--sp = (int64_t)t;

  // run...
  cycle = 0;
  while (1) {
    i = *pc++; ++cycle;
    if (debug) {
      printf("%lld> %.4s", cycle,
        &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,LI32,SI  ,SC  ,SI32,PSH ,"
         "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
         "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,WRIT,SYST,POPN,PCLS,FRED,"[i * 5]);
      if (i <= ADJ) printf(" %lld\n", *pc); else printf("\n");
    }
    if      (i == LEA) a = (int64_t)(bp + *pc++);                             // load local address
    else if (i == IMM) a = *pc++;                                         // load global address or immediate
    else if (i == JMP) pc = (int64_t *)*pc;                                   // jump
    else if (i == JSR) { *--sp = (int64_t)(pc + 1); pc = (int64_t *)*pc; }        // jump to subroutine
    else if (i == BZ)  pc = a ? pc + 1 : (int64_t *)*pc;                      // branch if zero
    else if (i == BNZ) pc = a ? (int64_t *)*pc : pc + 1;                      // branch if not zero
    else if (i == ENT) { *--sp = (int64_t)bp; bp = sp; sp = sp - *pc++; }     // enter subroutine
    else if (i == ADJ) sp = sp + *pc++;                                   // stack adjust
    else if (i == LEV) { sp = bp; bp = (int64_t *)*sp++; pc = (int64_t *)*sp++; } // leave subroutine
    else if (i == LI)  a = *(int64_t *)a;                                     // load int64
    else if (i == LC)  a = *(char *)a;                                    // load char
    else if (i == LI32) a = *(int32_t *)a;                                // load int32 (sign-extend)
    else if (i == SI)  *(int64_t *)*sp++ = a;                                 // store int64
    else if (i == SC)  a = *(char *)*sp++ = a;                            // store char
    else if (i == SI32) *(int32_t *)*sp++ = (int32_t)a;                   // store int32
    else if (i == PSH) *--sp = a;                                         // push

    else if (i == OR)  a = *sp++ |  a;
    else if (i == XOR) a = *sp++ ^  a;
    else if (i == AND) a = *sp++ &  a;
    else if (i == EQ)  a = *sp++ == a;
    else if (i == NE)  a = *sp++ != a;
    else if (i == LT)  a = *sp++ <  a;
    else if (i == GT)  a = *sp++ >  a;
    else if (i == LE)  a = *sp++ <= a;
    else if (i == GE)  a = *sp++ >= a;
    else if (i == SHL) a = *sp++ << a;
    else if (i == SHR) a = *sp++ >> a;
    else if (i == ADD) a = *sp++ +  a;
    else if (i == SUB) a = *sp++ -  a;
    else if (i == MUL) a = *sp++ *  a;
    else if (i == DIV) a = *sp++ /  a;
    else if (i == MOD) a = *sp++ %  a;

    else if (i == OPEN) a = (pc[1] == 2) ? open((char *)sp[1], *sp) : open((char *)sp[2], sp[1], *sp);
    else if (i == READ) a = read(sp[2], (char *)sp[1], *sp);
    else if (i == CLOS) a = close(*sp);
    else if (i == PRTF) { t = sp + pc[1]; a = printf((char *)t[-1], t[-2], t[-3], t[-4], t[-5], t[-6]); }
    else if (i == MALC) a = (int64_t)malloc(*sp);
    else if (i == FREE) free((void *)*sp);
    else if (i == MSET) a = (int64_t)memset((char *)sp[2], sp[1], *sp);
    else if (i == MCMP) a = memcmp((char *)sp[2], (char *)sp[1], *sp);
    else if (i == EXIT) { printf("exit(%lld) cycle = %lld\n", *sp, cycle); return *sp; }
    else if (i == WRIT) a = write(sp[2], (char *)sp[1], *sp);
    else if (i == SYST) a = system((char *)*sp);
    else if (i == POPN) a = (int64_t)popen((char *)sp[1], (char *)*sp);
    else if (i == PCLS) a = pclose((void *)*sp);
    else if (i == FRED) a = fread((void *)sp[3], sp[2], sp[1], (void *)*sp);
    else { printf("unknown instruction = %lld! cycle = %lld\n", i, cycle); return -1; }
  }
}
