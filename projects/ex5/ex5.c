#include <stdint.h>
#include <stddef.h>
#include "zynq_registers.h"

#define XTTCPS_CLK_CNTRL_PS_EN_MASK		0x00000001U  /**< Prescale enable */
#define XTTCPS_CLK_CNTRL_PS_VAL_SHIFT			 1U  /**< Prescale shift */
#define XTTCPS_CNT_CNTRL_RST_MASK		0x00000010U /**< Reset counter */
#define XTTCPS_CNT_CNTRL_DIS_MASK		0x00000001U /**< Disable the counter */
#define XTTCPS_CNT_CNTRL_MATCH_MASK		0x00000008U /**< Match mode */
#define XTTCPS_CNT_CNTRL_POL_WAVE_MASK	0x00000040U /**< Waveform polarity */
#define XTTCPS_MATCH_0_OFFSET		0x00000030U  /**< Match 1 value */

void usleep(uint32_t usecs) {
  for(volatile uint32_t i = 0; i < usecs; i++);
}

int main(void) {

	// Two TTC module in the Zynq PS (TTC0 & TTC1)
	// Each TTC module contains three independent 16-bit prescalers and 16-bit up/down counters (0,1,2)
	// The register naming follows Xilinx TRM UG585 for consistency - however it's not very good example of proper naming!

	// Clock Control register - TTC0_CLK_CNTRLx
	// Set prescale to 0 (plus 1) (hint: (value << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT)
	// enable prescaler (hint: use XTTCPS_CLK_CNTRL_PS_EN_MASK mask)
	TTC0_CLK_CNTRL  = 1;
	TTC0_CLK_CNTRL2 = TTC0_CLK_CNTRL; // Set identical to TTC0_CLK_CNTRL
	TTC0_CLK_CNTRL3 = TTC0_CLK_CNTRL; // Set identical to TTC0_CLK_CNTRL

	// Operational mode and reset register - TTC0_CNT_CNTRLx
	//     1. Reset count value (hint: use XTTCPS_CNT_CNTRL_RST_MASK mask)
	//     2. Disable counter (XTTCPS_CNT_CNTRL_DIS_MASK)
	//     3. Set timer to Match mode (XTTCPS_CNT_CNTRL_MATCH_MASK)
	//     4. Enable output Waveform (XTTCPS_CNT_CNTRL_POL_WAVE_MASK)
	//        (Waveform output is default to EMIO, which is connected in the FPGA to the RGB led (LD6)
	TTC0_CNT_CNTRL  = 1;
	TTC0_CNT_CNTRL2 = TTC0_CNT_CNTRL; // Set identical to TTC0_CNT_CNTRL
	TTC0_CNT_CNTRL3 = TTC0_CNT_CNTRL; // Set identical to TTC0_CNT_CNTRL

	// Operational mode and reset register - TTC0_CNT_CNTRLx
	//     1. Start timer (hint: clear operation using XTTCPS_CNT_CNTRL_DIS_MASK)
	TTC0_CNT_CNTRL  =1;
	TTC0_CNT_CNTRL2 =1;
	TTC0_CNT_CNTRL3 =1;

	// Variable initialization
	uint16_t pwm = 0;
	uint8_t state = 0;
	volatile uint32_t* ptr_register = NULL;

	while(1) {
		switch (state) {
			case  0:  ptr_register = &TTC0_MATCH_0; 			break;
			case  3:  ptr_register = &TTC0_MATCH_1_COUNTER_2; 	break;
			case  6:  ptr_register = &TTC0_MATCH_1_COUNTER_3; 	break;
			case  1:  case 4: case 7: *ptr_register = pwm++;	break;
			case  2:  case 5: case 8: *ptr_register = pwm--;	break;
			case  9: TTC0_MATCH_0 = TTC0_MATCH_1_COUNTER_2 = TTC0_MATCH_1_COUNTER_3 = pwm++; break;
			case 10: TTC0_MATCH_0 = TTC0_MATCH_1_COUNTER_2 = TTC0_MATCH_1_COUNTER_3 = pwm--; break;
		}

		if (pwm == 0) // If pwm variable value zero
			state == 10 ? state = 0 : state++; // change state

		usleep(10); // Sleep 10µs
	}

	return 0;
}
