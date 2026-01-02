#include "winx-event-to-value.h"
#include "aether/misc.h"

static Str key_code_names[] = {
  [WinxKeyCodeEscape] = STR_LIT("escape"),
  [WinxKeyCodeTab] = STR_LIT("tab"),
  [WinxKeyCodeLeftShift] = STR_LIT("left-shift"),
  [WinxKeyCodeRightShift] = STR_LIT("right-shift"),
  [WinxKeyCodeLeftControl] = STR_LIT("left-ctrl"),
  [WinxKeyCodeRightControl] = STR_LIT("right-ctrl"),
  [WinxKeyCodeLeftAlt] = STR_LIT("left-alt"),
  [WinxKeyCodeRightAlt] = STR_LIT("right-alt"),
  [WinxKeyCodeLeftSuper] = STR_LIT("left-super"),
  [WinxKeyCodeRightSuper] = STR_LIT("right-super"),
  [WinxKeyCodeMenu] = STR_LIT("menu"),
  [WinxKeyCodeNumLock] = STR_LIT("num-lock"),
  [WinxKeyCodeCapsLock] = STR_LIT("caps-lock"),
  [WinxKeyCodePrintScreen] = STR_LIT("print-screen"),
  [WinxKeyCodeScrollLock] = STR_LIT("scroll-lock"),
  [WinxKeyCodePause] = STR_LIT("pause"),
  [WinxKeyCodeDelete] = STR_LIT("delete"),
  [WinxKeyCodeBackspace] = STR_LIT("backspace"),
  [WinxKeyCodeEnter] = STR_LIT("enter"),
  [WinxKeyCodeHome] = STR_LIT("home"),
  [WinxKeyCodeEnd] = STR_LIT("end"),
  [WinxKeyCodePageUp] = STR_LIT("page-up"),
  [WinxKeyCodePageDown] = STR_LIT("page-down"),
  [WinxKeyCodeInsert] = STR_LIT("insert"),
  [WinxKeyCodeLeft] = STR_LIT("left"),
  [WinxKeyCodeRight] = STR_LIT("right"),
  [WinxKeyCodeDown] = STR_LIT("down"),
  [WinxKeyCodeUp] = STR_LIT("up"),
  [WinxKeyCodeF1] = STR_LIT("f1"),
  [WinxKeyCodeF2] = STR_LIT("f2"),
  [WinxKeyCodeF3] = STR_LIT("f3"),
  [WinxKeyCodeF4] = STR_LIT("f4"),
  [WinxKeyCodeF5] = STR_LIT("f5"),
  [WinxKeyCodeF6] = STR_LIT("f6"),
  [WinxKeyCodeF7] = STR_LIT("f7"),
  [WinxKeyCodeF8] = STR_LIT("f8"),
  [WinxKeyCodeF9] = STR_LIT("f9"),
  [WinxKeyCodeF10] = STR_LIT("f10"),
  [WinxKeyCodeF11] = STR_LIT("f11"),
  [WinxKeyCodeF12] = STR_LIT("f12"),
  [WinxKeyCodeF13] = STR_LIT("f13"),
  [WinxKeyCodeF14] = STR_LIT("f14"),
  [WinxKeyCodeF15] = STR_LIT("f15"),
  [WinxKeyCodeF16] = STR_LIT("f16"),
  [WinxKeyCodeF17] = STR_LIT("f17"),
  [WinxKeyCodeF18] = STR_LIT("f18"),
  [WinxKeyCodeF19] = STR_LIT("f19"),
  [WinxKeyCodeF20] = STR_LIT("f20"),
  [WinxKeyCodeF21] = STR_LIT("f21"),
  [WinxKeyCodeF22] = STR_LIT("f22"),
  [WinxKeyCodeF23] = STR_LIT("f23"),
  [WinxKeyCodeF24] = STR_LIT("f24"),
  [WinxKeyCodeF25] = STR_LIT("f25"),
  [WinxKeyCodeKpDivide] = STR_LIT("k/"),
  [WinxKeyCodeKpMultiply] = STR_LIT("k*"),
  [WinxKeyCodeKpSubtract] = STR_LIT("k-"),
  [WinxKeyCodeKpAdd] = STR_LIT("k+"),
  [WinxKeyCodeKp0] = STR_LIT("k0"),
  [WinxKeyCodeKp1] = STR_LIT("k1"),
  [WinxKeyCodeKp2] = STR_LIT("k2"),
  [WinxKeyCodeKp3] = STR_LIT("k3"),
  [WinxKeyCodeKp4] = STR_LIT("k4"),
  [WinxKeyCodeKp5] = STR_LIT("k5"),
  [WinxKeyCodeKp6] = STR_LIT("k6"),
  [WinxKeyCodeKp7] = STR_LIT("k7"),
  [WinxKeyCodeKp8] = STR_LIT("k8"),
  [WinxKeyCodeKp9] = STR_LIT("k9"),
  [WinxKeyCodeKpDecimal] = STR_LIT("k."),
  [WinxKeyCodeKpEqual] = STR_LIT("k="),
  [WinxKeyCodeKpEnter] = STR_LIT("kenter"),
  [WinxKeyCodeA] = STR_LIT("a"),
  [WinxKeyCodeB] = STR_LIT("b"),
  [WinxKeyCodeC] = STR_LIT("c"),
  [WinxKeyCodeD] = STR_LIT("d"),
  [WinxKeyCodeE] = STR_LIT("e"),
  [WinxKeyCodeF] = STR_LIT("f"),
  [WinxKeyCodeG] = STR_LIT("g"),
  [WinxKeyCodeH] = STR_LIT("h"),
  [WinxKeyCodeI] = STR_LIT("i"),
  [WinxKeyCodeJ] = STR_LIT("j"),
  [WinxKeyCodeK] = STR_LIT("k"),
  [WinxKeyCodeL] = STR_LIT("l"),
  [WinxKeyCodeM] = STR_LIT("m"),
  [WinxKeyCodeN] = STR_LIT("n"),
  [WinxKeyCodeO] = STR_LIT("o"),
  [WinxKeyCodeP] = STR_LIT("p"),
  [WinxKeyCodeQ] = STR_LIT("q"),
  [WinxKeyCodeR] = STR_LIT("r"),
  [WinxKeyCodeS] = STR_LIT("s"),
  [WinxKeyCodeT] = STR_LIT("t"),
  [WinxKeyCodeU] = STR_LIT("u"),
  [WinxKeyCodeV] = STR_LIT("v"),
  [WinxKeyCodeW] = STR_LIT("w"),
  [WinxKeyCodeX] = STR_LIT("x"),
  [WinxKeyCodeY] = STR_LIT("y"),
  [WinxKeyCodeZ] = STR_LIT("z"),
  [WinxKeyCode1] = STR_LIT("1"),
  [WinxKeyCode2] = STR_LIT("2"),
  [WinxKeyCode3] = STR_LIT("3"),
  [WinxKeyCode4] = STR_LIT("4"),
  [WinxKeyCode5] = STR_LIT("5"),
  [WinxKeyCode6] = STR_LIT("6"),
  [WinxKeyCode7] = STR_LIT("7"),
  [WinxKeyCode8] = STR_LIT("8"),
  [WinxKeyCode9] = STR_LIT("9"),
  [WinxKeyCode0] = STR_LIT("0"),
  [WinxKeyCodeSpace] = STR_LIT(" "),
  [WinxKeyCodeMinus] = STR_LIT("-"),
  [WinxKeyCodeEqual] = STR_LIT("="),
  [WinxKeyCodeLeftBracket] = STR_LIT("["),
  [WinxKeyCodeRightBracket] = STR_LIT("]"),
  [WinxKeyCodeBackslash] = STR_LIT("\\"),
  [WinxKeyCodeSemicolon] = STR_LIT(";"),
  [WinxKeyCodeApostrophe] = STR_LIT("`"),
  [WinxKeyCodeComma] = STR_LIT(","),
  [WinxKeyCodePeriod] = STR_LIT("."),
  [WinxKeyCodeSlash] = STR_LIT("/"),
};

