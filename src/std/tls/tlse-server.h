#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpragmas"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunknown-warning-option"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-parameter"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wsign-compare"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wtype-limits"
#endif
#ifdef __clang__
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wsometimes-uninitialized"
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#endif
#define i32 _i32
#define i64 _i64
#define u32 _u32
#define u64 _u64
#include "tlse/tlse.c"
#undef u64
#undef u32
#undef i64
#undef i32
#ifdef __GNUC__
  #pragma GCC diagnostic pop
  #pragma GCC diagnostic pop
  #pragma GCC diagnostic pop
  #pragma GCC diagnostic pop
  #pragma GCC diagnostic pop
  #pragma GCC diagnostic pop
  #pragma GCC diagnostic pop
  #pragma GCC diagnostic pop
#endif
#ifdef __clang__
  #pragma clang diagnostic pop
  #pragma clang diagnostic pop
#endif
