#include "xospipsv.h"

XOspiPsv_Config XOspiPsv_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,versal-ospi-1.0", /* compatible */
		0xf1010000, /* reg */
		0xbebc200, /* xlnx,ospi-clk-freq-hz */
		0x0, /* xlnx,is-cache-coherent */
		0x0, /* xlnx,ospi-mode */
		0x407c, /* interrupts */
		0xf9000000, /* interrupt-parent */
		0x0 /* xlnx,ospi-bus-width */
	},
	 { 0 }
};