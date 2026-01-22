#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>

#include "aether/vm.h"
#include "aether/misc.h"

bool catch_kill = false;

static struct termios default_term_state = {0};
static bool is_term_state_initialized = false;

Value *get_size_intrinsic(Vm *vm, Value **args) {
  (void) args;

  struct winsize _size;
  ioctl(0, TIOCGWINSZ, &_size);

  Dict size = {0};

  Value *rows = value_alloc(vm->current_frame);
  rows->kind = ValueKindInt;
  rows->as._int = _size.ws_row;
  Value *rows_key = value_string(STR_LIT("rows"), vm->current_frame);
  dict_set_value(vm->current_frame, &size, rows_key, rows);

  Value *cols = value_alloc(vm->current_frame);
  cols->kind = ValueKindInt;
  cols->as._int = _size.ws_col;
  Value *cols_key = value_string(STR_LIT("cols"), vm->current_frame);
  dict_set_value(vm->current_frame, &size, cols_key, cols);

  return value_dict(size, vm->current_frame);
}

Value *raw_mode_on_intrinsic(Vm *vm, Value **args) {
  (void) vm;
  (void) args;

  if (!is_term_state_initialized) {
    tcgetattr(0, &default_term_state);
    is_term_state_initialized = true;
  }

  struct termios term_state = default_term_state;
  term_state.c_lflag = ~(ECHO | ICANON);
  tcsetattr(0, TCSANOW, &term_state);

  catch_kill = true;

  return value_unit(vm->current_frame);
}

Value *raw_mode_off_intrinsic(Vm *vm, Value **args) {
  (void) vm;
  (void) args;

  if (is_term_state_initialized)
    tcsetattr(0, TCSANOW, &default_term_state);

  catch_kill = false;

  return value_unit(vm->current_frame);
}

Intrinsic term_intrinsics[] = {
  { STR_LIT("term/get-size"), true, 0, {}, &get_size_intrinsic, NULL },
  { STR_LIT("term/raw-mode-on"), false, 0, {}, &raw_mode_on_intrinsic, NULL },
  { STR_LIT("term/raw-mode-off"), false, 0, {}, &raw_mode_off_intrinsic, NULL },
};

u32 term_intrinsics_len = ARRAY_LEN(term_intrinsics);
