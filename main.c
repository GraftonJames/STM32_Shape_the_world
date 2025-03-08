#include <stdint.h>
#include <stdbool.h>

#include "stm32wb15xx.h"

#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) ((uint8_t) pin & 255)
#define PINBANK(pin) (pin >> 8)

enum {GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF, GPIO_MODE_ANALOG};
#define GPIO(bank) ((GPIO_TypeDef *) (GPIOA_BASE + 0x400U * (bank))) 

static inline void
gpio_set_mode(uint16_t pin, uint8_t mode)
{
	GPIO_TypeDef *gpio = GPIO(PINBANK(pin));
	uint8_t n = PINNO(pin);

	gpio->MODER &= ~(3U << (n * 2));
	gpio->MODER |=	(mode & 3) << (n * 2);
}

static inline void
gpio_write(uint16_t pin, bool val)
{
	GPIO_TypeDef *gpio = GPIO(PINBANK(pin));
	gpio->BSRR = (1U << PINNO(pin)) << (val ? 0 : 16);
}

static inline void
systick_init(uint32_t ticks)
{
	if ((ticks - 1) > 0xffffff) return;
	
	SysTick->LOAD = ticks - 1;
	SysTick->VAL = 0;
	SysTick->CTRL = BIT(0) | BIT(1) | BIT(2);
}

static inline void
spin(volatile uint32_t count)
{
	while (count--) (void) 0;
}
static volatile uint32_t s_ticks;
void
delay(unsigned ms)
{
	uint32_t until = s_ticks + ms;
	while (s_ticks < until) (void) 0;
}

bool
timer_expired(uint32_t *t, uint32_t prd, uint32_t now)
{
	if (now + prd < *t) *t = 0;
	if (*t == 0) *t = now + prd;
	if (*t > now) return false;
	*t = (now - *t) > prd ? now + prd : *t + prd;
	return true;
}

int
main(void) {

	uint16_t led = PIN('B', 5);
	RCC->AHB2ENR |= BIT(PINBANK(led));
	systick_init(16000000/1000);
	gpio_set_mode(led, GPIO_MODE_OUTPUT);


	uint32_t timer, period = 500;
	for (;;) {
		if (timer_expired(&timer, period, s_ticks)) {
			static bool on;
			gpio_write(led, on);
			on = !on;
		}
	}
	return 0;
}

void
SysTick_Handler(void) {
	s_ticks++;
}

void SystemInit(void)
{
}

void SystemCoreClockUpdate(void)
{
}
