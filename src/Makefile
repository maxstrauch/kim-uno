#
#	KIM UNO
#	(c) 2018 - 2019. Maximilian Strauch.
#

# Frequency of the MCU
F_CPU = 8000000UL

all:
	@echo    
	@echo ---------- Building ----------
	@echo    

	avr-gcc -o main.elf -Os -DF_CPU=$(F_CPU) -mmcu=atmega328p main.c
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex

	@echo
	@echo
	@echo --- Memory usage ---
	@echo
	@avr-size -d -A main.hex

# Clean the entire project
clean:
	@rm *.elf || true
	@rm *.hex || true

# Flash the project to the microcontroller
install:
	avrdude -c stk500v1 -b 9600 -v -v -P /dev/cu.usbmodem1421 -p m328p -U flash:w:main.hex

# Adjust fuse bits
fuse:
	avrdude -c stk500v1 -b 9600 -v -v -P /dev/cu.usbmodem1421 -p m328p -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m
