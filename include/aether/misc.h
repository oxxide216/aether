#ifndef MISC_H
#define MISC_H

#include "aether/vm.h"

#define SB_PUSH_VALUE(sb, value, level, kind, quote_string, vm) \
  do {                                                          \
    sb_push_value(sb, value, level, kind, quote_string, vm);    \
    if (vm->state != ExecStateContinue &&                       \
        vm->state != ExecStateReturn)                           \
      return NULL;                                              \
  } while (0)

bool   value_to_bool(Value *value);
Value *dict_get_value(Dict *dict, Value *key);
void   dict_set_value(StackFrame *frame, Dict *dict,
                      Value *key, Value *value);
void   sb_push_value(StringBuilder *sb, Value *value,
                     u32 level, bool kind,
                     bool quote_string, Vm *vm);

#endif // MISC_H
