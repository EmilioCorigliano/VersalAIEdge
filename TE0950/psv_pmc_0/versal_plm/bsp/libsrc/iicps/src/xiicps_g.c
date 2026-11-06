#include "xiicps.h"

XIicPs_Config XIicPs_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"cdns,i2c-r1p14", /* compatible */
		0xff030000, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		0x400f, /* interrupts */
		0xffff, /* interrupt-parent */
		0x63 /* clocks */
	},
	{
		"cdns,i2c-r1p14", /* compatible */
		0xf1000000, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		0x407b, /* interrupts */
		0xffff, /* interrupt-parent */
		0x3e /* clocks */
	},
	 {
		 NULL
	}
};