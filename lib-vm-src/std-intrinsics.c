#include "std-intrinsics.h"
#include "io.h"
#include "shl_log.h"
#include "shl_arena.h"

#define DEFAULT_INPUT_BUFFER_SIZE 64

static void print_value(ValueStack *stack, Value *value) {
  switch (value->kind) {
  case ValueKindUnit: {
    fputs("unit", stdout);
  } break;

  case ValueKindList: {
    fputc('[', stdout);

    ListNode *node = value->as.list->next;
    while (node) {
      if (node != value->as.list->next)
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

  case ValueKindFunc: {
    fputs("(fun [", stdout);
    fputs(" [", stdout);

    for (u32 i = 0; i < value->as.func.args.len; ++i) {
      if (i > 0)
        fputc(' ', stdout);
      str_print(value->as.func.args.items[i]);
    }

    fputs("])", stdout);
  } break;
  }
}

void print_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  print_value(&vm->stack, &value);
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
      rc_arena_free(vm->rc_arena, prev_buffer);
    }

    buffer[len++] = ch;
  }

  value_stack_push_str(&vm->stack, STR(buffer, len));
}

void read_file_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindStr) {
    ERROR("read-file: wrong argument kind\n");
    exit(1);
  }

  char *path_cstr = aalloc(path.as.str.len + 1);
  memcpy(path_cstr, path.as.str.ptr, path.as.str.len);
  path_cstr[path.as.str.len] = '\0';

  Str content = read_file(path_cstr);
  if (content.len == (u32) -1)
    value_stack_push_unit(&vm->stack);
  else
    value_stack_push_str(&vm->stack, content);
}

void write_file_intrinsic(Vm *vm) {
  Value path = value_stack_pop(&vm->stack);
  Value content = value_stack_pop(&vm->stack);
  if (path.kind != ValueKindStr ||
      content.kind != ValueKindStr) {
    ERROR("write-file: wrong argument kinds\n");
    exit(1);
  }

  char *path_cstr = aalloc(path.as.str.len + 1);
  memcpy(path_cstr, path.as.str.ptr, path.as.str.len);
  path_cstr[path.as.str.len] = '\0';

  write_file(path_cstr, content.as.str);
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

  if (!value.as.list->next) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  DA_APPEND(vm->stack, value.as.list->next->value);
}

void tail_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList) {
    ERROR("tail: wrong argument kind\n");
    exit(1);
  }

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  if (value.as.list->next)
    new_list->next = list_clone(vm->rc_arena, value.as.list->next->next);

  value_stack_push_list(&vm->stack, new_list);
}

void len_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind == ValueKindList) {
    ListNode *node = value.as.list->next;
    u32 len = 0;
    while (node) {
      node = node->next;
      ++len;
    }

    value_stack_push_number(&vm->stack, len);
  } else if (value.kind == ValueKindStr) {
    value_stack_push_number(&vm->stack, value.as.str.len);
  } else {
    ERROR("len: wrong argument kind\n");
    exit(1);
  }
}

void is_empty_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindList &&
      value.kind != ValueKindStr) {
    ERROR("is-empty: wrong argument kind\n");
    exit(1);
  }

  if (value.kind == ValueKindList)
    value_stack_push_bool(&vm->stack, value.as.list->next == NULL);
  else
    value_stack_push_bool(&vm->stack, value.as.str.len == 0);
}

