/*
 * ps4-kexec - a kexec() implementation for Orbis OS / FreeBSD
 *
 * Copyright (C) 2015-2016 shuffle2 <godisgovernment@gmail.com>
 * Copyright (C) 2015-2016 Hector Martin "marcan" <marcan@marcan.st>
 *
 * This code is licensed to you under the 2-clause BSD license. See the LICENSE
 * file for more information.
 */

#include "kernel.h"
#include "linux_boot.h"
#include "x86.h"
#include "kexec.h"
#include "firmware.h"
#include "string.h"
#include "acpi.h"
#include "../sb_detect.h"

u8 sb_id = 0;

/* x86 legacy PCI config space access via 0xCF8/0xCFC */
static u32 pci_config_read(u8 bus, u8 dev, u8 func, u8 reg)
{
	outl(0xCF8, 0x80000000 | ((u32)bus << 16) | ((u32)dev << 11) |
		   ((u32)func << 8) | (reg & 0xFC));
	return inl(0xCFC);
}

static void pci_config_write(u8 bus, u8 dev, u8 func, u8 reg, u32 val)
{
	outl(0xCF8, 0x80000000 | ((u32)bus << 16) | ((u32)dev << 11) |
		   ((u32)func << 8) | (reg & 0xFC));
	outl(0xCFC, val);
}

/* GPU MMIO base (Liverpool/Gladius BAR0 at fixed PS4 address) */
#define GPU_MMIO_BASE  0xe4800000
#define GPU_RD(off)    (*(volatile u32 *)PA_TO_DM(GPU_MMIO_BASE + (off)))
#define GPU_WR(off, v) (*(volatile u32 *)PA_TO_DM(GPU_MMIO_BASE + (off)) = (v))

static int k_copyin(const void *uaddr, void *kaddr, size_t len)
{
    if (!uaddr || !kaddr)
        return EFAULT;
    memcpy(kaddr, uaddr, len);
    return 0;
}

static int k_copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
    const char *ustr = (const char*)uaddr;
    char *kstr = (char*)kaddr;
    size_t ret;
    if (!uaddr || !kaddr)
        return EFAULT;
    ret = strlcpy(kstr, ustr, len);
    if (ret >= len) {
        if (done)
            *done = len;
        return ENAMETOOLONG;
    } else {
        if (done)
            *done = ret + 1;
    }
    return 0;
}

static int k_copyout(const void *kaddr, void *uaddr, size_t len)
{
    if (!uaddr || !kaddr)
        return EFAULT;
    memcpy(uaddr, kaddr, len);
    return 0;
}

#define KEXEC_VRAM_MASK 0xFFFFu
#define KEXEC_FW_MASK   0xFFFu
#define KEXEC_SB_MASK   0xFu

static u16 kexec_get_vram_mb(int packed)
{
    return (u16)((u32)packed & KEXEC_VRAM_MASK);
}

static u16 kexec_get_fw_ver(int packed)
{
    // Shift past VRAM, then mask for 12 bits
    return (u16)(((u32)packed >> 16) & KEXEC_FW_MASK);
}

static u8 kexec_get_sb_id(int packed)
{
    // Shift to the very end for the 4-bit SB ID
    return (u8)(((u32)packed >> 28) & KEXEC_SB_MASK);
}

static const char* kexec_get_sb_name(u8 id)
{
    switch (id) {
        case SB_AEOLIA:  return "Aeolia";
        case SB_BELIZE:  return "Belize";
        case SB_BAIKAL:  return "Baikal";
        case SB_BELIZE2: return "Belize2";
        default:         return "Unknown Southbridge";
    }
}

static void kexec_print_banner(u16 fw_ver, u16 vram_mb, u8 sb_id)
{
    kern.printf("========================================\n");
    kern.printf("PS4 Linux Payloads AIO\n");
    if (fw_ver) {
        kern.printf("FW %u.%02u (%u)\n",
            (unsigned int)(fw_ver / 100),
            (unsigned int)(fw_ver % 100),
            (unsigned int)fw_ver);
    } else {
        kern.printf("FW unknown\n");
    }
    kern.printf("VRAM %u MB\n", (unsigned int)vram_mb);
    kern.printf("Southbridge: %s\n", kexec_get_sb_name(sb_id));
    kern.printf("========================================\n");
}

