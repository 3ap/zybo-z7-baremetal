/*
 * blink.c
*/
#include <stdint.h>
#include "zynq_registers.h"

void sleep(uint32_t secs) {
  for(volatile uint32_t i = 0; i < 5000000 * secs; i++);
}

// Defines hardware registers
int main(void) {
  GPIO_DIRM_2 = 0xF00;         // Set direction for bits 8 - 11 to output
  GPIO_OEN_2 = 0xF00;          // Enable output for bits 8 - 11
  while (1) {                  // Loop forever
    GPIO_DATA_2 = 0xF00;       // Turn on (high) the LEDs
    sleep(1);                  // Wait for 1s
    GPIO_DATA_2 = 0x000;       // Turn off (low) the LEDs
    sleep(1);                  // Wait again
  }
  return 0;
}
