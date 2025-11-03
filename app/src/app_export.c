// C-compatible wrappers for app's setup() and loop()
// Kernel calls these from C code, they forward to C++ functions

#ifdef __cplusplus
extern "C" {
#endif
void app_setup(void);
void app_loop(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern void setup(void);
extern void loop(void);
#else
void setup(void);
void loop(void);
#endif

#ifdef __cplusplus
extern "C" void app_setup(void) {
  setup();
}
extern "C" void app_loop(void) {
  loop();
}
#else
void app_setup(void) {
  setup();
}
void app_loop(void) {
  loop();
}
#endif
