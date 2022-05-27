CFLAGS=-m32 -fno-pic -c -ffreestanding -nostdlib -nostartfiles -g
LDFLAGS=-m32 -no-pie -ffreestanding -nostdlib -nostartfiles -T link.lds -g
ASFLAGS =-f elf32

SRCS=$(wildcard src/*.[c,S])
OBJS=$(addprefix out/, $(addsuffix .o, $(basename $(notdir $(SRCS:.c=.o)))))

all: kern.iso

run: kern.iso
	qemu-system-x86_64 -serial stdio -cdrom kern.iso

format:
	clang-format -i src/*.c src/*.h

kern.iso: iso/boot/kern.elf
	grub-mkrescue -o kern.iso iso

iso/boot/kern.elf: out/ $(OBJS)
	gcc $(LDFLAGS) $(OBJS) -o iso/boot/kern.elf

out/:
	mkdir -p out/

out/%.o: src/%.S
	nasm $(ASFLAGS) $< -o $@

out/%.o: src/%.c
	gcc $(CFLAGS) $< -o $@

clean:
	rm -rf kern.iso iso/boot/kern.elf out/

.PHONY: all clean run format
