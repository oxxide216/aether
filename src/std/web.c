#include <emscripten.h>
#include <emscripten/html5.h>

#include "aether/vm.h"
#include "aether/misc.h"

#define SET_CALLBACK(set_func_name, set_func, callback_func)                    \
  Value *set_func_name##_intrinsic(Vm *vm, Value **args) {                      \
    Value *name = args[0];                                                      \
    Value *callback = args[1];                                                  \
    char *name_cstr = str_to_cstr(name->as.string);                             \
    EventData *event_data = arena_alloc(&vm->frames->arena, sizeof(EventData)); \
    event_data->vm = vm;                                                        \
    event_data->callback = callback->as.func;                                   \
    set_func(name_cstr, event_data, false, callback_func);                      \
    free(name_cstr);                                                            \
    return value_unit(vm->current_frame);                                       \
  }

typedef struct {
  Vm   *vm;
  Func  callback;
} EventData;

static char *str_to_cstr(Str str) {
  char *cstr = malloc(str.len + 1);
  memcpy(cstr, str.ptr, str.len);
  cstr[str.len] = '\0';

  return cstr;
}

Value *alert_intrinsic(Vm *vm, Value **args) {
  Value *text = args[0];

  char *text_cstr = str_to_cstr(text->as.string);

  EM_ASM({
    alert(UTF8ToString($0));
  }, text_cstr);

  free(text_cstr);

  return value_unit(vm->current_frame);
}

Value *update_html_intrinsic(Vm *vm, Value **args) {
  Value *name = args[0];
  Value *html = args[1];

  char *name_cstr = str_to_cstr(name->as.string);
  char *html_cstr = str_to_cstr(html->as.string);

  EM_ASM({
    const element = document.querySelector(UTF8ToString($0));
    element.innerHTML = UTF8ToString($1);
  }, name_cstr, html_cstr);

  free(name_cstr);
  free(html_cstr);

  return value_unit(vm->current_frame);
}

Value *update_text_intrinsic(Vm *vm, Value **args) {
  Value *name = args[0];
  Value *text = args[1];

  char *name_cstr = str_to_cstr(name->as.string);
  char *text_cstr = str_to_cstr(text->as.string);

  EM_ASM({
    const element = document.querySelector(UTF8ToString($0));
    element.textContent = UTF8ToString($1);
  }, name_cstr, text_cstr);

  free(name_cstr);
  free(text_cstr);

  return value_unit(vm->current_frame);
}

Value *get_html_intrinsic(Vm *vm, Value **args) {
  Value *name = args[0];

  char *name_cstr = str_to_cstr(name->as.string);

  char *html = EM_ASM_PTR({
    const element = document.querySelector(UTF8ToString($0));
    return stringToNewUTF8(element.innerHTML);
  }, name_cstr);

  u32 html_len = strlen(html);
  char *html_in_arena = arena_alloc(&vm->current_frame->arena, html_len);
  memcpy(html_in_arena, html, html_len);

  free(name_cstr);
  free(html);

  return value_string(STR(html_in_arena, html_len), vm->current_frame);
}

Value *get_text_intrinsic(Vm *vm, Value **args) {
  Value *name = args[0];

  char *name_cstr = str_to_cstr(name->as.string);

  char *text = EM_ASM_PTR({
    const element = document.querySelector(UTF8ToString($0));
    return stringToNewUTF8(element.textContent);
  }, name_cstr);

  u32 text_len = strlen(text);
  char *text_in_arena = arena_alloc(&vm->current_frame->arena, text_len);
  memcpy(text_in_arena, text, text_len);

  free(name_cstr);
  free(text);

  return value_string(STR(text_in_arena, text_len), vm->current_frame);
}

bool key_event_callback(i32 event_type, const EmscriptenKeyboardEvent *key_event, void *data) {
  (void) event_type;

  EventData *event_data = data;
  Dict event_data_dict = {0};

  u32 key_len = strlen(key_event->key);
  Str key_str = { (char *) key_event->key, key_len };
  u32 code_len = strlen(key_event->code);
  Str code_str = { (char *) key_event->code, code_len };

  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("key"),
                          value_string(key_str, event_data->vm->current_frame));
  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("code"),
                          value_string(code_str, event_data->vm->current_frame));
  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("ctrl-key"),
                          value_bool(key_event->ctrlKey, event_data->vm->current_frame));
  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("shift-key"),
                          value_bool(key_event->shiftKey, event_data->vm->current_frame));
  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("alt-key"),
                          value_bool(key_event->altKey, event_data->vm->current_frame));
  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("meta-key"),
                          value_bool(key_event->metaKey, event_data->vm->current_frame));
  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("repeat"),
                          value_bool(key_event->repeat, event_data->vm->current_frame));

  Value *event_data_value = value_dict(event_data_dict, event_data->vm->current_frame);

  Value *args[] = { event_data_value };
  execute_func(event_data->vm, args, &event_data->callback, NULL, false);

  return true;
}

