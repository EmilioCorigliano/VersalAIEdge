set(axi_noc_0_ddr_memory_DDR_LOW_0 "0x40000;0x7ffc0000")
set(versal_cips_0_pspmc_0_psv_ocm_ram_0_memory_0 "0xfffc0000;0x40000")
set(DDR axi_noc_0_ddr_memory_DDR_LOW_0)
set(CODE axi_noc_0_ddr_memory_DDR_LOW_0)
set(DATA axi_noc_0_ddr_memory_DDR_LOW_0)
set(TOTAL_MEM_CONTROLLERS "axi_noc_0_ddr_memory_DDR_LOW_0;versal_cips_0_pspmc_0_psv_ocm_ram_0_memory_0")
set(MEMORY_SECTION "MEMORY
{
	psv_r5_0_atcm_MEM_0 : ORIGIN = 0x0, LENGTH = 0x10000
	psv_r5_0_atcm_lockstep_MEM_0 : ORIGIN = 0xFFE10000, LENGTH = 0x10000
	psv_r5_0_btcm_MEM_0 : ORIGIN = 0x20000, LENGTH = 0x10000
	psv_r5_0_btcm_lockstep_MEM_0 : ORIGIN = 0xFFE30000, LENGTH = 0x10000
	psv_pmc_ram_psv_pmc_ram : ORIGIN = 0xF2000000, LENGTH = 0x20000
	psv_r5_0_data_cache_MEM_0 : ORIGIN = 0xFFE50000, LENGTH = 0x10000
	psv_r5_0_instruction_cache_MEM_0 : ORIGIN = 0xFFE40000, LENGTH = 0x10000
	psv_r5_1_data_cache_MEM_0 : ORIGIN = 0xFFED0000, LENGTH = 0x10000
	psv_r5_1_instruction_cache_MEM_0 : ORIGIN = 0xFFEC0000, LENGTH = 0x10000
	psv_r5_0_atcm_global_MEM_0 : ORIGIN = 0xFFE00000, LENGTH = 0x10000
	psv_r5_1_atcm_global_MEM_0 : ORIGIN = 0xFFE90000, LENGTH = 0x10000
	psv_r5_0_btcm_global_MEM_0 : ORIGIN = 0xFFE20000, LENGTH = 0x10000
	psv_r5_1_btcm_global_MEM_0 : ORIGIN = 0xFFEB0000, LENGTH = 0x10000
	axi_noc_0_ddr_memory_DDR_LOW_0 : ORIGIN = 0x40000, LENGTH = 0x7ffc0000
	versal_cips_0_pspmc_0_psv_ocm_ram_0_memory_0 : ORIGIN = 0xfffc0000, LENGTH = 0x40000
}")
set(STACK_SIZE 0x2000)
set(HEAP_SIZE 0x2000)
