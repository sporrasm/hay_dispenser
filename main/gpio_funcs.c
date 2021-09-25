#include "gpio_funcs.h"

void write_lock_idx_isr(void* arg) {
  /*Implement lock idx write routine*/
  return;
}

uint8_t lock_pin_init() {
  gpio_config_t io_conf;
  io_conf.intr_type=GPIO_INTR_DISABLE;
  io_conf.mode=GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask=GPIO_OUTPUT_PIN_SEL;
  io_conf.pull_down_en=0;
  io_conf.pull_up_en=0;
  if (gpio_config(&io_conf) == ESP_ERR_INVALID_ARG) {
    return 1;
  }
  else {
    return 0;
  }
}

uint8_t dpd_pin_init() {
  gpio_config_t io_conf;
  io_conf.intr_type=GPIO_INTR_NEGEDGE;
  io_conf.mode=GPIO_MODE_INPUT;
  io_conf.pin_bit_mask=DPD_GPIO_SEL;
  io_conf.pull_down_en=0;
  io_conf.pull_up_en=1;
  if (gpio_config(&io_conf) == ESP_ERR_INVALID_ARG) {
    return 1;
  }
  if (gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT) != ESP_OK)
    
}

