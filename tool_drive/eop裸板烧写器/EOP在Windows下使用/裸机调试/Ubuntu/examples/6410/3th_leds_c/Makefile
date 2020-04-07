all: led.bin

led.bin: start.o led.o
	arm-linux-ld -Ttext 0 -o led.elf start.o led.o
	arm-linux-objcopy -O binary led.elf led.bin
	arm-linux-objdump -D led.elf > led.dis

start.o : start.S
	arm-linux-gcc -g -o start.o start.S -c
led.o : led.c
	arm-linux-gcc -g -o led.o led.c -c

clean:
	rm *.o led.elf led.bin led.dis
