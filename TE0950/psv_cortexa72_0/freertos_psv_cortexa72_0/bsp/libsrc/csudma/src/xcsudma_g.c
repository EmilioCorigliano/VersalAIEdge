#include "xcsudma.h"

XCsuDma_Config XCsuDma_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,zynqmp-csudma-1.0", /* compatible */
		0xf11c0000, /* reg */
		0x1, /* xlnx,dma-type */
		0x4083, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"xlnx,zynqmp-csudma-1.0", /* compatible */
		0xf11d0000, /* reg */
		0x2, /* xlnx,dma-type */
		0x4084, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	 { 0 }
};