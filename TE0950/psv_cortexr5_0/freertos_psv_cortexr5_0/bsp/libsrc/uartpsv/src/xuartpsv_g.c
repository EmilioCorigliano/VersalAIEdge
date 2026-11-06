#include "xuartpsv.h"

XUartPsv_Config XUartPsv_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"arm,pl011", /* compatible */
		0xff010000, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		0x0, /* ModemPinsConnected */
		0xe1000, /* xlnx,baudrate */
		0x4013, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	 { 0 }
};