#include <stdint.h>

#define MIO_PIN07_CTRL       *((uint32_t *)0xF800071C)
#define MIO_PIN07_CTRL_GPIO0 0x00003200

#define GPIO_DIRM0           *((uint32_t *)0xE000A204)
#define GPIO_OEN0            *((uint32_t *)0xE000A208)
#define GPIO_DATA0           *((uint32_t *)0xE000A040)

#define GPIO0_PIN7           0x80

void delay(uint32_t delay) {
  for(volatile uint32_t i = 0; i < delay; i++);
}

void init(void)
{
  MIO_PIN07_CTRL = MIO_PIN07_CTRL_GPIO0;

  GPIO_DIRM0 |= GPIO0_PIN7;
  GPIO_OEN0  |= GPIO0_PIN7;

  while(1) {
    GPIO_DATA0 &= ~GPIO0_PIN7;
    delay(1000000);
    GPIO_DATA0 |= GPIO0_PIN7;
    delay(1000000);
  }
}
