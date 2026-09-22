#include "stm32f4xx.h"

#define LED_PORT    GPIOD

void delay(volatile uint32_t count) {
  while (count--);
}

int main ()