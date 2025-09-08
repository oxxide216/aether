#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "air.h"
#include "vm.h"

Value print_intrinsic(Vm *vm, IrBlock *args);
Value println_intrinsic(Vm *vm, IrBlock *args);
Value input_intrinsic(Vm *vm, IrBlock *args);
Value get_args_intrinsic(Vm *vm, IrBlock *args);
Value head_intrinsic(Vm *vm, IrBlock *args);
Value tail_intrinsic(Vm *vm, IrBlock *args);
Value is_empty_intrinsic(Vm *vm, IrBlock *args);

Value add_intrinsic(Vm *vm, IrBlock *args);
Value sub_intrinsic(Vm *vm, IrBlock *args);
Value mul_intrinsic(Vm *vm, IrBlock *args);
Value div_intrinsic(Vm *vm, IrBlock *args);
Value mod_intrinsic(Vm *vm, IrBlock *args);

Value eq_intrinsic(Vm *vm, IrBlock *args);
Value ne_intrinsic(Vm *vm, IrBlock *args);
Value ls_intrinsic(Vm *vm, IrBlock *args);
Value le_intrinsic(Vm *vm, IrBlock *args);
Value gt_intrinsic(Vm *vm, IrBlock *args);
Value ge_intrinsic(Vm *vm, IrBlock *args);
Value not_intrinsic(Vm *vm, IrBlock *args);

#endif // INTRINSICS_H