void get_range_intrinsic(Vm *vm) {
  Value end = value_stack_pop(&vm->stack);
  Value begin = value_stack_pop(&vm->stack);
  Value value = value_stack_pop(&vm->stack);
  if (begin.kind != ValueKindNumber ||
      end.kind != ValueKindNumber ||
      (value.kind != ValueKindList &&
       value.kind != ValueKindStr)) {
    ERROR("get-range: wrong argument kinds\n");
    exit(1);
  }

  DA_APPEND(vm->stack, value);
  len_intrinsic(vm);
  Value len = value_stack_pop(&vm->stack);

  if (begin.as.number < 0 || begin.as.number >= len.as.number ||
      end.as.number <= 0 || end.as.number >= len.as.number ||
      begin.as.number >= end.as.number) {
    value_stack_push_unit(&vm->stack);
    return;
  }

  if (value.kind == ValueKindList) {
    ListNode *node = value.as.list->next;
    ListNode *sub_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    ListNode *sub_list_node = sub_list;

    for (u32 i = 0; i < begin.as.number; ++i)
      node = node->next;

    for (u32 i = 0; i < end.as.number - begin.as.number; ++i) {
      sub_list_node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      sub_list_node->next->value = node->value;

      node = node->next;
      sub_list_node = sub_list_node->next;
    }

    value_stack_push_list(&vm->stack, sub_list);
  } else {
    Str sub_str = {
      value.as.str.ptr + begin.as.number,
      value.as.str.len - begin.as.number - end.as.number,
    };

    value_stack_push_str(&vm->stack, sub_str);
  }
}

void map_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList) {
    ERROR("map: wrong argument kinds\n");
    exit(1);
  }

  ListNode *new_list = NULL;
  ListNode **new_list_next = &new_list;
  ListNode *node = list.as.list->next;
  while (node) {
    ValueStack args = {0};
    DA_APPEND(args, node->value);
    execute_func(vm, func.as.func.name, &args, true);

    *new_list_next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    (*new_list_next)->value = value_stack_pop(&vm->stack);
    *new_list_next = (*new_list_next)->next;

    free(args.items);

    node = node->next;
  }

  value_stack_push_list(&vm->stack, new_list);
}

void filter_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList) {
    ERROR("filter: wrong argument kinds\n");
    exit(1);
  }

  ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode **new_list_next = &new_list->next;
  ListNode *node = list.as.list->next;
  while (node) {
    ValueStack args = {0};
    DA_APPEND(args, node->value);
    execute_func(vm, func.as.func.name, &args, true);

    Value is_ok = value_stack_pop(&vm->stack);
    if (is_ok.kind != ValueKindBool) {
      ERROR("filter: wrong argument kinds\n");
      exit(1);
    }

    if (is_ok.as._bool) {
      *new_list_next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      (*new_list_next)->value = node->value;
      new_list_next = &(*new_list_next)->next;
    }

    free(args.items);

    node = node->next;
  }

  value_stack_push_list(&vm->stack, new_list);
}

void reduce_intrinsic(Vm *vm) {
  Value list = value_stack_pop(&vm->stack);
  Value initial_value = value_stack_pop(&vm->stack);
  Value func = value_stack_pop(&vm->stack);
  if (func.kind != ValueKindFunc ||
      list.kind != ValueKindList) {
    ERROR("reduce: wrong argument kinds\n");
    exit(1);
  }

  Value accumulator = initial_value;
  ListNode *node = list.as.list->next;
  while (node) {
    ValueStack args = {0};
    DA_APPEND(args, accumulator);
    DA_APPEND(args, node->value);
    execute_func(vm, func.as.func.name, &args, true);

    accumulator = value_stack_pop(&vm->stack);
    if (accumulator.kind != initial_value.kind) {
      ERROR("reduce: return value's and accumulator's types should be equal\n");
      exit(1);
    }

    node = node->next;
  }

  DA_APPEND(vm->stack, accumulator);
}

