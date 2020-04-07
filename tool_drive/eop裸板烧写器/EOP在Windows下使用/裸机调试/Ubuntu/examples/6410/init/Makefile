objs := init.o

all: $(objs)
#	arm-linux-ld -Ttext 0x0c000000 -o init.elf $^
	arm-linux-ld -Tinit.lds -o init.elf $^
	arm-linux-objcopy -O binary -S init.elf init.bin
	arm-linux-objdump -D -m arm init.elf > init.dis
	
%.o:%.c
	arm-linux-gcc -Wall -g -c -o $@ $<

%.o:%.S
	arm-linux-gcc -Wall -g -c -o $@ $<

clean:
	rm -f init.bin init.elf init.dis *.o		
	
