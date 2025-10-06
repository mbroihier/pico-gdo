
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "btstack_run_loop.h"

static int const LED = CYW43_WL_GPIO_LED_PIN;
int btstack_main(int argc, const char * argv[]);
int main() {
  stdio_init_all();
  cyw43_arch_init();
  cyw43_arch_gpio_put(LED, 0);
  btstack_main(0, NULL);
  btstack_run_loop_execute();}
