#include "intrinsics.h"
#include "shl_log.h"
#include "shl_arena.h"

#define DEFAULT_INPUT_BUFFER_SIZE 64

Intrinsic intrinsics[] = {
  { STR_LIT("print"), 1, false, &print_intrinsic },
  { STR_LIT("println"), 1, false, &println_intrinsic },
  { STR_LIT("input"), 0, true, &input_intrinsic },
  { STR_LIT("get-args"), 0, true, &get_args_intrinsic },
  { STR_LIT("head"), 1, true, &head_intrinsic },
  { STR_LIT("tail"), 1, true, &tail_intrinsic },
  { STR_LIT("is-empty"), 1, true, &is_empty_intrinsic },
  { STR_LIT("str-to-num"), 1, true, &str_to_num_intrinsic },
  { STR_LIT("num-to-str"), 1, true, &num_to_str_intrinsic },
  { STR_LIT("bool-to-str"), 1, true, &bool_to_str_intrinsic },
  { STR_LIT("bool-to-num"), 1, true, &bool_to_num_intrinsic },
  { STR_LIT("add"), 2, true, &add_intrinsic },
  { STR_LIT("sub"), 2, true, &sub_intrinsic },
  { STR_LIT("mul"), 2, true, &mul_intrinsic },
  { STR_LIT("div"), 2, true, &div_intrinsic },
  { STR_LIT("mod"), 2, true, &mod_intrinsic },
  { STR_LIT("eq"), 2, true, &eq_intrinsic },
  { STR_LIT("ne"), 2, true, &ne_intrinsic },
  { STR_LIT("ls"), 2, true, &ls_intrinsic },
  { STR_LIT("le"), 2, true, &le_intrinsic },
  { STR_LIT("gt"), 2, true, &gt_intrinsic },
  { STR_LIT("ge"), 2, true, &ge_intrinsic },
  { STR_LIT("not"), 1, true, &not_intrinsic },
};

u32 intrinsics_len = ARRAY_LEN(intrinsics);

static void print_value(ValueStack *stack, Value *value) {
  switch (value->kind) {
  case ValueKindUnit: {
    fputs("unit", stdout);
  } break;

  case ValueKindList: {
    fputc('[', stdout);

    ListNode *node = value->as.list;
    while (node) {
      if (node != value->as.list)
        fputc(' ', stdout);

      print_value(stack, &node->value);

      node = node->next;
    }

    fputc(']', stdout);
  } break;

  case ValueKindStr: {
    str_print(value->as.str);
  } break;

  case ValueKindNumber: {
    printf("%ld", value->as.number);
  } break;

  case ValueKindBool: {
    if (value->as._bool)
      fputs("true", stdout);
    else
      fputs("false", stdout);
  } break;
  }
}

void print_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  print_value(&vm->stack, &value);
}

void println_intrinsic(Vm *vm) {
  print_intrinsic(vm);
  fputc('\n', stdout);
}

void input_intrinsic(Vm *vm) {
  (void) vm;

  u32 buffer_size = DEFAULT_INPUT_BUFFER_SIZE;
  char *buffer = rc_arena_alloc(vm->rc_arena, buffer_size);
  u32 len = 0;

  char ch;
  while ((ch = getc(stdin)) != EOF && ch != '\n') {
    if (len >= buffer_size) {
      buffer_size += DEFAULT_INPUT_BUFFER_SIZE;

      char *prev_buffer = buffer;
      buffer = rc_arena_alloc(vm->rc_arena, buffer_size);
      memcpy(buffer, prev_buffer, len);
      rc_arena_free(vm->rc_arena, buffer);
    }

    buffer[len++] = ch;
  }

  value_stack_push_str(&vm->stack, STR(buffer, len));
}

void get_args_intrinsic(Vm *vm) {
  value_stack_push_list(&vm->stack, vm->args);
}

