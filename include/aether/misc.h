#ifndef MISC_H
#define MISC_H

#include "aether/vm.h"

bool    value_to_bool(Value *value);
Value **dict_get_value_root(Dict *dict, Value *key);
Value  *dict_get_value(Dict *dict, Value *key);
void    dict_set_value(StackFrame *frame, Dict *dict,
                       Value *key, Value *value);
Value  **get_child_root(Value *value, Value *key, InstrMeta *meta, Vm *vm);
void    sb_push_value(StringBuilder *sb, Value *value,
                      u32 level, bool kind, bool quote_string);
void    print_instr(Instr *instr, bool hide_strings);

#endif // MISC_H
