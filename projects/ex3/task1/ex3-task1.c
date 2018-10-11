/*
 * GPIO_Buttons.c
 */

#include "zynq_registers.h"

int main() {
	// Set inputs - Buttons BTN0..3 - GPIO Bank 2 bits [4:7]
	GPIO_DIRM_2 &= ~0xF0; // Clear "Direction Mode" (0 = Input)

	// Set outputs - LEDS LD0..3 - GPIO Bank 2 bits [11:8]
	GPIO_DIRM_2 |= 0xF00; // Set "Direction Mode" (1 = Output)
	GPIO_OEN_2 |= 0xF00; // Set "Output Enable" (1 = Enabled)

	while(1) {
		// Use bitmasking and bitshifting to copy inputs to outputs
		GPIO_DATA_2 = (GPIO_DATA_2_RO & 0xF0)<<4;
	}
	return 0;
}
