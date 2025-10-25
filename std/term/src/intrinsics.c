#include <sys/ioctl.h>
#include <termios.h>

#include "term/intrinsics.h"
#include "aether/misc.h"

static struct termios default_term_state = {0};
bool is_term_state_initialized = false;

bool get_size_intrinsic(Vm *vm) {
  struct winsize _size;
  ioctl(0, TIOCGWINSZ, &_size);

  Dict size = {0};

  Value *rows = rc_arena_alloc(vm->rc_arena, sizeof(Value));
  *rows = (Value) { ValueKindInt, { ._int = _size.ws_row }, 0 };
  dict_push_value_str_key(vm->rc_arena, &size, STR_LIT("rows"), rows);

  Value *cols = rc_arena_alloc(vm->rc_arena, sizeof(Value));
  *cols = (Value) { ValueKindInt, { ._int = _size.ws_col }, 0 };
  dict_push_value_str_key(vm->rc_arena, &size, STR_LIT("cols"), cols);

  value_stack_push_dict(&vm->stack, vm->rc_arena, size);

  return true;
}

bool raw_mode_on_intrinsic(Vm *vm) {
  (void) vm;

  if (!is_term_state_initialized) {
    tcgetattr(0, &default_term_state);
    is_term_state_initialized = true;
  }

  struct termios term_state = default_term_state;
  term_state.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(0, TCSAFLUSH, &term_state);

  return true;
}

bool raw_mode_off_intrinsic(Vm *vm) {
  (void) vm;

  if (is_term_state_initialized)
    tcsetattr(0, TCSAFLUSH, &default_term_state);

  return true;
}

Intrinsic term_intrinsics[] = {
  { STR_LIT("term/get-size"), true, 0, {}, &get_size_intrinsic },
  { STR_LIT("term/raw-mode-on"), false, 0, {}, &raw_mode_on_intrinsic },
  { STR_LIT("term/raw-mode-off"), false, 0, {}, &raw_mode_off_intrinsic },
};

u32 term_intrinsics_len = ARRAY_LEN(term_intrinsics);
