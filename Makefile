CC=gcc
AS=nasm

CFLAGS=-Wall -m32 -I src/include -fno-pic -c -ffreestanding -nostdlib -nostartfiles -g
LDFLAGS=-m32 -no-pie -ffreestanding -nostdlib -nostartfiles -T link.lds -g
ASFLAGS =-f elf32

CFLAGS+=-DPRINTF_DISABLE_SUPPORT_FLOAT -DPRINTF_DISABLE_SUPPORT_LONG_LONG

C_SRCS=$(shell find src -name "*.c")
ASM_SRCS=$(shell find src -name "*.asm")
SRCS=$(C_SRCS) $(ASM_SRCS)

OBJS= $(patsubst %.asm,%.o,$(patsubst %.c,%.o,$(SRCS)))

all: kern.iso

run: kern.iso
	qemu-system-i386 -monitor stdio -cdrom kern.iso

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
