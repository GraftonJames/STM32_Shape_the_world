#include <stdint.h>
#include <stdbool.h>

#include "stm32wb15xx.h"

#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) ((uint8_t) pin & 255)
#define PINBANK(pin) (pin >> 8)
#define BANK(bank) ((bank) - 'A')

enum {GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF, GPIO_MODE_ANALOG};
#define GPIO(bank) ((GPIO_TypeDef *) (GPIOA_BASE + 0x400U * (bank))) 

static inline void
gpio_set_mode(uint32_t pin, uint8_t mode)
{
	GPIO_TypeDef *gpio = GPIO(PINBANK(pin));
	uint8_t n = PINNO(pin);

	gpio->MODER &= ~(3U << (n * 2));
        gpio->MODER |= (mode & 3) << (n * 2);
}

static inline void
gpio_write(uint32_t pin, bool val)
{
	GPIO_TypeDef *gpio = GPIO(PINBANK(pin));
	gpio->BSRR = (1U << PINNO(pin)) << (val ? 0 : 16);
}


int
main(void)
{
	while(true) {};
}

const uint32_t sine_wave[16] = {4,5,6,7,7,7,6,5,4,3,2,1,1,1,2,3};
const uint32_t sheet_music[] = {1, 1};
uint32_t i = 0;
uint8_t period = 0;
const uint32_t *note = &sheet_music[0];

void
DAC_out(uint32_t val) 
{	
	GPIO_TypeDef *gpiob = GPIO(BANK('B'));
	// sets bits
	gpiob->BSRR = val | (~val << 16);
}

void
SysTick_Handler(void)
{
	i = (i+1)&0x000F;
	DAC_out(sine_wave[i]);
	return;
}

void 
TIM1_UP_IRQHandler()
{
	
}

void _init(void) { return; }

static inline void
systick_init(uint32_t ticks)
{
	if ((ticks - 1) > 0xffffff) return;
	
	SysTick->LOAD = ticks - 1;
	SysTick->VAL = 0;
	SysTick->CTRL = BIT(0) | BIT(1) | BIT(2);
}

static inline void
tim2_init()
{
	TIM2->PSC = 999;
	TIM2->ARR = 999;
	TIM2->DIER &= BIT(0);
}
void
SystemInit(void)
{
	// Default clock msi at 4MHz 568 for 440hz sin wave
	systick_init(568);
	tim2_init();

	uint16_t out1 = PIN('B', 1);
	uint16_t out2 = PIN('B', 2);
	uint16_t out3 = PIN('B', 3);

	RCC->AHB2ENR |= BIT(PINBANK(out1));
	gpio_set_mode(out1, GPIO_MODE_OUTPUT);
	gpio_set_mode(out2, GPIO_MODE_OUTPUT);
	gpio_set_mode(out3, GPIO_MODE_OUTPUT);

}

void
SystemCoreClockUpdate(void)
{
}
