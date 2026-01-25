#include "aether/optimizer.h"
#include "shl/shl-log.h"

typedef struct {
  Str name;
  u32 body_index;
} FuncDef;

typedef Da(FuncDef) FuncDefs;

void eliminate_dead_code_instrs(Instrs *instrs, Ir *ir,
                                FuncDefs *defs,
                                FuncDefs *global_defs) {
  for (u32 i = 0; i < instrs->len; ++i) {
    Instr *instr = instrs->items + i;

    switch (instr->kind) {
    case InstrKindString: break;
    case InstrKindInt: break;
    case InstrKindFloat: break;
    case InstrKindBytes: break;

    case InstrKindFunc: {
      u32 func_index = instr->as.func.body_index;

      if (ir->items[func_index].is_dead) {
        ir->items[func_index].is_dead = false;

        FuncDefs temp_defs = {0};
        eliminate_dead_code_instrs(&ir->items[func_index].instrs,
                                   ir, &temp_defs, global_defs);
        if (temp_defs.items)
          free(temp_defs.items);
      }
    } break;

    case InstrKindFuncCall:  break;

    case InstrKindDefVar: {
      Instr *last_instr = instrs->items + i - 1;
      if (last_instr->kind == InstrKindFunc) {
        FuncDef new_def = {
          instr->as.def_var.name,
          last_instr->as.func.body_index,
        };
        DA_APPEND(*defs, new_def);
      }
    } break;

    case InstrKindGetVar: {
      bool found = false;

      for (u32 j = defs->len; j > 0; --j) {
        if (str_eq(instr->as.get_var.name, defs->items[j - 1].name)) {
          u32 func_index = defs->items[j - 1].body_index;

          if (ir->items[func_index].is_dead) {
            ir->items[func_index].is_dead = false;

            FuncDefs temp_defs = {0};
            eliminate_dead_code_instrs(&ir->items[func_index].instrs,
                                       ir, &temp_defs, global_defs);
            if (temp_defs.items)
              free(temp_defs.items);
          }

          break;
        }
      }

      if (found || global_defs == defs)
        break;

      for (u32 j = global_defs->len; j > 0; --j) {
        if (str_eq(instr->as.get_var.name, global_defs->items[j - 1].name)) {
          u32 func_index = global_defs->items[j - 1].body_index;

          if (ir->items[func_index].is_dead) {
            ir->items[func_index].is_dead = false;

            FuncDefs temp_defs = {0};
            eliminate_dead_code_instrs(&ir->items[func_index].instrs,
                                       ir, &temp_defs, global_defs);
            if (temp_defs.items)
              free(temp_defs.items);
          }

          break;
        }
      }
    } break;

    case InstrKindJump:        break;
    case InstrKindCondJump:    break;
    case InstrKindCondNotJump: break;
    case InstrKindLabel:       break;
    case InstrKindMatchBegin:  break;
    case InstrKindMatchCase:   break;
    case InstrKindMatchEnd:    break;
    case InstrKindGet:         break;
    case InstrKindSet:         break;
    case InstrKindRet:         break;
    case InstrKindList:        break;
    case InstrKindDict:        break;
    case InstrKindSelf:        break;
    }
  }
}

void eliminate_dead_code(Ir *ir) {
  FuncDefs global_defs = {0};

  for (u32 i = 1; i < ir->len; ++i)
    ir->items[i].is_dead = true;

  eliminate_dead_code_instrs(&ir->items[0].instrs, ir, &global_defs, &global_defs);

  if (global_defs.items)
    free(global_defs.items);
}
