/*
 * Memory_map.c
 *
 */

#include <stdint.h> // For uint32_t
#define MY_LEDPORT_DIR (*((volatile uint32_t *)(0xE000A000+0x00000284))) // DIRM
#define MY_LEDPORT_OUT (*((volatile uint32_t *)(0xE000A000+0x00000048))) // DATA
#define MY_LEDPORT_OEN (*((volatile uint32_t *)(0xE000A000+0x00000288))) // OEN
#define MY_PUSHBUTTON_DIR (*((volatile uint32_t *)(0xE000A000+0x00000284))) // DIRM
#define MY_PUSHBUTTON_IN (*((volatile uint32_t *)(0xE000A000+0x00000068)))  // RO

int main() {
	// Add here initialization (as in Task 1., but use own macros)
	// Data Direction registers, Output Enable registers, �
	MY_LEDPORT_DIR = 0xF00;
	MY_LEDPORT_OEN = 0xF00;
	MY_LEDPORT_OUT = 0x000;

	MY_PUSHBUTTON_DIR &= ~0x00F;

	while(1) {
		// Use own macros to read and write the Pin Output & Input registers
		// Remember bit positions of your hardware components
		MY_LEDPORT_OUT = MY_PUSHBUTTON_IN<<4;
	}
}
