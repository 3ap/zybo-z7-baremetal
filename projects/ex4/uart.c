#include <stdint.h>
#include "zynq_registers.h"

#define XUARTPS_CR_TXRST	0x00000002U  /**< TX logic reset */
#define XUARTPS_CR_RXRST	0x00000001U  /**< RX logic reset */

#define XUARTPS_CR_OFFSET       0x0000U  /**< Control Register [8:0] */
#define XUARTPS_MR_OFFSET       0x0004U  /**< Mode Register [9:0] */
#define XUARTPS_BAUDGEN_OFFSET  0x0018U  /**< Baud Rate Generator [15:0] */
#define XUARTPS_BAUDDIV_OFFSET  0x0034U  /**< Baud Rate Divider [7:0] */
#define XUARTPS_FIFO_OFFSET     0x0030U  /**< FIFO [7:0] */
#define XUARTPS_SR_OFFSET       0x002CU  /**< Channel Status [14:0] */
#define XPS_UART1_BASEADDR      0xE0001000U

#define XUARTPS_MR_CHMODE_NORM		0x00000000U /**< Normal mode */
#define XUARTPS_MR_STOPMODE_1_BIT	0x00000000U /**< 1 stop bit */
#define XUARTPS_MR_PARITY_NONE		0x00000020U /**< No parity mode */
#define XUARTPS_MR_CHARLEN_8_BIT	0x00000000U /**< 8 bits data */
#define XUARTPS_MR_CLKSEL			0x00000001U /**< Input clock selection */

#define XUARTPS_SR_TNFUL	0x00004000U /**< TX FIFO Nearly Full Status */
#define XUARTPS_SR_TACTIVE	0x00000800U /**< TX active */
#define XUARTPS_SR_RXEMPTY	0x00000002U /**< RX FIFO empty */

#define XUARTPS_CR_TX_DIS	0x00000020U  /**< TX disabled. */
#define XUARTPS_CR_TX_EN	0x00000010U  /**< TX enabled */
#define XUARTPS_CR_RX_DIS	0x00000008U  /**< RX disabled. */
#define XUARTPS_CR_RX_EN	0x00000004U  /**< RX enabled */

#define POuint32_tER_TO_REGISTER(REG)  ( *((volatile uint32_t*)(REG))) //u32 (xil_types.h) data type is declared as uuint32_t32_t (stduint32_t.h)

#define UART_BASE      XPS_UART1_BASEADDR										// Base Address
#define UART_CTRL      POuint32_tER_TO_REGISTER(UART_BASE + XUARTPS_CR_OFFSET)      // Control Register
#define UART_MODE      POuint32_tER_TO_REGISTER(UART_BASE + XUARTPS_MR_OFFSET)      // Mode Register

#define UART_BAUD_GEN  POuint32_tER_TO_REGISTER(UART_BASE + XUARTPS_BAUDGEN_OFFSET) // Baud Rate Generator "CD"
#define UART_BAUD_DIV  POuint32_tER_TO_REGISTER(UART_BASE + XUARTPS_BAUDDIV_OFFSET) // Baud Rate Divider "BDIV"
#define UART_FIFO      POuint32_tER_TO_REGISTER(UART_BASE + XUARTPS_FIFO_OFFSET)    // FIFO
#define UART_STATUS    POuint32_tER_TO_REGISTER(UART_BASE + XUARTPS_SR_OFFSET)      // Channel Status

#define BUFFER_SIZE 20

static char str[] = "\tHello World\r";

// Send one character through UART interface
void uart_send(char c) {
	while (UART_STATUS & XUARTPS_SR_TNFUL);
	UART_FIFO = c;
	while (UART_STATUS & XUARTPS_SR_TACTIVE);
}

// Send string (character array) through UART interface
void uart_send_string(char str[BUFFER_SIZE]) {
	char *ptr = str;

	while (*ptr != '\0') {
		uart_send(*ptr);
		ptr++;
	}
	}

// Check if UART receive FIFO is not empty and return the new data
char uart_receive() {
	if ((UART_STATUS & XUARTPS_SR_RXEMPTY) == XUARTPS_SR_RXEMPTY) return 0;
	return UART_FIFO;
}

// Set LED outputs based on character value '1', '2', '3', '4'
void set_leds(uint8_t input) {
	if (input < '0' || '4' < input) return;
	uint8_t mask = 0;
	char c = input - '0';
	for (uint8_t i = 0; i < c; i++) {
		mask <<= 1; // Bitwise left shift assignment
		mask |= 1; // Set first bit true / led on
	}
	GPIO_DATA_2 = mask << 8; // LEDS LD0..3 - GPIO Bank 2 bits [11:8]
}

int main(void) {
	GPIO_DIRM_2 = 0xF00;         // Set direction for bits 8 - 11 to output
	GPIO_OEN_2 = 0xF00;          // Enable output for bits 8 - 11

	uint32_t r = 0; // Temporary value variable

	r = UART_CTRL;
	r &= ~(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN); // Clear Tx & Rx Enable
	r |= XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS; // Tx & Rx Disable
	UART_CTRL = r;

	UART_MODE = 0;
	UART_MODE &= ~XUARTPS_MR_CLKSEL; // Clear "Input clock selection" - 0: clock source is uart_ref_clk
	UART_MODE |= XUARTPS_MR_CHARLEN_8_BIT; 	// Set "8 bits data"
	UART_MODE |= XUARTPS_MR_PARITY_NONE; 	// Set "No parity mode"
	UART_MODE |= XUARTPS_MR_STOPMODE_1_BIT; // Set "1 stop bit"
	UART_MODE |= XUARTPS_MR_CHMODE_NORM; 	// Set "Normal mode"

	// baud_rate = sel_clk / (CD * (BDIV + 1) (ref: UG585 - TRM - Ch. 19 UART)
	UART_BAUD_DIV = 6; // ("BDIV")
	UART_BAUD_GEN = 124; // ("CD")
	// Baud Rate = 100Mhz / (124 * (6 + 1)) = 115200 bps

	UART_CTRL |= (XUARTPS_CR_TXRST | XUARTPS_CR_RXRST); // TX & RX logic reset

	r = UART_CTRL;
	r |= XUARTPS_CR_RX_EN | XUARTPS_CR_TX_EN; // Set TX & RX enabled
	r &= ~(XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS); // Clear TX & RX disabled
	UART_CTRL = r;

	while(1) {
		// Count to big number before exiting for loop - delay loop
		for(uint32_t i = 1; i < (1 << 20); i++){
			char input = uart_receive(); // polling UART receive buffer
			if(input) set_leds(input); // if new data received call set_leds()
		}

		static char c = '0';
		uart_send(c++); // Send and increment character variable c
		// If incremented over 'Z', initialize to '0' (ref: see ASCII character table)
		if (c > 'Z') c = '0';
		uart_send_string(str); // Send character array variable str

	}
}
