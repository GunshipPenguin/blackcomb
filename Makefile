CC=gcc
AS=nasm

CFLAGS=-m32 -fno-pic -c -ffreestanding -nostdlib -nostartfiles -g
LDFLAGS=-m32 -no-pie -ffreestanding -nostdlib -nostartfiles -T link.lds -g
ASFLAGS =-f elf32

CFLAGS+=-DPRINTF_DISABLE_SUPPORT_FLOAT -DPRINTF_DISABLE_SUPPORT_LONG_LONG

SRCS=$(wildcard src/*.c src/*.asm)
OBJS=$(addprefix out/, $(addsuffix .o, $(basename $(notdir $(SRCS:.c=.o)))))

all: kern.iso

run: kern.iso
	qemu-system-i386 -monitor stdio -cdrom kern.iso

format:
	clang-format -i src/*.c src/*.h

kern.iso: iso/boot/kern.elf
	grub-mkrescue -o kern.iso iso

iso/boot/kern.elf: out/ $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o iso/boot/kern.elf

out/:
	mkdir -p out/

out/%.o: src/%.asm
	$(AS) $(ASFLAGS) $< -o $@

out/%.o: src/%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf kern.iso iso/boot/kern.elf out/

.PHONY: all clean run format
