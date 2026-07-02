/*
 * aio_types.h — Isolated kernel-style integer types.
 *
 * INTENTIONALLY excludes any POSIX or standard headers. This guarantees
 * no typedef conflicts when AIO payload is compiled with FreeBSD sys/types.h
 * headers already included in the path.
 */

#ifndef AIO_TYPES_H
#define AIO_TYPES_H

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;

#endif /* AIO_TYPES_H */
