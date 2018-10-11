/*
 * main.c
 *
 */

#include "zynq_registers.h"

#define READ_DIR_SW GPIO_DATA_2 & 0x1
#define READ_INV_SW GPIO_DATA_2 & 0x2
#define READ_BUTTONS ((GPIO_DATA_2_RO & 0xF0) >> 4) // 0..15

const uint8_t led_bp = 8; // LED array bit position

void usleep(uint32_t usecs) {
  for(volatile uint32_t i = 0; i < usecs; i++);
}

int main() {
	// Set buttons and two switches as input.
	GPIO_DIRM_2 &= ~0xF3;
	// Set LEDs as output
	GPIO_DIRM_2 |= 0xF00;
	GPIO_OEN_2 |= 0xF00;

	uint8_t leds = 1;
	while(1) {
		if (READ_DIR_SW) {
			// Check if 4 bit led is about to overflow
			if (leds == 0x8) leds = 1;
			else leds <<= 1;
		} else {
			// Check if 4 bit led is about to underflow
			if (leds == 0x1) leds = 0x8;
			else leds >>= 1;
		}
		if (READ_INV_SW) {
			GPIO_DATA_2 = (~leds & 0xF) << led_bp;
		} else {
			GPIO_DATA_2 = (leds & 0xF) << led_bp;
		}
		usleep(1000000 - READ_BUTTONS * 66000);
	}
	return 0;
}
