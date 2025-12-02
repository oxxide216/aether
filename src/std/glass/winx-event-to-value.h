#ifndef AETHER_GLASS_WINX_EVENT_TO_VALUE_H
#define AETHER_GLASS_WINX_EVENT_TO_VALUE_H

#include "aether/vm.h"
#include "winx/event.h"

Value *winx_event_to_value(WinxEvent *event, Vm *vm);

#endif // AETHER_GLASS_WINX_EVENT_TO_VALUE_H