static Str mouse_button_names[] = {
  [WinxMouseButtonLeft] = STR_LIT("left"),
  [WinxMouseButtonMiddle] = STR_LIT("middle"),
  [WinxMouseButtonRight] = STR_LIT("right"),
  [WinxMouseButtonWheelUp] = STR_LIT("wheel-up"),
  [WinxMouseButtonWheelDown] = STR_LIT("wheel-down"),
  [WinxMouseButtonUnknown] = STR_LIT("unknown"),
};

Value *winx_event_to_value(WinxEvent *event, Vm *vm) {
  Dict result = {0};
  Str type = {0};

  switch (event->kind) {
  case WinxEventKindNone: break;

  case WinxEventKindKeyPress: {
    type = STR_LIT("key-press");

    WinxKeyCode key_code = event->as.key.key_code;
    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("key-code"),
                           value_string(key_code_names[key_code], vm->current_frame));

    Str _char = { arena_alloc(&vm->current_frame->arena, 1), 1 };
    *_char.ptr = (char) event->as.key._char;
    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("char"),
                           value_string(_char, vm->current_frame));
  } break;

  case WinxEventKindKeyRelease: {
    type = STR_LIT("key-release");

    WinxKeyCode key_code = event->as.key.key_code;
    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("key-code"),
                           value_string(key_code_names[key_code], vm->current_frame));

    Str _char = { arena_alloc(&vm->current_frame->arena, 1), 1 };
    *_char.ptr = (char) event->as.key._char;
    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("char"),
                           value_string(_char, vm->current_frame));
  } break;

  case WinxEventKindKeyHold: {
    type = STR_LIT("key-hold");

    WinxKeyCode key_code = event->as.key.key_code;
    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("key-code"),
                           value_string(key_code_names[key_code], vm->current_frame));

    Str _char = { arena_alloc(&vm->current_frame->arena, 1), 1 };
    *_char.ptr = (char) event->as.key._char;
    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("char"),
                           value_string(_char, vm->current_frame));
  } break;

  case WinxEventKindButtonPress: {
    type = STR_LIT("button-press");

    WinxMouseButton button = event->as.button.button;
    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("button"),
                           value_string(mouse_button_names[button], vm->current_frame));
  } break;

  case WinxEventKindButtonRelease: {
    type = STR_LIT("button-release");

    WinxMouseButton button = event->as.button.button;
    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("button"),
                           value_string(mouse_button_names[button], vm->current_frame));
  } break;

  case WinxEventKindMouseMove: {
    type = STR_LIT("mouse-move");

    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("x"),
                           value_int(event->as.mouse_move.x, vm->current_frame));

    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("y"),
                           value_int(event->as.mouse_move.y, vm->current_frame));
  } break;

  case WinxEventKindFocus: {
    type = STR_LIT("focus");
  } break;

  case WinxEventKindUnfocus: {
    type = STR_LIT("unfocus");
  } break;

  case WinxEventKindResize: {
    type = STR_LIT("resize");

    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("width"),
                           value_float((f32) event->as.resize.width, vm->current_frame));

    dict_set_value_str_key(vm->current_frame, &result, STR_LIT("height"),
                           value_float((f32) event->as.resize.height, vm->current_frame));
  } break;

  case WinxEventKindQuit: {
    type = STR_LIT("quit");
  } break;
  }

  dict_set_value_str_key(vm->current_frame, &result, STR_LIT("type"),
                         value_string(type, vm->current_frame));

  return value_dict(result, vm->current_frame);
}
