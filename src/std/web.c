#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>

#include "aether/vm.h"
#include "aether/misc.h"

#define SET_CALLBACK(set_func_name, set_func, callback_func)                    \
  Value *set_func_name##_intrinsic(Vm *vm, Value **args) {                      \
    Value *name = args[0];                                                      \
    Value *callback = args[1];                                                  \
    char *name_cstr = str_to_cstr(name->as.string.str);                         \
    EventData *event_data = arena_alloc(&vm->frames->arena, sizeof(EventData)); \
    event_data->vm = vm;                                                        \
    event_data->callback = callback->as.func;                                   \
    set_func(name_cstr, event_data, false, callback_func);                      \
    free(name_cstr);                                                            \
    return value_unit(vm->current_frame);                                       \
  }

typedef struct {
  Vm        *vm;
  FuncValue *callback;
} EventData;

typedef struct {
  Vm        *vm;
  FuncValue *ok_callback;
  FuncValue *fail_callback;
} FetchData;

static char *str_to_cstr(Str str) {
  char *cstr = malloc(str.len + 1);
  memcpy(cstr, str.ptr, str.len);
  cstr[str.len] = '\0';

  return cstr;
}

Value *alert_intrinsic(Vm *vm, Value **args) {
  Value *text = args[0];

  char *text_cstr = str_to_cstr(text->as.string.str);

  EM_ASM({
    alert(UTF8ToString($0));
  }, text_cstr);

  free(text_cstr);

  return value_unit(vm->current_frame);
}

