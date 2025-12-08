#ifndef INTRINSICS_H
#define INTRINSICS_H

#define MODULE(name)                    \
  extern Intrinsic name##_intrinsics[]; \
  extern u32 name##_intrinsics_len;

MODULE(core);
MODULE(math);
MODULE(str);
#ifndef NOSYSTEM
MODULE(base);
MODULE(io);
MODULE(path);
MODULE(net);
MODULE(term);
MODULE(system);
#endif
#ifdef GLASS
MODULE(glass);
#endif
#ifdef EMSCRIPTEN
MODULE(web);
#endif

#endif // INTRINSICS_H
