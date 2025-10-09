
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "btstack_run_loop.h"
#include "lock.h"
extern lock * lock_ptr;
int btstack_main();
int main() {
  int parms[] = {1807, 45289, 214326};
  lock lock_object(parms);
  lock_ptr = &lock_object;
  stdio_init_all();
  cyw43_arch_init();
  btstack_main();
  btstack_run_loop_execute();
}
