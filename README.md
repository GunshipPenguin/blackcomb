# Blackcomb

Blackcomb &mdash; It's like
[Whistler](https://en.wikipedia.org/wiki/List_of_Microsoft_codenames#Windows_NT_family),
but different. 

Blackcomb is a hobbyist OS for x86-64. It's currently in rather early
development, and documentation is scarce while it's actually bootstrapped. The
codebase currently looks like this:


```
.
├── build-cross.sh            # Script to build the cross-compiler, with which the kernel is built 
├── iso                       # Input directory passed to grub-mkrescue, which builds a boot CD
│   └── boot
│         └─── grub
│              └── grub.cfg   # Grub configuration to boot blackcomb
├── link.lds                  # Kernel link script
├── Makefile
├── README.md
└── src
    ├── drivers               # Architecture and device specific drivers (e.g. interrupt setup code, ATA driver)
    ├── entry.asm             # Kernel entry point
    ├── fs                    # Partial ext2 implementation and exec code
    ├── include
    ├── kernel                # Main body of "canonical" kernel code 
    ├── lib                   # Misc. utilities 
    └── mm                    # Memory management code
```

## Current Status

- Runs in long (64-bit) mode
- Interrupt handling
- Paged virtual memory and per-process address spaces
- Round-robin process scheduling
- (a subset of) ext2
- Support for the `syscall` instruction and a basic syscall layer providing the following
  - write (to stdout only)
  - fork 

## License

[MIT](https://github.com/GunshipPenguin/blackcomb/blob/master/LICENSE) &#x00a9; Rhys Rustad-Elliott
