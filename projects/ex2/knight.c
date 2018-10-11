/*
 * main.c
 *
 */

#include "zynq_registers.h"

const uint32_t led_bp = 8; // LED array bit position

void usleep(uint32_t usecs) {
  for(volatile uint32_t i = 0; i < usecs; i++);
}

int main() {
	// Set LEDs as output
	GPIO_DIRM_2 |= 0xF00;
	GPIO_OEN_2 |= 0xF00;

	uint32_t leds = 1;
	uint32_t dir = 1;
	while(1) {
		GPIO_DATA_2 = (leds & 0xF) << led_bp;

		if (dir) leds <<= 1;
		else 	 leds >>= 1;

		if (leds == 0x8 || leds == 0x1) dir = !dir;

		usleep(1000000);
	}
	return 0;
}
