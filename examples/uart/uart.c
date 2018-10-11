#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

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

#define POINTER_TO_REGISTER(REG)  ( *((volatile uint32_t*)(REG)))

#define UART_BASE      XPS_UART1_BASEADDR
#define UART_CTRL      POINTER_TO_REGISTER(UART_BASE + XUARTPS_CR_OFFSET)      // Control Register
#define UART_MODE      POINTER_TO_REGISTER(UART_BASE + XUARTPS_MR_OFFSET)      // Mode Register

#define UART_BAUD_GEN  POINTER_TO_REGISTER(UART_BASE + XUARTPS_BAUDGEN_OFFSET) // Baud Rate Generator "CD"
#define UART_BAUD_DIV  POINTER_TO_REGISTER(UART_BASE + XUARTPS_BAUDDIV_OFFSET) // Baud Rate Divider "BDIV"
#define UART_FIFO      POINTER_TO_REGISTER(UART_BASE + XUARTPS_FIFO_OFFSET)    // FIFO
#define UART_STATUS    POINTER_TO_REGISTER(UART_BASE + XUARTPS_SR_OFFSET)      // Channel Status

#define MIO_PIN07_CTRL       *((uint32_t *)0xF800071C)
#define MIO_PIN07_CTRL_GPIO0 0x00003200

#define GPIO_DIRM0           *((uint32_t *)0xE000A204)
#define GPIO_OEN0            *((uint32_t *)0xE000A208)
#define GPIO_DATA0           *((uint32_t *)0xE000A040)

#define GPIO0_PIN7           0x80

/*
 * To implement the STDIO functions you need to create
 * the _read and _write functions and hook them to the
 * USART you are using. This example also has a buffered
 * read function for basic line editing.
 */
int _write(int fd, char *ptr, int len);
int _read(int fd, char *ptr, int len);
void get_buffered_line(void);

/*
 * This is a pretty classic ring buffer for characters
 */
#define BUFLEN 127

static uint16_t start_ndx;
static uint16_t end_ndx;
static char buf[BUFLEN+1];
#define buf_len ((end_ndx - start_ndx) % BUFLEN)
static inline int inc_ndx(int n) { return ((n + 1) % BUFLEN); }
static inline int dec_ndx(int n) { return (((n + BUFLEN) - 1) % BUFLEN); }

void uart_send(char c) {
  while (UART_STATUS & XUARTPS_SR_TNFUL);
  UART_FIFO = c;
  while (UART_STATUS & XUARTPS_SR_TACTIVE);
}

char uart_recv() {
  if ((UART_STATUS & XUARTPS_SR_RXEMPTY) == XUARTPS_SR_RXEMPTY)
    return 0;

  return UART_FIFO;
}

char uart_recv_blocking() {
  while ((UART_STATUS & XUARTPS_SR_RXEMPTY) == XUARTPS_SR_RXEMPTY);
  return UART_FIFO;
}

void uart_setup(void) {
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
}

void gpio_setup() {
  MIO_PIN07_CTRL = MIO_PIN07_CTRL_GPIO0;

  GPIO_DIRM0 |= GPIO0_PIN7;
  GPIO_OEN0  |= GPIO0_PIN7;
}

static inline void led_toggle() {
  GPIO_DATA0 ^= GPIO0_PIN7;
}

/* back up the cursor one space */
static inline void back_up(void)
{
  end_ndx = dec_ndx(end_ndx);
  uart_send('\010');
  uart_send(' ');
  uart_send('\010');
}

/*
 * A buffered line editing function.
 */
void
get_buffered_line(void) {
  char  c;

  if (start_ndx != end_ndx) {
    return;
  }
  while (1) {
    c = uart_recv_blocking();
    if (c == '\r') {
      buf[end_ndx] = '\n';
      end_ndx = inc_ndx(end_ndx);
      buf[end_ndx] = '\0';
      uart_send('\r');
      uart_send('\n');
      return;
    }
    /* ^H or DEL erase a character */
    if ((c == '\010') || (c == '\177')) {
      if (buf_len == 0) {
        uart_send('\a');
      } else {
        back_up();
      }
    /* ^W erases a word */
    } else if (c == 0x17) {
      while ((buf_len > 0) &&
          (!(isspace((int) buf[end_ndx])))) {
        back_up();
      }
    /* ^U erases the line */
    } else if (c == 0x15) {
      while (buf_len > 0) {
        back_up();
      }
    /* Non-editing character so insert it */
    } else {
      if (buf_len == (BUFLEN - 1)) {
        uart_send('\a');
      } else {
        buf[end_ndx] = c;
        end_ndx = inc_ndx(end_ndx);
        uart_send(c);
      }
    }
  }
}

/*
 * Called by libc stdio fwrite functions
 */
int
_write(int fd, char *ptr, int len)
{
  int i = 0;

  /*
   * Write "len" of char from "ptr" to file id "fd"
   * Return number of char written.
   *
   * Only work for STDOUT, STDIN, and STDERR
   */
  if (fd > 2) {
    return -1;
  }
  while (*ptr && (i < len)) {
    uart_send(*ptr);
    if (*ptr == '\n') {
      uart_send('\r');
    }
    i++;
    ptr++;
  }
  return i;
}

/*
 * Called by the libc stdio fread fucntions
 *
 * Implements a buffered read with line editing.
 */
int
_read(int fd, char *ptr, int len)
{
  int my_len;

  if(fd > 2)
    return -1;

  get_buffered_line();
  my_len = 0;
  while ((buf_len > 0) && (len > 0)) {
    *ptr++ = buf[start_ndx];
    start_ndx = inc_ndx(start_ndx);
    my_len++;
    len--;
  }
  return my_len; /* return the length we got */
}

int get_delay() {
  char local_buf[32];
  printf("Enter the delay constant for blink: ");
  fflush(stdout);
  fgets(local_buf, sizeof(local_buf), stdin);
  return atoi(local_buf);
}

static inline void led_blink(int delay) {
  int i;
  while(1) {
    led_toggle();

    for (i = 0; i < delay; i++);

    if(uart_recv() == 0x3) // ^C
      break;
  }
}

int main(void)
{
  int delay = 0;

  uart_setup();
  gpio_setup();

  printf("\nStandard I/O Example.\n");

  while(1) {
    delay = get_delay();
    printf("Blinking with a delay of %d\n", delay);
    led_blink(delay);
  }

  return 0;
}
