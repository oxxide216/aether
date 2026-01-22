#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "aether/bytecode.h"

void eliminate_dead_code_instrs(Instrs *instrs);
void eliminate_dead_code(Ir *ir);

#endif // OPTIMIZER_H
