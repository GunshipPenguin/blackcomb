#ifndef __PS2_H
#define __PS2_H

#include <stddef.h>

void ps2_init();
void ps2_key_in();

void ps2_read(char *buf, size_t n);

#endif /* __PS2_H */
