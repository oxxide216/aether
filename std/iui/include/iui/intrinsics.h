#ifndef IUI_H
#define IUI_H

#include "../src/widgets.h"
#include "../src/renderer.h"
#include "aether/vm.h"
#include "winx/winx.h"
#include "winx/event.h"
#include "glass/glass.h"
#include "glass/math.h"

typedef Da(WinxEvent) IuiEvents;

typedef struct {
  Winx        winx;
  WinxWindow  window;
  IuiEvents   events;
  IuiWidgets  widgets;
  IuiRenderer renderer;
} Iui;

extern Intrinsic iui_intrinsics[];
extern u32 iui_intrinsics_len;

#endif // IUI_H