void split_intrinsic(Vm *vm) {
  Value delimeter = value_stack_pop(&vm->stack);
  Value str = value_stack_pop(&vm->stack);
  if (str.kind != ValueKindStr ||
      delimeter.kind != ValueKindStr) {
    ERROR("split: wrong argument kinds\n");
    exit(1);
  }

  ListNode *list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
  ListNode *node = list;
  u32 index = 0, i = 0;
  for (; i < str.as.str.len; ++i) {
    u32 found = true;
    for (u32 j = 0; j + i < str.as.str.len && j < delimeter.as.str.len; ++j) {
      if (str.as.str.ptr[j + i] != delimeter.as.str.ptr[j]) {
        found = false;
        break;
      }
    }

    if (found) {
      node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      node->next->value = (Value) {
        ValueKindStr,
        { .str = STR(str.as.str.ptr + index, i - index) },
      };

      index = i + 1;
      node = node->next;
    }
  }

  if (i > 0) {
    node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    node->next->value = (Value) {
      ValueKindStr,
      { .str = STR(str.as.str.ptr + index, i - index) },
    };
  }

  value_stack_push_list(&vm->stack, list);
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
  } else if (a.kind == ValueKindList &&
             b.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(vm->rc_arena, a.as.list->next);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node)
      node->next = list_clone(vm->rc_arena, b.as.list->next);

    value_stack_push_list(&vm->stack, new_list);
  } else if (a.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = list_clone(vm->rc_arena, a.as.list->next);

    ListNode *node = new_list;
    while (node && node->next)
      node = node->next;

    if (node) {
      node->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
      node->next->value = b;
    }

    value_stack_push_list(&vm->stack, new_list);
  } else if (b.kind == ValueKindList) {
    ListNode *new_list = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next = rc_arena_alloc(vm->rc_arena, sizeof(ListNode));
    new_list->next->value = a;
    new_list->next->next = list_clone(vm->rc_arena, b.as.list->next);

    value_stack_push_list(&vm->stack, new_list);
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

void abs_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindNumber) {
    ERROR("abs: wrong argument kind\n");
    exit(1);
  }

  if (value.as.number < 0)
    value.as.number = -value.as.number;

  DA_APPEND(vm->stack, value);
}

void min_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "min", vm);

  value_stack_push_number(&vm->stack, a.as.number <= b.as.number ?
                                      a.as.number :
                                      b.as.number);
}

void max_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "max", vm);

  value_stack_push_number(&vm->stack, a.as.number >= b.as.number ?
                                      a.as.number :
                                      b.as.number);
}

void eq_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_bool(&vm->stack, a.as.number == b.as.number);
  } else if (a.kind == ValueKindStr &&
             b.kind == ValueKindStr) {
    value_stack_push_bool(&vm->stack, str_eq(a.as.str, b.as.str));
  } else {
    ERROR("eq: wrong argument kinds: %u:%u\n", a.kind, b.kind);
    exit(1);
  }
}

void ne_intrinsic(Vm *vm) {
  Value b = value_stack_pop(&vm->stack);
  Value a = value_stack_pop(&vm->stack);

  if (a.kind == ValueKindNumber &&
      b.kind == ValueKindNumber) {
    value_stack_push_bool(&vm->stack, a.as.number != b.as.number);
  } else if (a.kind == ValueKindStr &&
             b.kind == ValueKindStr) {
    value_stack_push_bool(&vm->stack, !str_eq(a.as.str, b.as.str));
  } else {
    ERROR("ne: wrong argument kinds\n");
    exit(1);
  }
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

void and_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "gt", vm);

  value_stack_push_number(&vm->stack, a.as.number & b.as.number);
}

void or_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "gt", vm);

  value_stack_push_number(&vm->stack, a.as.number | b.as.number);
}

void xor_intrinsic(Vm *vm) {
  Value a, b;
  prepare_two_numbers(&a, &b, "gt", vm);

  value_stack_push_number(&vm->stack, a.as.number ^ b.as.number);
}

void not_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  if (value.kind != ValueKindBool) {
    ERROR("not: wrong argument kind\n");
    exit(1);
  }

  value_stack_push_bool(&vm->stack, !value.as._bool);
}

void is_unit_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindUnit);
}

void is_list_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindList);
}

void is_str_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindStr);
}

void is_number_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindNumber);
}

void is_bool_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindBool);
}

void is_fun_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);
  value_stack_push_bool(&vm->stack, value.kind == ValueKindFunc);
}

void type_intrinsic(Vm *vm) {
  Value value = value_stack_pop(&vm->stack);

  if (value.kind == ValueKindUnit) {
    value_stack_push_str(&vm->stack, STR_LIT("unit"));
  } else if (value.kind == ValueKindList) {
    value_stack_push_str(&vm->stack, STR_LIT("list"));
  } else if (value.kind == ValueKindStr) {
    value_stack_push_str(&vm->stack, STR_LIT("str"));
  } else if (value.kind == ValueKindNumber) {
    value_stack_push_str(&vm->stack, STR_LIT("number"));
  } else if (value.kind == ValueKindBool) {
    value_stack_push_str(&vm->stack, STR_LIT("bool"));
  } else if (value.kind == ValueKindFunc) {
    value_stack_push_str(&vm->stack, STR_LIT("fun"));
  } else {
    ERROR("Unknown type\n");
    exit(1);
  }
}

