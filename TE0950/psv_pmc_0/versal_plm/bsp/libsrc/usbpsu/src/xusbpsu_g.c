#include "xusbpsu.h"

XUsbPsu_Config XUsbPsu_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"snps,dwc3", /* compatible */
		0xfe200000, /* reg */
		0x0, /* dma-coherent */
		0x0, /* xlnx,enable-superspeed */
		{0x4016,  0x4016,  0x401a}, /* interrupts */
		0xffff /* interrupt-parent */
	},
	 {
		 NULL
	}
};