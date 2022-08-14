#ifndef _COMPILER_ATTRIBUTES_H
#define _COMPILER_ATTRIBUTES_H

#define __noreturn                      __attribute__((__noreturn__))

#define __printf(a, b)                  __attribute__((__format__(printf, a, b)))
#define __scanf(a, b)                   __attribute__((__format__(scanf, a, b)))

#define __packed                        __attribute__((__packed__))

#define __aligned(x)                    __attribute__((__aligned__(x)))

#define __always_inline                 inline __attribute__((__always_inline__))

#define __must_check                    __attribute__((__warn_unused_result__))

#endif