Intrinsic std_intrinsics[] = {
  // Io
  { STR_LIT("print"), 1, false, &print_intrinsic },
  { STR_LIT("input"), 0, true, &input_intrinsic },
  { STR_LIT("read-file"), 1, true, &read_file_intrinsic },
  { STR_LIT("write-file"), 2, false, &write_file_intrinsic },
  { STR_LIT("get-args"), 0, true, &get_args_intrinsic },
  // Base
  { STR_LIT("head"), 1, true, &head_intrinsic },
  { STR_LIT("tail"), 1, true, &tail_intrinsic },
  { STR_LIT("len"), 1, true, &len_intrinsic },
  { STR_LIT("is-empty"), 1, true, &is_empty_intrinsic },
  { STR_LIT("get-range"), 3, true, &get_range_intrinsic },
  // Functional stuff
  { STR_LIT("map"), 2, true, &map_intrinsic },
  { STR_LIT("filter"), 2, true, &filter_intrinsic },
  { STR_LIT("reduce"), 3, true, &reduce_intrinsic },
  // String operations
  { STR_LIT("split"), 2, true, &split_intrinsic },
  // Conversions
  { STR_LIT("str-to-num"), 1, true, &str_to_num_intrinsic },
  { STR_LIT("num-to-str"), 1, true, &num_to_str_intrinsic },
  { STR_LIT("bool-to-str"), 1, true, &bool_to_str_intrinsic },
  { STR_LIT("bool-to-num"), 1, true, &bool_to_num_intrinsic },
  // Math
  { STR_LIT("add"), 2, true, &add_intrinsic },
  { STR_LIT("sub"), 2, true, &sub_intrinsic },
  { STR_LIT("mul"), 2, true, &mul_intrinsic },
  { STR_LIT("div"), 2, true, &div_intrinsic },
  { STR_LIT("mod"), 2, true, &mod_intrinsic },
  // Advanced math
  { STR_LIT("abs"), 1, true, &abs_intrinsic },
  { STR_LIT("min"), 2, true, &min_intrinsic },
  { STR_LIT("max"), 2, true, &max_intrinsic },
  // TODO: floating point numbers
  // { STR_LIT("sqrt"), 1, true, &sqrt_intrinsic },
  // { STR_LIT("pow"), 2, true, &pow_intrinsic },
  // { STR_LIT("round"), 1, true, &round_intrinsic },
  // Comparisons
  { STR_LIT("eq"), 2, true, &eq_intrinsic },
  { STR_LIT("ne"), 2, true, &ne_intrinsic },
  { STR_LIT("ls"), 2, true, &ls_intrinsic },
  { STR_LIT("le"), 2, true, &le_intrinsic },
  { STR_LIT("gt"), 2, true, &gt_intrinsic },
  { STR_LIT("ge"), 2, true, &ge_intrinsic },
  // Boolean
  { STR_LIT("and"), 2, true, &and_intrinsic },
  { STR_LIT("or"), 2, true, &or_intrinsic },
  { STR_LIT("xor"), 2, true, &xor_intrinsic },
  { STR_LIT("not"), 1, true, &not_intrinsic },
  // Types
  { STR_LIT("unit?"), 1, true, &is_unit_intrinsic },
  { STR_LIT("list?"), 1, true, &is_list_intrinsic },
  { STR_LIT("str?"), 1, true, &is_str_intrinsic },
  { STR_LIT("number?"), 1, true, &is_number_intrinsic },
  { STR_LIT("bool?"), 1, true, &is_bool_intrinsic },
  { STR_LIT("fun?"), 1, true, &is_fun_intrinsic },
  { STR_LIT("type"), 1, true, &type_intrinsic },
};

u32 std_intrinsics_len = ARRAY_LEN(std_intrinsics);
