#include "../src/compiler.h"
#include "../src/object.h"
#include "../src/op.h"
#include "../src/vm.h"
#include "test.h"

static VM vm;
static ObjectFunction *out_func;
static int byte_idx = 0;
static uint16_t jump_val;

#define COMPILER_INIT()                                                                            \
  InterpretResult result;                                                                          \
  mesche_vm_init(&vm, 0, NULL);

#define COMPILE(source)                                                                            \
  byte_idx = 0;                                                                                    \
  out_func = mesche_compile_source(&vm, source);                                                   \
  if (out_func == NULL) {                                                                          \
    FAIL("Compilation failed!");                                                                   \
  }

#define CHECK_SET_FUNC(func)                                                                       \
  out_func = func;                                                                                 \
  byte_idx = 0;

#define CHECK_BYTE(byte)                                                                           \
  if (out_func->chunk.code[byte_idx] != byte) {                                                    \
    FAIL("Expected byte %s, got %d", __stringify(byte), out_func->chunk.code[byte_idx]);           \
  }                                                                                                \
  byte_idx++;

#define CHECK_BYTES(byte1, byte2)                                                                  \
  CHECK_BYTE(byte1);                                                                               \
  CHECK_BYTE(byte2);

#define CHECK_JUMP(instr, source, dest)                                                            \
  CHECK_BYTE(instr);                                                                               \
  byte_idx += 2;

#define CHECK_CALL(instr, arg_count, keyword_count)                                                \
  CHECK_BYTE(instr);                                                                               \
  CHECK_BYTE(arg_count);                                                                           \
  CHECK_BYTE(keyword_count);

/* jump_val = (uint16_t)(out_func->chunk.code[byte_idx + 1] << 8); \ */
/* jump_val |= out_func->chunk.code[byte_idx + 2]; \ */
/* ASSERT_INT(byte_idx, offset + 3 + sign * jump_val); */

static void compiles_module_import() {
  COMPILER_INIT();

  COMPILE("(module-import (test alpha))");

  CHECK_BYTES(OP_CONSTANT, 0);
  CHECK_BYTES(OP_CONSTANT, 1);
  CHECK_BYTES(OP_LIST, 2);
  CHECK_BYTE(OP_RESOLVE_MODULE);
  CHECK_BYTE(OP_IMPORT_MODULE);
  CHECK_BYTE(OP_RETURN);

  PASS();
}

static void compiles_let() {
  COMPILER_INIT();

  COMPILE("(let ((x 3) (y 4))"
          "  (+ x y))");

  // A let is turned into an anonymous lambda, so check both the usage site and
  // the lambda definition

  CHECK_BYTES(OP_CLOSURE, 2);
  CHECK_BYTES(OP_CONSTANT, 0);
  CHECK_BYTES(OP_CONSTANT, 1);
  CHECK_CALL(OP_CALL, 2, 0);
  CHECK_BYTE(OP_RETURN);

  CHECK_SET_FUNC(AS_FUNCTION(out_func->chunk.constants.values[2]));
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_BYTES(OP_READ_LOCAL, 2);
  CHECK_BYTE(OP_ADD);
  CHECK_BYTES(OP_POP_SCOPE, 1);
  CHECK_BYTES(OP_POP_SCOPE, 1);
  CHECK_BYTE(OP_RETURN);

  PASS();
}

static void compiles_named_let() {
  COMPILER_INIT();

  COMPILE("(let test-let ((x 3) (y 4))"
          "  (test-let (* x y) y))");

  // A let is turned into an anonymous lambda, so check both the usage site and
  // the lambda definition

  CHECK_BYTES(OP_CLOSURE, 2);
  CHECK_BYTES(OP_CONSTANT, 0);
  CHECK_BYTES(OP_CONSTANT, 1);
  CHECK_CALL(OP_CALL, 2, 0);
  CHECK_BYTE(OP_RETURN);

  CHECK_SET_FUNC(AS_FUNCTION(out_func->chunk.constants.values[2]));

  CHECK_BYTES(OP_READ_LOCAL, 0);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_BYTES(OP_READ_LOCAL, 2);
  CHECK_BYTE(OP_MULTIPLY);
  CHECK_BYTES(OP_READ_LOCAL, 2);
  CHECK_CALL(OP_TAIL_CALL, 2, 0);
  CHECK_BYTES(OP_POP_SCOPE, 1);
  CHECK_BYTES(OP_POP_SCOPE, 1);
  CHECK_BYTE(OP_RETURN);

  PASS();
}

