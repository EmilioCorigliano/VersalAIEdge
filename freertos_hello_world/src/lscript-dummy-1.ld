/******************************************************************************
* Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x5000;
_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0xfff0;

_ABORT_STACK_SIZE = DEFINED(_ABORT_STACK_SIZE) ? _ABORT_STACK_SIZE : 2048;
_SUPERVISOR_STACK_SIZE = DEFINED(_SUPERVISOR_STACK_SIZE) ? _SUPERVISOR_STACK_SIZE : 4096;
_IRQ_STACK_SIZE = DEFINED(_IRQ_STACK_SIZE) ? _IRQ_STACK_SIZE : 2048;
_FIQ_STACK_SIZE = DEFINED(_FIQ_STACK_SIZE) ? _FIQ_STACK_SIZE : 2048;
_UNDEF_STACK_SIZE = DEFINED(_UNDEF_STACK_SIZE) ? _UNDEF_STACK_SIZE : 2048;

MEMORY
{
	psv_r5_0_atcm_MEM_0                          : ORIGIN = 0x0, LENGTH = 0x10000
	psv_r5_0_btcm_MEM_0                          : ORIGIN = 0x20000, LENGTH = 0x10000
	axi_noc_0_ddr_memory_DDR_LOW_0               : ORIGIN = 0x40000, LENGTH = 0x3ffc0000
   axi_noc_0_ddr_memory_DDR_LOW_1               : ORIGIN = 0x40000000, LENGTH = 0x40000000
	psv_pmc_ram_psv_pmc_ram                      : ORIGIN = 0xF2000000, LENGTH = 0x20000
	psv_r5_0_atcm_global_MEM_0                   : ORIGIN = 0xFFE00000, LENGTH = 0x10000
	psv_r5_0_atcm_lockstep_MEM_0                 : ORIGIN = 0xFFE10000, LENGTH = 0x10000
	psv_r5_0_btcm_global_MEM_0                   : ORIGIN = 0xFFE20000, LENGTH = 0x10000
	psv_r5_0_btcm_lockstep_MEM_0                 : ORIGIN = 0xFFE30000, LENGTH = 0x10000
	psv_r5_0_instruction_cache_MEM_0             : ORIGIN = 0xFFE40000, LENGTH = 0x10000
	psv_r5_0_data_cache_MEM_0                    : ORIGIN = 0xFFE50000, LENGTH = 0x10000
	psv_r5_1_atcm_global_MEM_0                   : ORIGIN = 0xFFE90000, LENGTH = 0x10000
	psv_r5_1_btcm_global_MEM_0                   : ORIGIN = 0xFFEB0000, LENGTH = 0x10000
	psv_r5_1_instruction_cache_MEM_0             : ORIGIN = 0xFFEC0000, LENGTH = 0x10000
	psv_r5_1_data_cache_MEM_0                    : ORIGIN = 0xFFED0000, LENGTH = 0x10000
	versal_cips_0_pspmc_0_psv_ocm_ram_0_memory_0 : ORIGIN = 0xfffc0000, LENGTH = 0x40000
}

__atcm0_start = ORIGIN(psv_r5_0_atcm_MEM_0);
__atcm0_end   = ORIGIN(psv_r5_0_atcm_MEM_0) + LENGTH(psv_r5_0_atcm_MEM_0);

__btcm0_start = ORIGIN(psv_r5_0_btcm_MEM_0);
__btcm0_end   = ORIGIN(psv_r5_0_btcm_MEM_0) + LENGTH(psv_r5_0_btcm_MEM_0);

__pmcram_start = ORIGIN(psv_pmc_ram_psv_pmc_ram);
__pmcram_end   = ORIGIN(psv_pmc_ram_psv_pmc_ram) + LENGTH(psv_pmc_ram_psv_pmc_ram);

__atcm_global0_start = ORIGIN(psv_r5_0_atcm_global_MEM_0);
__atcm_global0_end   = ORIGIN(psv_r5_0_atcm_global_MEM_0) + LENGTH(psv_r5_0_atcm_global_MEM_0);

__atcm_global1_start = ORIGIN(psv_r5_1_atcm_global_MEM_0);
__atcm_global1_end   = ORIGIN(psv_r5_1_atcm_global_MEM_0) + LENGTH(psv_r5_1_atcm_global_MEM_0);

__atcm_lockstep0_start = ORIGIN(psv_r5_0_atcm_lockstep_MEM_0);
__atcm_lockstep0_end   = ORIGIN(psv_r5_0_atcm_lockstep_MEM_0) + LENGTH(psv_r5_0_atcm_lockstep_MEM_0);

__btcm_global0_start = ORIGIN(psv_r5_0_btcm_global_MEM_0);
__btcm_global0_end   = ORIGIN(psv_r5_0_btcm_global_MEM_0) + LENGTH(psv_r5_0_btcm_global_MEM_0);

__btcm_global1_start = ORIGIN(psv_r5_1_btcm_global_MEM_0);
__btcm_global1_end   = ORIGIN(psv_r5_1_btcm_global_MEM_0) + LENGTH(psv_r5_1_btcm_global_MEM_0);

__btcm_lockstep0_start = ORIGIN(psv_r5_0_btcm_lockstep_MEM_0);
__btcm_lockstep0_end   = ORIGIN(psv_r5_0_btcm_lockstep_MEM_0) + LENGTH(psv_r5_0_btcm_lockstep_MEM_0);

__ocm_start = ORIGIN(versal_cips_0_pspmc_0_psv_ocm_ram_0_memory_0);
__ocm_end   = ORIGIN(versal_cips_0_pspmc_0_psv_ocm_ram_0_memory_0) + LENGTH(versal_cips_0_pspmc_0_psv_ocm_ram_0_memory_0);


/* Where to put the data in the chip */
REGION_ALIAS("NOT_EXPOSED_LOCATION", axi_noc_0_ddr_memory_DDR_LOW_1)

