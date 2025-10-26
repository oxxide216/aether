#ifndef MISC_H
#define MISC_H

#include "aether/vm.h"

bool prepare_rwo_numbers(Value **a, Value **b, Vm *vm);
bool value_to_bool(Value *value);
void dict_push_value(Dict *dict, Value *key, Value *value);
void dict_push_value_str_key(RcArena *rc_arena, Dict *dict,
                             Str key, Value *value);

#endif // MISC_H