/*

  Check section 11.20 of R6RS spec, it mentions how to determine tail contexts.

  "A tail call is a procedure call that occurs in a tail context."
*/

static void compiles_tail_call_basic() {
  COMPILER_INIT();

  COMPILE("(define (test-func x)"
          "  (next-func x)"
          "  (next-func x))");

  CHECK_SET_FUNC(AS_FUNCTION(out_func->chunk.constants.values[1]));
  CHECK_BYTES(OP_READ_GLOBAL, 0);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_CALL(OP_CALL, 1, 0);
  CHECK_BYTE(OP_POP);
  CHECK_BYTES(OP_READ_GLOBAL, 0);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_CALL(OP_TAIL_CALL, 1, 0);
  CHECK_BYTE(OP_RETURN);

  // Not a tail call

  COMPILE("(define (test-func x)"
          "  (+ 1 (next-func x)))");

  CHECK_SET_FUNC(AS_FUNCTION(out_func->chunk.constants.values[1]));
  CHECK_BYTES(OP_CONSTANT, 0);
  CHECK_BYTES(OP_READ_GLOBAL, 1);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_CALL(OP_CALL, 1, 0);
  CHECK_BYTE(OP_ADD);
  CHECK_BYTE(OP_RETURN);

  PASS();
}

static void compiles_tail_call_begin() {
  COMPILER_INIT();

  COMPILE("(begin"
          "  (next-func x)"
          "  (next-func x))");

  CHECK_BYTES(OP_READ_GLOBAL, 0);
  CHECK_BYTES(OP_READ_GLOBAL, 1);
  CHECK_CALL(OP_CALL, 1, 0);
  CHECK_BYTE(OP_POP);
  CHECK_BYTES(OP_READ_GLOBAL, 0);
  CHECK_BYTES(OP_READ_GLOBAL, 1);
  CHECK_CALL(OP_TAIL_CALL, 1, 0);
  CHECK_BYTE(OP_RETURN);

  // Not a tail call

  COMPILE("(begin"
          "  (* 1 (next-func x)))");

  CHECK_BYTES(OP_CONSTANT, 0);
  CHECK_BYTES(OP_READ_GLOBAL, 1);
  CHECK_BYTES(OP_READ_GLOBAL, 2);
  CHECK_CALL(OP_CALL, 1, 0);
  CHECK_BYTE(OP_MULTIPLY);
  CHECK_BYTE(OP_RETURN);

  PASS();
}

static void compiles_tail_call_let() {
  COMPILER_INIT();

  COMPILE("(let ((x 1))"
          "  (next-func x)"
          "  (next-func x))");

  out_func = AS_FUNCTION(out_func->chunk.constants.values[1]);
  CHECK_BYTES(OP_READ_GLOBAL, 0);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_CALL(OP_CALL, 1, 0);
  CHECK_BYTE(OP_POP);
  CHECK_BYTES(OP_READ_GLOBAL, 0);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_CALL(OP_TAIL_CALL, 1, 0);
  CHECK_BYTES(OP_POP_SCOPE, 1);
  CHECK_BYTE(OP_RETURN);

  PASS();
}

static void compiles_tail_call_if_expr() {
  COMPILER_INIT();

  COMPILE("(define (test-func x)"
          "  (if (not (equal? x 5))"
          "      (test-func (+ x 1))"
          "      x))");

  out_func = AS_FUNCTION(out_func->chunk.constants.values[1]);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_BYTES(OP_CONSTANT, 0);
  CHECK_BYTE(OP_EQUAL);
  CHECK_BYTE(OP_NOT);
  CHECK_JUMP(OP_JUMP_IF_FALSE, 6, 23);
  CHECK_BYTE(OP_POP);
  CHECK_BYTES(OP_READ_GLOBAL, 1);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_BYTES(OP_CONSTANT, 2);
  CHECK_BYTE(OP_ADD);
  CHECK_CALL(OP_TAIL_CALL, 1, 0);
  CHECK_JUMP(OP_JUMP, 20, 26);
  CHECK_BYTE(OP_POP);
  CHECK_BYTES(OP_READ_LOCAL, 1);
  CHECK_BYTE(OP_RETURN);

  PASS();
}

static void compiler_suite_cleanup() { mesche_vm_free(&vm); }

void test_compiler_suite() {
  SUITE();

  test_suite_cleanup_func = compiler_suite_cleanup;

  compiles_module_import();
  compiles_let();
  compiles_named_let();
  compiles_tail_call_basic();
  compiles_tail_call_begin();
  compiles_tail_call_let();
  compiles_tail_call_if_expr();

  END_SUITE();
}
