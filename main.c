#include <stdint.h>
#include <stdbool.h>



#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) ((uint8_t) pin & 255)
#define PINBANK(pin) (pin >> 8)

enum {GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF, GPIO_MODE_ANALOG};

struct gpio {
	volatile uint32_t 
		MODER,
		OTYPER,
		OSPEEDR,
		PUPDR,
		IDR,
		ODR,
		BSRR,
		LCKR,
		AFRL, 
		AFRH,
		BRR;
};
#define GPIO(bank) (((struct gpio *) 0x48000000 + 0x400 * (bank)))

struct rcc { // Reset and Clock Control
	volatile uint32_t
		CR,
		ICSCR,
		CFGR,
		PLLCFGR,
		_RESERVED1[2],
		CIER,
		CIFR,
		CICR,
		SMPSCR,
		AMB1RSTR,
		AHB2RSTR,
		AHB4RSTR,
		_RESERVED2[1],
		APB1RSTR1,
		APB1RSTR2,
		APB2RSTR,
		APB3RSTR,
		AHB1ENR,
		AHB2ENR,
		AHB4ENR,
		_RESERVED3[1],
		APB1ENR1,
		APB1ENR2,
		APB2ENR,
		_RESERVED4[1],
		AHB1SMENR,
		AHB2SMENR,
		AHB4SMENR,
		_RESERVED5[1],
		APB1SMENR1,
		APB1SMENR2,
		APB2SMENR,
		_RESERVED6[1],
		CCIPR,
		_RESERVED7[1],
		BDCR,
		CSR,
		HSECR,
		_RESERVED8[26],
		EXTCFGR,
		_RESERVED9[15],
		C2AHB1ENR,
		C2AHB2ENR,
		C2AHB4ENR,
		_RESERVED10[1],
		C2APB1ENR1,
		C2APB1ENR2,
		C2APB3ENR,
		C2AHB1SMENR,
		C2AHB2SMENR,
		C2AHB4SMENR,
		_RESERVED11[1],
		C2APB1SMENR1,
		C2APB1SMENR2,
		C2APB2SMENR,
		C2APB3SMENR;
};
#define RCC ((struct rcc *) 0x58000000)

static inline void
gpio_set_mode(uint16_t pin, uint8_t mode)
{
	struct gpio *gpio = GPIO(PINBANK(pin));
	uint8_t n = PINNO(pin);

	gpio->MODER &= ~(3U << (n * 2));
	gpio->MODER |=	(mode & 3) << (pin * 2);
}

static inline void
gpio_write(uint16_t pin, bool val)
{
	struct gpio *gpio = GPIO(PINBANK(pin));
	gpio->BSRR = (1U << PINNO(pin)) << (val ? 0 : 16);
}

static inline void
spin(volatile uint32_t count)
{
	while (count--) (void) 0;
}

int
main(void) {
	uint16_t led = PIN('B', 7);
	RCC->AHB1ENR |= BIT(PINBANK(led));
	gpio_set_mode(led, GPIO_MODE_OUTPUT);
	for (;;) {
		gpio_write(led, true);
		spin(999999);
		gpio_write(led, false);
		spin(999999);
	}
	return 0;
}

//Startup code
__attribute__((naked, noreturn)) void _reset(void) {
	extern long _sbss, _ebss, _sdata, _edata, _sidata;
	for (long *dst = &_sbss; dst < &_ebss; dst++) *dst = 0;
	for (long *dst = &_sdata, *src = &_sidata; dst < &_edata;) *dst++ = *src++;

	main();
	for (;;) (void) 0;
}

extern void _estack(void);

__attribute__((section(".vectors"))) void (*const tab[16 + 63])(void) = {
	_estack, _reset
};