Value *update_html_intrinsic(Vm *vm, Value **args) {
  Value *name = args[0];
  Value *html = args[1];

  char *name_cstr = str_to_cstr(name->as.string.str);
  char *html_cstr = str_to_cstr(html->as.string.str);

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

  char *name_cstr = str_to_cstr(name->as.string.str);
  char *text_cstr = str_to_cstr(text->as.string.str);

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

  char *name_cstr = str_to_cstr(name->as.string.str);

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

  char *name_cstr = str_to_cstr(name->as.string.str);

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
  Dict *event_data_dict = arena_alloc(&event_data->vm->current_frame->arena, sizeof(Dict));

  u32 key_len = strlen(key_event->key);
  Str key_str = { (char *) key_event->key, key_len };
  u32 code_len = strlen(key_event->code);
  Str code_str = { (char *) key_event->code, code_len };

  Value *key = value_string(STR_LIT("key"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_string(key_str, event_data->vm->current_frame));

  key = value_string(STR_LIT("code"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_string(code_str, event_data->vm->current_frame));

  key = value_string(STR_LIT("ctrl-key"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_bool(key_event->ctrlKey, event_data->vm->current_frame));

  key = value_string(STR_LIT("shift-key"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_bool(key_event->shiftKey, event_data->vm->current_frame));

  key = value_string(STR_LIT("alt-key"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_bool(key_event->altKey, event_data->vm->current_frame));

  key = value_string(STR_LIT("meta-key"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_bool(key_event->metaKey, event_data->vm->current_frame));

  key = value_string(STR_LIT("repeat"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_bool(key_event->repeat, event_data->vm->current_frame));

  Value *event_data_value = value_dict(event_data_dict, event_data->vm->current_frame);

  DA_APPEND(event_data->vm->stack, event_data_value);
  execute_func(event_data->vm, event_data->callback, NULL);
  --event_data->vm->stack.len;

  return true;
}

bool mouse_event_callback(i32 event_type, const EmscriptenMouseEvent *mouse_event, void *data) {
  (void) event_type;

  EventData *event_data = data;
  Dict *event_data_dict = arena_alloc(&event_data->vm->current_frame->arena, sizeof(Dict));

  Value *key = value_string(STR_LIT("x"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_int(mouse_event->screenX, event_data->vm->current_frame));

  key = value_string(STR_LIT("y"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_int(mouse_event->screenY, event_data->vm->current_frame));

  key = value_string(STR_LIT("button"), event_data->vm->current_frame);
  dict_set_value(event_data->vm->current_frame, event_data_dict, key,
                 value_int(mouse_event->button, event_data->vm->current_frame));

  Value *event_data_value = value_dict(event_data_dict, event_data->vm->current_frame);

  DA_APPEND(event_data->vm->stack, event_data_value);
  execute_func(event_data->vm, event_data->callback, NULL);
  --event_data->vm->stack.len;

  return true;
}

Value *console_log_intrinsic(Vm *vm, Value **args) {
  Value *message = args[0];

  char *message_cstr = str_to_cstr(message->as.string.str);
  emscripten_console_log(message_cstr);
  free(message_cstr);

  return value_unit(vm->current_frame);
}

Value *console_warn_intrinsic(Vm *vm, Value **args) {
  Value *message = args[0];

  char *message_cstr = str_to_cstr(message->as.string.str);
  emscripten_console_warn(message_cstr);
  free(message_cstr);

  return value_unit(vm->current_frame);
}

Value *console_error_intrinsic(Vm *vm, Value **args) {
  Value *message = args[0];

  char *message_cstr = str_to_cstr(message->as.string.str);
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

void fetch_ok(emscripten_fetch_t *fetch) {
  FetchData *fetch_data = fetch->userData;

  Bytes bytes;
  bytes.len = fetch->numBytes;
  bytes.ptr = arena_alloc(&fetch_data->vm->current_frame->arena, fetch->numBytes);
  memcpy(bytes.ptr, fetch->data, fetch->numBytes);
  Value *resource = value_bytes(bytes, fetch_data->vm->current_frame);
  DA_APPEND(fetch_data->vm->stack, resource);

  execute_func(fetch_data->vm, fetch_data->ok_callback, NULL);

  --fetch_data->vm->stack.len;
  --fetch_data->vm->pending_fetches;

  emscripten_fetch_close(fetch);
}

void fetch_fail(emscripten_fetch_t *fetch) {
  FetchData *fetch_data = fetch->userData;

  Value *code = value_int(fetch->status, fetch_data->vm->current_frame);
  DA_APPEND(fetch_data->vm->stack, code);

  execute_func(fetch_data->vm, fetch_data->fail_callback, NULL);

  --fetch_data->vm->stack.len;
  --fetch_data->vm->pending_fetches;

  emscripten_fetch_close(fetch);
}

Value *fetch_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];
  Value *ok_callback = args[1];

  char *path_cstr = arena_alloc(&vm->current_frame->arena,
                                path->as.string.str.len + 1);
  memcpy(path_cstr, path->as.string.str.ptr, path->as.string.str.len);
  path_cstr[path->as.string.str.len] = '\0';

  FetchData *fetch_data = arena_alloc(&vm->frames->arena, sizeof(FetchData));
  fetch_data->vm = vm;
  fetch_data->ok_callback = ok_callback->as.func;

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.userData = fetch_data;
  attr.onsuccess = fetch_ok;
  emscripten_fetch(&attr, path_cstr);

  ++vm->pending_fetches;

  return value_unit(vm->current_frame);
}

Value *fetch_check_intrinsic(Vm *vm, Value **args) {
  Value *path = args[0];
  Value *ok_callback = args[1];
  Value *fail_callback = args[2];

  char *path_cstr = arena_alloc(&vm->current_frame->arena,
                                path->as.string.str.len + 1);
  memcpy(path_cstr, path->as.string.str.ptr, path->as.string.str.len);
  path_cstr[path->as.string.str.len] = '\0';

  FetchData *fetch_data = arena_alloc(&vm->frames->arena, sizeof(FetchData));
  fetch_data->vm = vm;
  fetch_data->ok_callback = ok_callback->as.func;
  fetch_data->fail_callback = fail_callback->as.func;

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.userData = fetch_data;
  attr.onsuccess = fetch_ok;
  attr.onerror = fetch_fail;
  emscripten_fetch(&attr, path_cstr);

  ++vm->pending_fetches;

  return value_unit(vm->current_frame);
}

Intrinsic web_intrinsics[] = {
  { STR_LIT("alert"), false, 1, { ValueKindString }, &alert_intrinsic, NULL },
  { STR_LIT("update-html"), false, 2, { ValueKindString, ValueKindString }, &update_html_intrinsic, NULL },
  { STR_LIT("update-text"), false, 2, { ValueKindString, ValueKindString }, &update_text_intrinsic, NULL },
  { STR_LIT("get-html"), true, 1, { ValueKindString }, &get_html_intrinsic, NULL },
  { STR_LIT("get-text"), true, 1, { ValueKindString }, &get_text_intrinsic, NULL },
  { STR_LIT("on-key-press"), false, 2, { ValueKindString, ValueKindFunc }, &on_key_press_intrinsic, NULL },
  { STR_LIT("on-key-down"), false, 2, { ValueKindString, ValueKindFunc }, &on_key_down_intrinsic, NULL },
  { STR_LIT("on-key-up"), false, 2, { ValueKindString, ValueKindFunc }, &on_key_up_intrinsic, NULL },
  { STR_LIT("on-click"), false, 2, { ValueKindString, ValueKindFunc }, &on_click_intrinsic, NULL },
  { STR_LIT("on-mouse-down"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_down_intrinsic, NULL },
  { STR_LIT("on-mouse-up"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_up_intrinsic, NULL },
  { STR_LIT("on-double-click"), false, 2, { ValueKindString, ValueKindFunc }, &on_double_click_intrinsic, NULL },
  { STR_LIT("on-mouse-move"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_move_intrinsic, NULL },
  { STR_LIT("on-mouse-enter"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_enter_intrinsic, NULL },
  { STR_LIT("on-mouse-leave"), false, 2, { ValueKindString, ValueKindFunc }, &on_mouse_leave_intrinsic, NULL },
  { STR_LIT("console-log"), false, 1, { ValueKindString }, &console_log_intrinsic, NULL },
  { STR_LIT("console-warn"), false, 1, { ValueKindString }, &console_warn_intrinsic, NULL },
  { STR_LIT("console-error"), false, 1, { ValueKindString }, &console_error_intrinsic, NULL },
  { STR_LIT("fetch"), false, 2, { ValueKindString, ValueKindFunc }, &fetch_intrinsic, NULL },
  { STR_LIT("fetch-check"), false, 3,
    { ValueKindString, ValueKindFunc, ValueKindFunc },
    &fetch_check_intrinsic, NULL },
};

u32 web_intrinsics_len = ARRAY_LEN(web_intrinsics);
