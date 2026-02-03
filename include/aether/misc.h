#ifndef MISC_H
#define MISC_H

#include "aether/vm.h"

#define DA_ARENA_APPEND(da, element, arena)                               \
  do {                                                                    \
    if ((da).cap <= (da).len) {                                           \
      if ((da).cap != 0) {                                                \
        while ((da).cap <= (da).len)                                      \
          (da).cap *= 2;                                                  \
        void *new_items = arena_alloc(arena, sizeof(element) * (da).cap); \
        memcpy(new_items,                                                 \
              (da).items,                                                 \
              (da).len * sizeof(element));                                \
        (da).items = new_items;                                           \
      } else {                                                            \
        (da).cap = 1;                                                     \
        (da).items = arena_alloc(arena, sizeof(element));                 \
      }                                                                   \
    }                                                                     \
    (da).items[(da).len++] = element;                                     \
  } while (false)

bool    value_to_bool(Value *value);
u64     value_hash(Value *value);
Value **dict_get_value_root(Dict *dict, Value *key);
Value  *dict_get_value(Dict *dict, Value *key);
void    dict_set_value(StackFrame *frame, Dict *dict,
                       Value *key, Value *value);
Value **get_child_root(Value *value, Value *key, InstrMeta *meta, Vm *vm);
void    sb_push_value(StringBuilder *sb, Value *value,
                      u32 level, bool kind, bool quote_string);
void    print_instr(Instr *instr, bool hide_strings);
void    fprint_value(FILE *stream, Value *value, bool kind);
void    print_value(Value *value, bool kind);
Value  *get_from_string(Vm *vm, Str string, i64 index);

#endif // MISC_H
