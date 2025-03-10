.RECIPEPREFIX = >

CFLAGS  ?=  -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion \
            -Wformat-truncation -fno-common -Wconversion \
            -g3 -Os -ffunction-sections -fdata-sections \
            -I. -Iinclude -Ivendor/cmsis_arm_v6/CMSIS/Core/Include -Ivendor/cmsis_wb/Include \
            -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 $(EXTRA_CFLAGS)
LDFLAGS ?= -Tvendor/cmsis_wb/Source/Templates/gcc/linker/stm32wb15xx_flash_cm4.ld -nostartfiles -nostdlib --specs nano.specs -lc -lgcc -Wl,--gc-sections -Wl,-Map=$@.map
SOURCES = main.c 
SOURCES += vendor/cmsis_wb/Source/Templates/gcc/startup_stm32wb15xx_cm4.s

build: firmware.bin

cmsis_arm_v6:
> git clone --depth 1 -b "v6.1.0" "https://github.com/ARM-software/CMSIS_6.git" vendor/$@

cmsis_wb:
> git clone --depth 1 -b "v1.12.2" "https://github.com/STMicroelectronics/cmsis-device-wb.git" vendor/$@

firmware.elf: $(SOURCES) 
> bear -- arm-none-eabi-gcc $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $@

firmware.bin: firmware.elf
> arm-none-eabi-objcopy -O binary $< $@

flash: firmware.bin
> openocd -f interface/stlink.cfg -f target/stm32wbx.cfg -c "program firmware.elf verify reset exit"

clean:
> rm firmware.*
