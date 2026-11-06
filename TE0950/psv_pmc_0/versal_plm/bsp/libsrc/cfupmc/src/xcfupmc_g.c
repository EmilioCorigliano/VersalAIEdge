#include "xcfupmc.h"

XCfupmc_Config XCfupmc_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,psv-pmc-cfu-apb-1.0", /* compatible */
		0xf12b0000 /* reg */
	},
	 {
		 NULL
	}
};