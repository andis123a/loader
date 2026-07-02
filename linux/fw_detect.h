/*
 * fw_detect.h — firmware version detection via libc.sprx SCE header
 *
 * Adapted from Al-Azif/ps4-skeleton (MIT).
 * No special SDK required — only libkernel + libc.
 */

#ifndef FW_DETECT_H
#define FW_DETECT_H

#include "aio_types.h"

/*
 * Returns the firmware version as a plain integer decoded from BCD.
 * Examples: 505, 672, 700, 702, 900, 903, 960,
 *           1000, 1050, 1100, 1102, 1150, 1152, 1200, 1250, 1300, 1302
 *
 * Returns 0 on failure.
 * Result is cached after first call (safe to call multiple times).
 */
u16 get_firmware(void);

/*
 * Maps alias versions to the canonical base version used in the
 * offset table.  Must be called before find_offsets_by_fw().
 *
 * Examples:
 *   701/702  → 700
 *   1152     → 1150
 *   1202     → 1200
 *   1252     → 1250
 *   1302     → 1300  (NOTE: 1302 IS its own distinct fw with its own entry)
 *
 * For any unrecognised alias the raw value is returned unchanged so that
 * the caller can distinguish "no entry" from "wrong alias".
 */
u16 normalize_fw_ver(u16 raw_fw);

#endif /* FW_DETECT_H */