int sys_kexec(void *td, struct sys_kexec_args *uap)
{
    int err = 0;
    size_t initramfs_size = uap->initramfs_size;
    void *image = NULL;
    void *initramfs = NULL;
    size_t firmware_size = 0;
    struct boot_params *bp = NULL;
    size_t cmd_line_maxlen = 0;
    char *cmd_line = NULL;
    u16 vram_mb = kexec_get_vram_mb(uap->vram_gb);
    u16 fw_ver = kexec_get_fw_ver(uap->vram_gb);
    sb_id = kexec_get_sb_id(uap->vram_gb);

    int (*copyin)(const void *uaddr, void *kaddr, size_t len) = td ? kern.copyin : k_copyin;
    int (*copyinstr)(const void *uaddr, void *kaddr, size_t len, size_t *done) = td ? kern.copyinstr : k_copyinstr;
    int (*copyout)(const void *kaddr, void *uaddr, size_t len) = td ? kern.copyout : k_copyout;

    kern.printf("sys_kexec invoked\n");
    kexec_print_banner(fw_ver, vram_mb, sb_id);
    kern.printf("sys_kexec(%p, %zu, %p, %zu, \"%s\")\n", uap->image,
        uap->image_size, uap->initramfs, uap->initramfs_size, uap->cmd_line);

    // Look up our shutdown hook point
    void *icc_query_nowait = kern.icc_query_nowait;
    if (!icc_query_nowait) {
        err = ENOENT;
        goto cleanup;
    }

    // Set GPU frequencies, pstate, CU count, and voltage
    if (kern.gpu_devid_is_9924()){
        // PS4 PRO — Gladius (4 SE)
        //
        // Sony FUN_00cadab0 step 0-1: Neo Mode activation.
        // D18F5x8C enables hardware-level Neo at the northbridge
        // (power/clocks to SE2/SE3).  0x203E bit4=1, bit1=0
        // tells the GPU strap to run in 4-SE Neo mode.
        {
            u32 nb_val;

            nb_val = pci_config_read(0, 0x18, 5, 0x8C);
            pci_config_write(0, 0x18, 5, 0x8C, nb_val | 2);

            nb_val = GPU_RD(0x203E);
            GPU_WR(0x203E, (nb_val & ~0x1B) | 0x10);
        }

        kern.set_gpu_freq(0, 911);  // SCLK 911 MHz
        kern.set_gpu_freq(1, 853);  // MCLK 853 MHz
        kern.set_gpu_freq(2, 711);  // DCEFCLK 711 MHz
        kern.set_gpu_freq(3, 800);  // VCLK 800 MHz
        kern.set_gpu_freq(4, 911);  // Dispclk 911 MHz
        kern.set_gpu_freq(5, 800);  // ACCLK 800 MHz
        kern.set_gpu_freq(6, 984);  // ECLK 984 MHz
        kern.set_gpu_freq(7, 673);  // DMCLK 673 MHz
        kern.set_pstate(3);
        kern.update_vddnp(0x12);
        kern.set_cu_power_gate(0x24); // 36 CUs = 9×4 SE
    } else {
        // PS4 FAT/SLIM — Liverpool (2 SE)
        kern.set_gpu_freq(0, 800);  // SCLK 800 MHz
        kern.set_gpu_freq(1, 673);  // MCLK 673 MHz
        kern.set_gpu_freq(2, 609);  // DCEFCLK 609 MHz
        kern.set_gpu_freq(3, 800);  // VCLK 800 MHz
        kern.set_gpu_freq(4, 800);  // Dispclk 800 MHz
        kern.set_gpu_freq(5, 711);  // ACCLK 711 MHz
        kern.set_gpu_freq(6, 711);  // ECLK 711 MHz
        kern.set_gpu_freq(7, 673);  // DMCLK 673 MHz
        kern.set_pstate(3);
        kern.update_vddnp(0x12);
        kern.set_cu_power_gate(0x12); // 18 CUs = 9×2 SE
    }

    // Copy in kernel image
    image = kernel_alloc_contig(uap->image_size);
    if (!image) {
        kern.printf("Failed to allocate image\n");
        err = ENOMEM;
        goto cleanup;
    }
    err = copyin(uap->image, image, uap->image_size);
    if (err) {
        kern.printf("Failed to copy in image\n");
        goto cleanup;
    }

    // Copy in initramfs
    initramfs = kernel_alloc_contig(initramfs_size + FW_CPIO_SIZE);
    if (!initramfs) {
        kern.printf("Failed to allocate initramfs\n");
        err = ENOMEM;
        goto cleanup;
    }

    err = firmware_extract(((u8*)initramfs));
    if (err < 0) {
        kern.printf("Failed to extract GPU firmware - continuing anyway\n");
    } else {
        firmware_size = err;
    }

    if (initramfs_size) {
        err = copyin(uap->initramfs, initramfs + firmware_size, initramfs_size);
        if (err) {
            kern.printf("Failed to copy in initramfs\n");
            goto cleanup;
        }
    }
    initramfs_size += firmware_size;

    // Copy in cmdline
    cmd_line_maxlen = ((struct boot_params *)image)->hdr.cmdline_size + 1;
    cmd_line = kernel_alloc_contig(cmd_line_maxlen);
    if (!cmd_line) {
        kern.printf("Failed to allocate cmdline\n");
        err = ENOMEM;
        goto cleanup;
    }
    err = copyinstr(uap->cmd_line, cmd_line, cmd_line_maxlen, NULL);
    if (err) {
        kern.printf("Failed to copy in cmdline\n");
        goto cleanup;
    }
    cmd_line[cmd_line_maxlen - 1] = 0;

    kern.printf("\nkexec parameters:\n");
    kern.printf("    Kernel image size:   %zu bytes\n", uap->image_size);
    kern.printf("    Initramfs size:      %zu bytes (%zu from user)\n",
                initramfs_size, uap->initramfs_size);
    kern.printf("    Kernel command line: %s\n", cmd_line);
    kern.printf("    Kernel image buffer: %p\n", image);
    kern.printf("    Initramfs buffer:    %p\n", initramfs);

    // Allocate our boot params
    bp = kernel_alloc_contig(sizeof(*bp));
    if (!bp) {
        kern.printf("Failed to allocate bp\n");
        err = ENOMEM;
        goto cleanup;
    }

    // Initialize bp
    // TODO should probably do this from cpu_quiesce_gate, then bp doesn't
    // need to be allocated here, just placed directly into low mem
    set_nix_info(image, bp, initramfs, initramfs_size, cmd_line, (int)vram_mb);

    prepare_boot_params(bp, image);

    // Hook the final ICC shutdown function
    if (!kernel_hook_install(hook_icc_query_nowait, icc_query_nowait)) {
        kern.printf("Failed to install shutdown hook\n");
        err = EINVAL;
        goto cleanup;
    }
    
    kern.printf("******************************************************\n");
    kern.printf("kexec successfully armed. Please shut down the system.\n");
    kern.printf("******************************************************\n\n");

/*
    kern.printf("\nkern_reboot(0x%x)...\n", RB_POWEROFF);
    if (kern.kern_reboot(RB_POWEROFF) == -1)
        kern.printf("\nkern_reboot(0x%x) failed\n", RB_POWEROFF);
*/
    return 0;

cleanup:
    kernel_free_contig(cmd_line, cmd_line_maxlen);
    kernel_free_contig(bp, sizeof(*bp));
    kernel_free_contig(image, uap->image_size);
    kernel_free_contig(initramfs, uap->initramfs_size);
    return err;

    copyout(NULL, NULL, 0);
}

int kexec_init(void *_early_printf, sys_kexec_t *sys_kexec_ptr)
{
    int rv = 0;

    // potentially needed to write early_printf
    u64 flags = intr_disable();
    u64 wp = write_protect_disable();

    if (kernel_init(_early_printf) < 0) {
        rv = -1;
        goto cleanup;
    }

    kern.printf("Installing sys_kexec to system call #%d\n", SYS_KEXEC);
    kernel_syscall_install(SYS_KEXEC, sys_kexec, SYS_KEXEC_NARGS);
    kern.printf("kexec_init() successful\n\n");

    if (sys_kexec_ptr)
        *sys_kexec_ptr = sys_kexec;

cleanup:
    write_protect_restore(wp);
    if (kern.sched_unpin && wp & CR0_WP) {
        // If we're returning to a state with WP enabled, assume the caller
        // wants the thread unpinned. Else the caller is expected to
        // call kern.sched_unpin() manually.
        kern.sched_unpin();
    }
    intr_restore(flags);
    return rv;
}
