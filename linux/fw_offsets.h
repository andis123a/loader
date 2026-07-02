/*
Those are the offsets needed for the linux payload, all other offsets are for the ps4-kexec
*/

#ifndef FW_OFFSETS_H
#define FW_OFFSETS_H

#include "aio_types.h"

typedef struct {
    u16 fw_ver;           /* Normalised version (key)              */
    u64 xfast_syscall;    /* kernel_offset_xfast_syscall           */
    u64 printf_off;       /* kern_off_printf                       */
    u64 kmem_alloc;       /* kern_off_kmem_free (alloc entry)      */
    u64 kernel_map;       /* kern_off_kernel_map                   */
    u64 patch1;           /* kmem_alloc branch patch — byte 1      */
    u64 patch2;           /* kmem_alloc branch patch — byte 2      */
    u64 pstate;           /* kern_off_pstate_before_shutdown        */
} fw_offsets_t;

/*
 * fw_table[] — one entry per supported canonical firmware version.
 *
 * Alias versions (e.g. 7.01, 7.02 → 700) are resolved by
 * normalize_fw_ver() in fw_detect.c before this table is consulted.
 * Versions marked with a distinct entry (e.g. 1102, 1302) have
 * measurably different kernel layouts and are NOT aliases.
 *
 */
static fw_offsets_t fw_table[] = {
    // ver   xfast  printf      kmem_alloc  kernel_map  patch1      patch2      pstate
    {  505, 0x1c0,  0x436040,   0x0FCC80,   0x1AC60E0,  0x0FCD48,   0x0FCD56,   0x10D97E  },
    {  672, 0x1c0,  0x123280,   0x250730,   0x220DFC0,  0x2507F5,   0x250803,   0x20734E  },
    {  700, 0x1c0,  0x0BC730,   0x1170F0,   0x21C8EE0,  0x1171BE,   0x1171C6,   0x2CDD6E  },
    {  750, 0x1c0,  0x26F740,   0x1753E0,   0x21405B8,  0x1754AC,   0x1754B4,   0x0D2ED0  },
    {  800, 0x1c0,  0x430AE0,   0x01B3F0,   0x1B243E0,  0x01B4BC,   0x01B4C4,   0x155B50  },
    {  850, 0x1c0,  0x15D570,   0x2199A0,   0x1C64228,  0x219A6C,   0x219A74,   0x40BA10  },
    {  900, 0x1c0,  0x0B7A30,   0x37BE70,   0x2268D48,  0x37BF3C,   0x37BF44,   0x29A970  },
    {  903, 0x1c0,  0x0B79E0,   0x37A070,   0x2264D48,  0x37A13C,   0x37A144,   0x29A5F0  },
    {  960, 0x1c0,  0x205470,   0x1889D0,   0x2147830,  0x188A9C,   0x188AA4,   0x3323E0  },
    { 1000, 0x1c0,  0x0C50F0,   0x33B040,   0x227BEF8,  0x33B10C,   0x33B114,   0x4812D0  },
    { 1050, 0x1c0,  0x450E80,   0x428960,   0x22A9250,  0x428A2C,   0x428A34,   0x45DCC0  },
    { 1100, 0x1c0,  0x2FCBD0,   0x245E10,   0x21FF130,  0x245EDC,   0x245EE4,   0x198650  },
    { 1102, 0x1c0,  0x2FCBF0,   0x245E30,   0x21FF130,  0x245EFC,   0x245F04,   0x198670  },
    { 1150, 0x1c0,  0x2E01A0,   0x4657A0,   0x22D1D50,  0x46586C,   0x465874,   0x3A2120  },
    { 1200, 0x1c0,  0x2E03E0,   0x4659E0,   0x22D1D50,  0x465AAC,   0x465AB4,   0x3A2360  },
    { 1250, 0x1c0,  0x2E0420,   0x465A20,   0x22D1D50,  0x465AEC,   0x465AF4,   0x3A23A0  },
    { 1300, 0x1c0,  0x2E0440,   0x465A40,   0x22D1D50,  0x465B0C,   0x465B14,   0x3A23C0  },
    { 1302, 0x1c0,  0x2E0450,   0x465A50,   0x22D1D50,  0x465B1C,   0x465B24,   0x3A23D0  },
    { 1350, 0x1c0,  0x2E0460,   0x465E90,   0x22D1D50,  0x465F5C,   0x465F64,   0x3A2780  },
    { 0 }   /* sentinel */
};

#endif
