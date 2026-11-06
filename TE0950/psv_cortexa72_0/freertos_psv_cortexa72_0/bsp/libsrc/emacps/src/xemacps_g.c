#include "xemacps.h"

XEmacPs_Config XEmacPs_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,versal-gem", /* compatible */
		0xff0c0000, /* reg */
		0x0, /* dma-coherent */
		0x4038, /* interrupts */
		0xf9000000, /* interrupt-parent */
		0x31, /* clocks */
		"rgmii-id", /* phy-mode */
		0x0, /* phy-handle */
		0x0, /* mdioproducer-baseaddr */
		0x0 /* gmiitorgmii-addr */
	},
	 { 0 }
};