/* Specify the default entry point to the program */

ENTRY(_boot)

/* Define the sections, and where they are mapped in memory */

SECTIONS
{
.vectors : {
   KEEP (*(.vectors))
   *(.boot)
} > psv_r5_0_atcm_MEM_0

.bootdata : {
   *(.bootdata)
} > NOT_EXPOSED_LOCATION

.text : {
   *(.text)
   *(.text.*)
   *(.gnu.linkonce.t.*)
   *(.plt)
   *(.gnu_warning)
   *(.gcc_execpt_table)
   *(.glue_7)
   *(.glue_7t)
   *(.vfp11_veneer)
   *(.ARM.extab)
   *(.gnu.linkonce.armextab.*)
} > NOT_EXPOSED_LOCATION

.note.gnu.build-id : {
   KEEP (*(.note.gnu.build-id))
} > NOT_EXPOSED_LOCATION


.init : {
   KEEP (*(.init))
} > NOT_EXPOSED_LOCATION

.fini : {
   KEEP (*(.fini))
} > NOT_EXPOSED_LOCATION

.interp : {
   KEEP (*(.interp))
} > NOT_EXPOSED_LOCATION

.note-ABI-tag : {
   KEEP (*(.note-ABI-tag))
} > NOT_EXPOSED_LOCATION

.rodata : {
   __rodata_start = .;
   *(.rodata)
   *(.rodata.*)
   *(.gnu.linkonce.r.*)
   __rodata_end = .;
} > NOT_EXPOSED_LOCATION

.rodata1 : {
   __rodata1_start = .;
   *(.rodata1)
   *(.rodata1.*)
   __rodata1_end = .;
} > NOT_EXPOSED_LOCATION

.sdata2 : {
   __sdata2_start = .;
   *(.sdata2)
   *(.sdata2.*)
   *(.gnu.linkonce.s2.*)
   __sdata2_end = .;
} > NOT_EXPOSED_LOCATION

.sbss2 : {
   __sbss2_start = .;
   *(.sbss2)
   *(.sbss2.*)
   *(.gnu.linkonce.sb2.*)
   __sbss2_end = .;
} > NOT_EXPOSED_LOCATION

.data : {
   __data_start = .;
   *(.data)
   *(.data.*)
   *(.gnu.linkonce.d.*)
   *(.jcr)
   *(.got)
   *(.got.plt)
   __data_end = .;
} > NOT_EXPOSED_LOCATION

.data1 : {
   __data1_start = .;
   *(.data1)
   *(.data1.*)
   __data1_end = .;
} > NOT_EXPOSED_LOCATION

.got : {
   *(.got)
} > NOT_EXPOSED_LOCATION

.ctors : {
   __CTOR_LIST__ = .;
   ___CTORS_LIST___ = .;
   KEEP (*crtbegin.o(.ctors))
   KEEP (*(EXCLUDE_FILE(*crtend.o) .ctors))
   KEEP (*(SORT(.ctors.*)))
   KEEP (*(.ctors))
   __CTOR_END__ = .;
   ___CTORS_END___ = .;
} > NOT_EXPOSED_LOCATION

.dtors : {
   __DTOR_LIST__ = .;
   ___DTORS_LIST___ = .;
   KEEP (*crtbegin.o(.dtors))
   KEEP (*(EXCLUDE_FILE(*crtend.o) .dtors))
   KEEP (*(SORT(.dtors.*)))
   KEEP (*(.dtors))
   __DTOR_END__ = .;
   ___DTORS_END___ = .;
} > NOT_EXPOSED_LOCATION

.fixup : {
   __fixup_start = .;
   *(.fixup)
   __fixup_end = .;
} > NOT_EXPOSED_LOCATION

.eh_frame : {
   *(.eh_frame)
} > NOT_EXPOSED_LOCATION

.eh_framehdr : {
   __eh_framehdr_start = .;
   *(.eh_framehdr)
   __eh_framehdr_end = .;
} > NOT_EXPOSED_LOCATION

.gcc_except_table : {
   *(.gcc_except_table)
} > NOT_EXPOSED_LOCATION

.mmu_tbl (ALIGN(16384)) : {
   __mmu_tbl_start = .;
   *(.mmu_tbl)
   __mmu_tbl_end = .;
} > NOT_EXPOSED_LOCATION

.ARM.exidx : {
   __exidx_start = .;
   *(.ARM.exidx*)
   *(.gnu.linkonce.armexidix.*.*)
   __exidx_end = .;
} > NOT_EXPOSED_LOCATION

.preinit_array : {
   __preinit_array_start = .;
   KEEP (*(SORT(.preinit_array.*)))
   KEEP (*(.preinit_array))
   __preinit_array_end = .;
} > NOT_EXPOSED_LOCATION

.init_array : {
   __init_array_start = .;
   KEEP (*(SORT(.init_array.*)))
   KEEP (*(.init_array))
   __init_array_end = .;
} > NOT_EXPOSED_LOCATION

.fini_array : {
   __fini_array_start = .;
   KEEP (*(SORT(.fini_array.*)))
   KEEP (*(.fini_array))
   __fini_array_end = .;
} > NOT_EXPOSED_LOCATION

.drvcfg_sec : {
    . = ALIGN(8);
     __drvcfgsecdata_start = .;
    KEEP (*(.drvcfg_sec))
    __drvcfgsecdata_end = .;
    __drvcfgsecdata_size = __drvcfgsecdata_end - __drvcfgsecdata_start;
} > NOT_EXPOSED_LOCATION

.ARM.attributes : {
   __ARM.attributes_start = .;
   *(.ARM.attributes)
   __ARM.attributes_end = .;
} > NOT_EXPOSED_LOCATION

.sdata : {
   __sdata_start = .;
   *(.sdata)
   *(.sdata.*)
   *(.gnu.linkonce.s.*)
   __sdata_end = .;
} > NOT_EXPOSED_LOCATION

.sbss (NOLOAD) : {
   __sbss_start = .;
   *(.sbss)
   *(.sbss.*)
   *(.gnu.linkonce.sb.*)
   __sbss_end = .;
} > NOT_EXPOSED_LOCATION

.tdata : {
   __tdata_start = .;
   *(.tdata)
   *(.tdata.*)
   *(.gnu.linkonce.td.*)
   __tdata_end = .;
} > NOT_EXPOSED_LOCATION

.tbss : {
   __tbss_start = .;
   *(.tbss)
   *(.tbss.*)
   *(.gnu.linkonce.tb.*)
   __tbss_end = .;
} > NOT_EXPOSED_LOCATION

.bss (NOLOAD) : {
   . = ALIGN(4);
   __bss_start__ = .;
   *(.bss)
   *(.bss.*)
   *(.gnu.linkonce.b.*)
   *(COMMON)
   . = ALIGN(4);
   __bss_end__ = .;
} > NOT_EXPOSED_LOCATION

_SDA_BASE_ = __sdata_start + ((__sbss_end - __sdata_start) / 2 );

_SDA2_BASE_ = __sdata2_start + ((__sbss2_end - __sdata2_start) / 2 );

/* Generate Stack and Heap definitions */

.heap (NOLOAD) : {
   . = ALIGN(16);
   _heap = .;
   HeapBase = .;
   _heap_start = .;
   . += _HEAP_SIZE;
   _heap_end = .;
   HeapLimit = .;
} > NOT_EXPOSED_LOCATION

.stack (NOLOAD) : {
   . = ALIGN(16);
   _stack_end = .;
   . += _STACK_SIZE;
   _stack = .;
   __stack = _stack;
   . = ALIGN(16);
   _irq_stack_end = .;
   . += _IRQ_STACK_SIZE;
   __irq_stack = .;
   _supervisor_stack_end = .;
   . += _SUPERVISOR_STACK_SIZE;
   . = ALIGN(16);
   __supervisor_stack = .;
   _abort_stack_end = .;
   . += _ABORT_STACK_SIZE;
   . = ALIGN(16);
   __abort_stack = .;
   _fiq_stack_end = .;
   . += _FIQ_STACK_SIZE;
   . = ALIGN(16);
   __fiq_stack = .;
   _undef_stack_end = .;
   . += _UNDEF_STACK_SIZE;
   . = ALIGN(16);
   __undef_stack = .;
} > NOT_EXPOSED_LOCATION

end = .;
}
