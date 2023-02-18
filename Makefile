CC=./cross/bin/x86_64-elf-gcc
AS=nasm

CFLAGS=-mcmodel=kernel -Wall -I src/include -c -ffreestanding -nostdlib -g
LDFLAGS=-ffreestanding -nostdlib -T link.lds -g
ASFLAGS =-f elf64

CFLAGS+=-DPRINTF_DISABLE_SUPPORT_FLOAT -DPRINTF_DISABLE_SUPPORT_LONG_LONG

C_SRCS=$(shell find src -name "*.c")
ASM_SRCS=$(shell find src -name "*.asm")
SRCS=$(C_SRCS) $(ASM_SRCS)

OBJS= $(patsubst %.asm,%.o,$(patsubst %.c,%.o,$(SRCS)))

all: kern.iso

run: kern.iso
	qemu-system-x86_64 -monitor stdio -cdrom kern.iso -hda hdd.img

format:
	clang-format -i $(C_SRCS)

kern.iso: iso/boot/kern.elf
	grub-mkrescue -o kern.iso iso

iso/boot/kern.elf: out/ $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o iso/boot/kern.elf

out/:
	mkdir -p out/

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf kern.iso iso/boot/kern.elf $(OBJS)

.PHONY: all clean run format