void head_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList) {
    ERROR("head: wrong argument kind\n");
    exit(1);
  }

  if (!value.as.list) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  DA_APPEND(vm->stack, value.as.list->value);
}

void tail_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList) {
    ERROR("tail: wrong argument kind\n");
    exit(1);
  }

  if (!value.as.list) {
    DA_APPEND(vm->stack, value);
    return;
  }

  value_stack_push_list(&vm->stack, value.as.list->next);
}

void is_empty_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList) {
    ERROR("is-empty: wrong argument kind: %u\n", value.kind);
    exit(1);
  }

  value_stack_push_bool(&vm->stack, value.as.list == NULL);
}

void str_to_num_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindStr) {
    ERROR("str-to-num: wrong argument kind");
    exit(1);
  }

  value_stack_push_number(&vm->stack, str_to_i64(value.as.str));
}

void num_to_str_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindNumber) {
    ERROR("num-to-str: wrong argument kind");
    exit(1);
  }

  StringBuilder sb = {0};
  sb_push_i64(&sb, value.as.number);

  value_stack_push_str(&vm->stack, sb_to_str(sb));
}

void bool_to_str_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool) {
    ERROR("bool-to-str: wrong argument kind");
    exit(1);
  }

  Str str;
  char *cstr;

  if (value.as._bool) {
    str.len = 4;
    cstr = "true";
  } else {
    str.len = 5;
    cstr = "false";
  }

  str.ptr = rc_arena_alloc(vm->rc_arena, str.len);
  memcpy(str.ptr, cstr, str.len);

  value_stack_push_str(&vm->stack, str);
}

void bool_to_num_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool) {
    ERROR("bool-to-num: wrong argument kind");
    exit(1);
  }

  value_stack_push_number(&vm->stack, value.as._bool);
}

void add_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_number(&vm->stack, a.as.number + b.as.number);
  } else if (a.kind == ValueKindStr &&
             b.kind == ValueKindStr) {
    StringBuilder sb = {0};
    sb_push_str(&sb, a.as.str);
    sb_push_str(&sb, b.as.str);

    value_stack_push_str(&vm->stack, sb_to_str(sb));
  } else {
    ERROR("add: wrong argument kinds\n");
    exit(1);
  }
}

static void prepare_two_numbers(Value *a, Value *b, char *intrinsic_name, Vm *vm) {
  *b = value_stack_pop(&vm->stack);
  *a = value_stack_pop(&vm->stack);

  if (a->kind != ValueKindNumber ||
      b->kind != ValueKindNumber) {
    ERROR("%s: wrong argument kinds\n", intrinsic_name);
    exit(1);
  }
}

void sub_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "sub", vm);

  value_stack_push_number(&vm->stack, a.as.number - b.as.number);
}

void mul_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "mul", vm);

  value_stack_push_number(&vm->stack, a.as.number * b.as.number);
}

void div_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "div", vm);

  value_stack_push_number(&vm->stack, a.as.number / b.as.number);
}

void mod_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "mod", vm);

  value_stack_push_number(&vm->stack, a.as.number % b.as.number);
}

void eq_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "eq", vm);

  value_stack_push_bool(&vm->stack, a.as.number == b.as.number);
}

void ne_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ne", vm);

  value_stack_push_bool(&vm->stack, a.as.number != b.as.number);
}

void ls_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ls", vm);

  value_stack_push_bool(&vm->stack, a.as.number < b.as.number);
}

void le_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "le", vm);

  value_stack_push_bool(&vm->stack, a.as.number <= b.as.number);
}

void gt_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "gt", vm);

  value_stack_push_bool(&vm->stack, a.as.number > b.as.number);
}

void ge_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "ge", vm);

  value_stack_push_bool(&vm->stack, a.as.number >= b.as.number);
}

void not_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool) {
    ERROR("not: wrong argument kind\n");
    exit(1);
  }

  value_stack_push_bool(&vm->stack, !value.as._bool);
}
