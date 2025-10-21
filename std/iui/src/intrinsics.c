#include "iui/intrinsics.h"
#include "renderer.h"
#include "winx/event.h"
#include "shl_log.h"

static Iui iui = {0};

static bool process_event(IuiEvents *events, WinxEvent *event) {
  if (event->kind == WinxEventKindQuit) {
    return false;
  } else if (event->kind == WinxEventKindResize) {
    glass_resize(event->as.resize.width,
                 event->as.resize.height);
    iui_renderer_resize(&iui.renderer,
                      event->as.resize.width,
                      event->as.resize.height);
  } else {
    DA_APPEND(*events, *event);
  }

  return true;
}

static void init_styles(Iui *iui) {
  IuiStyle box_style = {
    STR_LIT(":box:"),
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(0.0, 0.0, 0.0, 0.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(0.0, 0.0, 0.0, 0.0),
  };
  DA_APPEND(iui->widgets.styles, box_style);

  IuiStyle button_style = {
    STR_LIT(":button:"),
    vec4(0.0, 0.0, 0.0, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(0.0, 0.0, 0.0, 1.0),
    vec4(0.5, 0.5, 0.5, 1.0),
  };
  DA_APPEND(iui->widgets.styles, button_style);

  IuiStyle text_style = {
    STR_LIT(":text:"),
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(0.0, 0.0, 0.0, 0.0),
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(0.0, 0.0, 0.0, 0.0),
  };
  DA_APPEND(iui->widgets.styles, text_style);

  IuiStyle input_style = {
    STR_LIT(":input:"),
    vec4(1.0, 1.0, 1.0, 1.0),
    vec4(0.0, 0.0, 0.0, 0.5),
    vec4(1.0, 1.0, 1.0, 0.5),
    vec4(0.0, 0.0, 0.0, 0.5),
  };
  DA_APPEND(iui->widgets.styles, input_style);
}

bool iui_main_loop_intrinsic(Vm *vm) {
  Value body = value_stack_pop(&vm->stack);
  Value height = value_stack_pop(&vm->stack);
  Value width = value_stack_pop(&vm->stack);
  Value title = value_stack_pop(&vm->stack);

  iui.winx = winx_init();
  iui.window = winx_init_window(&iui.winx, title.as.string.str,
                                width.as._int, height.as._int,
                                WinxGraphicsModeOpenGL,
                                NULL);
  iui.renderer = iui_init_renderer();

  init_styles(&iui);

  bool is_running = true;
  while (is_running) {
    iui.events.len = 0;

    WinxEvent event;
    while ((event = winx_get_event(&iui.window, false)).kind != WinxEventKindNone) {
      is_running = process_event(&iui.events, &event);
      if (!is_running)
        break;
    }

    Dict window_size = {0};

    NamedValue width = {
      STR_LIT("x"),
      {
        ValueKindFloat,
        { ._float = iui.window.width },
      }
    };
    DA_APPEND(window_size, width);

    NamedValue height = {
      STR_LIT("y"),
      {
        ValueKindFloat,
        { ._float =  iui.window.height },
      }
    };
    DA_APPEND(window_size, height);

    value_stack_push_dict(&vm->stack, window_size);

    EXECUTE_FUNC(vm, &body.as.func, false);

    Vec4 bounds = { 0.0, 0.0, iui.window.width, iui.window.height };
    iui_widgets_recompute_layout(&iui.widgets, bounds);
    iui_renderer_render_widgets(&iui.renderer, &iui.widgets);
    iui.widgets.is_dirty = false;

    winx_draw(&iui.window);
  }

  return true;
}

bool iui_vbox_intrinsic(Vm *vm) {
  Value body = value_stack_pop(&vm->stack);
  Value spacing = value_stack_pop(&vm->stack);
  Value margin_y = value_stack_pop(&vm->stack);
  Value margin_x = value_stack_pop(&vm->stack);

  iui_widgets_push_box_begin(&iui.widgets, vec2(margin_x.as._float, margin_y.as._float),
                             spacing.as._float, IuiBoxDirectionVertical);

  EXECUTE_FUNC(vm, &body.as.func, false);

  iui_widgets_push_box_end(&iui.widgets);

  return true;
}

bool iui_hbox_intrinsic(Vm *vm) {
  Value body = value_stack_pop(&vm->stack);
  Value spacing = value_stack_pop(&vm->stack);
  Value margin_y = value_stack_pop(&vm->stack);
  Value margin_x = value_stack_pop(&vm->stack);

  iui_widgets_push_box_begin(&iui.widgets, vec2(margin_x.as._float, margin_y.as._float),
                             spacing.as._float, IuiBoxDirectionHorizontal);

  EXECUTE_FUNC(vm, &body.as.func, false);

  iui_widgets_push_box_end(&iui.widgets);

  return true;
}

bool iui_button_intrinsic(Vm *vm) {
  Value on_click = value_stack_pop(&vm->stack);
  Value text = value_stack_pop(&vm->stack);

  IuiWidget *button = iui_widgets_push_button(&iui.widgets,
                                              text.as.string.str,
                                              on_click.as.func);

  for (u32 i = 0; i < iui.events.len; ++i) {
    WinxEvent *event = iui.events.items + i;
    if (event->kind == WinxEventKindButtonPress ||
        event->kind == WinxEventKindButtonRelease) {
      WinxEventButton *button_event = &event->as.button;

      if (event->kind == WinxEventKindButtonRelease)
        button->as.button.pressed = false;

      if (button_event->x >= button->bounds.x &&
          button_event->y >= button->bounds.y &&
          button_event->x <= button->bounds.x + button->bounds.z &&
          button_event->y <= button->bounds.y + button->bounds.w) {
        if (event->kind == WinxEventKindButtonPress)
          button->as.button.pressed = true;
        else
          EXECUTE_FUNC(vm, &on_click.as.func, false);
        break;
      }
    }
  }

  return true;
}

bool iui_text_intrinsic(Vm *vm) {
  Value left_padding = value_stack_pop(&vm->stack);
  Value center_y = value_stack_pop(&vm->stack);
  Value center_x = value_stack_pop(&vm->stack);
  Value text = value_stack_pop(&vm->stack);

  iui_widgets_push_text(&iui.widgets, text.as.string.str,
                        center_x.as._bool, center_y.as._bool,
                        left_padding.as._float);

  return true;
}

bool iui_input_intrinsic(Vm *vm) {
  Value on_submit = value_stack_pop(&vm->stack);
  Value left_padding = value_stack_pop(&vm->stack);
  Value placeholder = value_stack_pop(&vm->stack);

  IuiWidget *input = iui_widgets_push_input(&iui.widgets, placeholder.as.string.str,
                                            left_padding.as._float, on_submit.as.func);

  for (u32 i = 0; i < iui.events.len; ++i) {
    WinxEvent *event = iui.events.items + i;
    if (event->kind == WinxEventKindButtonPress) {
      WinxEventButton *button_event = &event->as.button;

      if (button_event->x >= input->bounds.x &&
          button_event->y >= input->bounds.y &&
          button_event->x <= input->bounds.x + input->bounds.z &&
          button_event->y <= input->bounds.y + input->bounds.w)
        input->as.input.focused = true;
      else
        input->as.input.focused = false;
    } else if (input->as.input.focused &&
               (event->kind == WinxEventKindKeyPress ||
                event->kind == WinxEventKindKeyHold)) {
      WinxEventKey *key_event = &event->as.key;

      switch (key_event->key_code) {
      case WinxKeyCodeEnter: {
        input->as.input.focused = false;

        Str content = {
          input->as.input.buffer.items,
          input->as.input.buffer.len,
        };
        value_stack_push_string(&vm->stack, content);

        EXECUTE_FUNC(vm, &on_submit.as.func, false);
      } break;

      case WinxKeyCodeBackspace: {
        if (input->as.input.cursor_pos > 0) {
          --input->as.input.cursor_pos;
          DA_REMOVE_AT(input->as.input.buffer, input->as.input.cursor_pos);
        }
      } break;

      case WinxKeyCodeDelete: {
        if (input->as.input.cursor_pos < input->as.input.buffer.len)
          DA_REMOVE_AT(input->as.input.buffer, input->as.input.cursor_pos);
      } break;

      case WinxKeyCodeLeft: {
        if (input->as.input.cursor_pos > 0)
          --input->as.input.cursor_pos;
      } break;

      case WinxKeyCodeRight: {
        if (input->as.input.cursor_pos < input->as.input.buffer.len)
          ++input->as.input.cursor_pos;
      } break;

      case WinxKeyCodeUp: {
        input->as.input.cursor_pos = 0;
      } break;

      case WinxKeyCodeDown: {
        input->as.input.cursor_pos = input->as.input.buffer.len;
      } break;

      default: {
        DA_INSERT(input->as.input.buffer,
                  input->as.input.cursor_pos,
                  key_event->_char);

        ++input->as.input.cursor_pos;
      } break;
      }
    }
  }

  return true;
}

bool iui_abs_bounds_intrinsic(Vm *vm) {
  Value height = value_stack_pop(&vm->stack);
  Value width = value_stack_pop(&vm->stack);
  Value y = value_stack_pop(&vm->stack);
  Value x = value_stack_pop(&vm->stack);

  Vec4 bounds = {
    x.as._float,
    y.as._float,
    width.as._float,
    height.as._float,
  };
  iui_widgets_abs_bounds(&iui.widgets, bounds);

  return true;
}

Intrinsic iui_intrinsics[] = {
  { STR_LIT("iui-main-loop"), false, 4,
    { ValueKindString, ValueKindInt, ValueKindInt, ValueKindFunc },
    &iui_main_loop_intrinsic },
  { STR_LIT("iui-vbox"), false, 4,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFunc },
    &iui_vbox_intrinsic },
  { STR_LIT("iui-hbox"), false, 4,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFunc },
    &iui_hbox_intrinsic },
  { STR_LIT("iui-button"), false, 2,
    { ValueKindString, ValueKindFunc },
    &iui_button_intrinsic },
  { STR_LIT("iui-text"), false, 4,
    { ValueKindString, ValueKindBool, ValueKindBool, ValueKindFloat },
    &iui_text_intrinsic },
  { STR_LIT("iui-input"), false, 3,
    { ValueKindString, ValueKindFloat, ValueKindFunc },
    &iui_input_intrinsic },
  { STR_LIT("iui-abs-bounds"), false, 4,
    { ValueKindFloat, ValueKindFloat, ValueKindFloat, ValueKindFloat },
    &iui_abs_bounds_intrinsic },
};

u32 iui_intrinsics_len = ARRAY_LEN(iui_intrinsics);
