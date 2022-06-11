#ifndef __UTIL_H
#define __UTIL_H

#define ALIGNUP(val, align)                                                                        \
    ({                                                                                             \
        const typeof((val)) _val = (val);                                                          \
        const typeof((align)) _align = (align);                                                    \
        (_val + (_align - 1)) & -_align;                                                           \
    })

#define ALIGNDOWN(val, align)                                                                      \
    ({                                                                                             \
        const typeof((val)) _val = (val);                                                          \
        const typeof((align)) _align = (align);                                                    \
        _val & ~(_align - 1);                                                                      \
    })

void panic(char *s);

#endif /* __UTIL_H */