bool mouse_event_callback(i32 event_type, const EmscriptenMouseEvent *mouse_event, void *data) {
  (void) event_type;

  EventData *event_data = data;
  Dict event_data_dict = {0};

  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("x"),
                          value_int(mouse_event->screenX, event_data->vm->current_frame));
  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("y"),
                          value_int(mouse_event->screenY, event_data->vm->current_frame));
  dict_push_value_str_key(event_data->vm->current_frame, &event_data_dict, STR_LIT("button"),
                          value_int(mouse_event->button, event_data->vm->current_frame));

  Value *event_data_value = value_dict(event_data_dict, event_data->vm->current_frame);

  Value *args[] = { event_data_value };
  execute_func(event_data->vm, args, &event_data->callback, NULL, false);

  return true;
}

Value *console_log_intrinsic(Vm *vm, Value **args) {
  Value *message = args[0];

  char *message_cstr = str_to_cstr(message->as.string);
  emscripten_console_log(message_cstr);
  free(message_cstr);

  return value_unit(vm->current_frame);
}

Value *console_warn_intrinsic(Vm *vm, Value **args) {
  Value *message = args[0];

  char *message_cstr = str_to_cstr(message->as.string);
  emscripten_console_warn(message_cstr);
  free(message_cstr);

  return value_unit(vm->current_frame);
}

Value *console_error_intrinsic(Vm *vm, Value **args) {
  Value *message = args[0];

  char *message_cstr = str_to_cstr(message->as.string);
  emscripten_console_error(message_cstr);
  free(message_cstr);

  return value_unit(vm->current_frame);
}

SET_CALLBACK(on_key_press, emscripten_set_keypress_callback, key_event_callback);
SET_CALLBACK(on_key_down, emscripten_set_keydown_callback, key_event_callback);
SET_CALLBACK(on_key_up, emscripten_set_keyup_callback, key_event_callback);
SET_CALLBACK(on_click, emscripten_set_click_callback, mouse_event_callback);
SET_CALLBACK(on_mouse_down, emscripten_set_mousedown_callback, mouse_event_callback);
SET_CALLBACK(on_mouse_up, emscripten_set_mouseup_callback, mouse_event_callback);
SET_CALLBACK(on_double_click, emscripten_set_dblclick_callback, mouse_event_callback);
SET_CALLBACK(on_mouse_move, emscripten_set_mousemove_callback, mouse_event_callback);
SET_CALLBACK(on_mouse_enter, emscripten_set_mouseenter_callback, mouse_event_callback);
SET_CALLBACK(on_mouse_leave, emscripten_set_mouseleave_callback, mouse_event_callback);

Intrinsic web_intrinsics[] = {
  { STR_LIT("alert"), false, 1, { ValueKindString }, &alert_intrinsic },
  { STR_LIT("update-html"), false, 2, { ValueKindString, ValueKindString }, &update_html_intrinsic },
  { STR_LIT("update-text"), false, 2, { ValueKindString, ValueKindString }, &update_text_intrinsic },
  { STR_LIT("get-html"), true, 1, { ValueKindString }, &get_html_intrinsic },
  { STR_LIT("get-text"), true, 1, { ValueKindString }, &get_text_intrinsic },
  { STR_LIT("on-key-press"), false, 2, { ValueKindString, ValueKindFunc }, &on_key_press_intrinsic },
  { STR_LIT("on-key-down"), false, 2, { ValueKindString, ValueKindFunc }, &on_key_down_intrinsic },
  { STR_LIT("on-key-up"), false, 2, { ValueKindString, ValueKindFunc }, &on_key_up_intrinsic },
  { STR_LIT("on-click"), false, 2, { ValueKindString, ValueKindFunc }, &on_click_intrinsic },
  { STR_LIT("on-mouse-down"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_down_intrinsic },
  { STR_LIT("on-mouse-up"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_up_intrinsic },
  { STR_LIT("on-double-click"), false, 2, { ValueKindString, ValueKindFunc }, &on_double_click_intrinsic },
  { STR_LIT("on-mouse-move"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_move_intrinsic },
  { STR_LIT("on-mouse-enter"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_enter_intrinsic },
  { STR_LIT("on-mouse-leave"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_leave_intrinsic },
  { STR_LIT("console-log"), false, 1, { ValueKindString }, &console_log_intrinsic },
  { STR_LIT("console-warn"), false, 1, { ValueKindString }, &console_warn_intrinsic },
  { STR_LIT("console-error"), false, 1, { ValueKindString }, &console_error_intrinsic },
};

u32 web_intrinsics_len = ARRAY_LEN(web_intrinsics);
