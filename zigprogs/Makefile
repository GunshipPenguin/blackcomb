FLAGS=-target x86_64-freestanding-none -mcpu x86_64-sse-sse2+soft_float

all: shell init

shell:
	zig build-exe $(FLAGS) shell.zig

init:
	zig build-exe $(FLAGS) init.zig
