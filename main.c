#include <stdint.h>
#include <stdbool.h>

#include "stm32wb15xx.h"

#define SystemClkRt ((uint16_t) 4000000)

#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) ((uint8_t) pin & 255)
#define PINBANK(pin) (pin >> 8)
#define BANK(bank) ((bank) - 'A')

enum {GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF, GPIO_MODE_ANALOG};
#define GPIO(bank) ((GPIO_TypeDef *) (GPIOA_BASE + 0x400U * (bank))) 

uint32_t wait = 2;
struct note *cur;
volatile bool led_on = false;
uint16_t buadrate = 9600;

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

// note freqs:
// C 523
// B 494
// B flat 466
// A 440
// A flat 415
// G 392
// G flat 370
// F 349
// E 330
// E flat 311
// D 294
// D flat 277
// C 262

struct note {
	int8_t oct;
	uint32_t freq;
	uint32_t period;
};

uint32_t sine_wave[16] = {4,5,6,7,7,7,6,5,4,3,2,1,1,1,2,3};
struct note sheet_music[] = {
	{1, 526, 8},
	{2, 494, 4},
	{1, 466, 8},
	{2, 440, 4},
	{1, 415, 8},
	{2, 392, 4},
	{1, 370, 8},
	{2, 349, 4},
	{1, 330, 8},
	{4, 311, 4},
	{3, 294, 8},
	{2, 277, 4},
	{1, 262, 8},
	{0, 0, 0}
};

void
USART_tran(uint8_t m)
{
	while (!(USART1->ISR & USART_ISR_TXFT));
	USART1->TDR = m;
}

int
main(void)
{

	// init after mem init
	cur = &sheet_music[0];

	


	return 1;
}

void
DAC_out(uint32_t val) 
{	
	if (led_on) val |= BIT(5);
	GPIO_TypeDef *gpiob = GPIO(BANK('B'));
	// sets bits
	gpiob->BSRR = val;
	gpiob->BSRR = (~val << 16);
}

uint8_t i = 0;
void
SysTick_Handler(void)
{
	i = (i+1)&0x000F;
	if (cur->freq != 0) DAC_out(sine_wave[i]);
	else DAC_out(0);
	return;
}

void
play_note(struct note *n)
{
	uint32_t oct_mult = 1;
	for (int j = 1; j < n->oct; j++) oct_mult *= 2;
	SysTick->LOAD = (250000) / (n->freq * oct_mult);
	wait = n->period;
}

void 
TIM2_IRQHandler(void)
{
	TIM2->SR &= ~TIM_SR_UIF;
	if (--wait != 0) return;
	cur++;
	if (cur->freq == 0 && cur->period == 0) cur = &sheet_music[0];
	play_note(cur);
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
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN; 

	TIM2->PSC = 999;
	TIM2->ARR = 999;
	TIM2->EGR |= TIM_EGR_UG;
	TIM2->DIER |= TIM_DIER_UIE;
	TIM2->SR &= ~TIM_SR_UIF;

	NVIC_SetPriority(TIM2_IRQn, 0x03);
	NVIC_EnableIRQ(TIM2_IRQn);
	TIM2->CR1 |= TIM_CR1_CEN;
}

static inline void
uart_init()
{
	RCC->AHB2ENR &= RCC_AHB2ENR_GPIOAEN;
	RCC->APB2ENR &= RCC_APB2ENR_USART1EN;

	uint16_t uartdiv = SystemClkRt / buadrate;
	USART1->BRR = (uint32_t) ((uartdiv /16) << 4) | (uartdiv % 16);


	GPIO_TypeDef *gpioa = GPIO(BANK('A'));

	uint16_t uart_ck = PIN('A', 8);
	uint16_t uart_tx = PIN('A', 9);
	uint16_t uart_rx = PIN('A', 10);
	// uint16_t uart_cts = PIN('A', 11);

	gpio_set_mode(uart_ck, GPIO_MODE_AF);
	gpio_set_mode(uart_tx, GPIO_MODE_AF);
	gpio_set_mode(uart_rx, GPIO_MODE_AF);

	// enable USART Alternate Function registers
	gpioa->AFR[1] &= 7UL << GPIO_AFRH_AFSEL8_Pos;
	gpioa->AFR[1] &= 7UL << GPIO_AFRH_AFSEL9_Pos;
	gpioa->AFR[1] &= 7UL << GPIO_AFRH_AFSEL10_Pos;

	// enable USART Transmitter FIFO and enable
	USART1->CR1 &= USART_CR1_TE & USART_CR1_FIFOEN & USART_CR1_UE;
}
void
SystemInit(void)
{
	// Default clock msi at 4MHz
	systick_init(1000);
	uart_init();



	uint16_t out1 = PIN('B', 1);
	uint16_t out2 = PIN('B', 2);
	uint16_t out3 = PIN('B', 3);
	uint16_t led = PIN('B', 5);

	RCC->AHB2ENR |= BIT(PINBANK(out1));
	gpio_set_mode(out1, GPIO_MODE_OUTPUT);
	gpio_set_mode(out2, GPIO_MODE_OUTPUT);
	gpio_set_mode(out3, GPIO_MODE_OUTPUT);
	gpio_set_mode(led, GPIO_MODE_OUTPUT);

	tim2_init();

}

void
SystemCoreClockUpdate(void)
{
}
