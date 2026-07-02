#pragma once
#define kernel_offset_xfast_syscall 0x1c0
#define kernel_offset_allproc 0x1B244E0
#define kernel_offset_vmspace_acquire_ref 0x3E74E0
#define kernel_offset_vmspace_free 0x3E7310
#define kernel_offset_printf 0x430AE0
#define kernel_offset_kmem_alloc 0x1B3F0
#define kernel_offset_kernel_map 0x1B243E0
#define kernel_offset_sysent 0x10FC4D0
#define kernel_offset_proc_rwmem 0x173770

#define kernel_patch_kmem_alloc_1 0x1B4BC
#define kernel_patch_kmem_alloc_2 0x1B4C4
