#ifndef MISC_H
#define MISC_H

#include "aether/vm.h"

bool value_to_bool(Value *value);
void dict_push_value(Dict *dict, Value *key, Value *value);
void dict_push_value_str_key(RcArena *rc_arena, Dict *dict,
                             Str key, Value *value);
Value *dict_get_value_str_key(RcArena *rc_arena, Dict *dict, Str key);
void sb_push_value(StringBuilder *sb, Value *value, u32 level);

#endif // MISC_H
