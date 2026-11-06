#include "xzdma.h"

XZDma_Config XZDma_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,zynqmp-dma-1.0", /* compatible */
		0xffa80000, /* reg */
		0x1, /* xlnx,dma-mode */
		0x0, /* dma-coherent */
		0x403c, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"xlnx,zynqmp-dma-1.0", /* compatible */
		0xffa90000, /* reg */
		0x1, /* xlnx,dma-mode */
		0x0, /* dma-coherent */
		0x403d, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"xlnx,zynqmp-dma-1.0", /* compatible */
		0xffaa0000, /* reg */
		0x1, /* xlnx,dma-mode */
		0x0, /* dma-coherent */
		0x403e, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"xlnx,zynqmp-dma-1.0", /* compatible */
		0xffab0000, /* reg */
		0x1, /* xlnx,dma-mode */
		0x0, /* dma-coherent */
		0x403f, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"xlnx,zynqmp-dma-1.0", /* compatible */
		0xffac0000, /* reg */
		0x1, /* xlnx,dma-mode */
		0x0, /* dma-coherent */
		0x4040, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"xlnx,zynqmp-dma-1.0", /* compatible */
		0xffad0000, /* reg */
		0x1, /* xlnx,dma-mode */
		0x0, /* dma-coherent */
		0x4041, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"xlnx,zynqmp-dma-1.0", /* compatible */
		0xffae0000, /* reg */
		0x1, /* xlnx,dma-mode */
		0x0, /* dma-coherent */
		0x4042, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"xlnx,zynqmp-dma-1.0", /* compatible */
		0xffaf0000, /* reg */
		0x1, /* xlnx,dma-mode */
		0x0, /* dma-coherent */
		0x4043, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	 { 0 }
};