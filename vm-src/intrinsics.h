#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "ir.h"
#include "vm.h"

void print_intrinsic(Vm *vm);
void println_intrinsic(Vm *vm);
void input_intrinsic(Vm *vm);
void get_args_intrinsic(Vm *vm);
void head_intrinsic(Vm *vm);
void tail_intrinsic(Vm *vm);
void is_empty_intrinsic(Vm *vm);

void str_to_num_intrinsic(Vm *vm);
void num_to_str_intrinsic(Vm *vm);
void bool_to_str_intrinsic(Vm *vm);
void bool_to_num_intrinsic(Vm *vm);

void add_intrinsic(Vm *vm);
void sub_intrinsic(Vm *vm);
void mul_intrinsic(Vm *vm);
void div_intrinsic(Vm *vm);
void mod_intrinsic(Vm *vm);

void eq_intrinsic(Vm *vm);
void ne_intrinsic(Vm *vm);
void ls_intrinsic(Vm *vm);
void le_intrinsic(Vm *vm);
void gt_intrinsic(Vm *vm);
void ge_intrinsic(Vm *vm);
void not_intrinsic(Vm *vm);

extern Intrinsic intrinsics[];
extern u32 intrinsics_len;

#endif // INTRINSICS_H
