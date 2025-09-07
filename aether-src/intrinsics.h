#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "air.h"
#include "vm.h"

Value print_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value println_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value get_args_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value head_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value tail_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value is_empty_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);

Value add_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value sub_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value mul_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value div_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value mod_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);

Value eq_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value ne_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value ls_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value le_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value gt_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value ge_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);
Value not_intrinsic(Vm *vm, IrBlock *args, bool is_inside_of_func);

#endif // INTRINSICS_H
