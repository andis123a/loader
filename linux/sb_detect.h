#ifndef SB_DETECT_H
#define SB_DETECT_H

// Southbridge Family IDs
#define SB_AEOLIA   0x1
#define SB_BELIZE   0x2
#define SB_BAIKAL   0x3
#define SB_BELIZE2  0x4

// Bit-packing offsets for the kexec_load argument
#define KEXEC_SB_SHIFT    28
#define KEXEC_FW_SHIFT    16
#define KEXEC_VRAM_MASK   0xFFFFu

#endif