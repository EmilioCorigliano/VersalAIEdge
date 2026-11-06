#include "xqspipsu.h"

XQspiPsu_Config XQspiPsu_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,versal-qspi-1.0", /* compatible */
		0xf1030000, /* reg */
		0x11e1a300, /* xlnx,clock-freq */
		0x2, /* xlnx,connection-mode */
		0x2, /* xlnx,bus-width */
		0x0, /* dma-coherent */
		0x407d, /* interrupts */
		0xffff, /* interrupt-parent */
		0x39, /* clocks */
		0x1, /* xlnx,qspi-fbclk */
		0x8 /* xlnx,baud-rate-div */
	},
	 {
		 NULL
	}
};