#include "xrtcpsu.h"

XRtcPsu_Config XRtcPsu_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,zynqmp-rtc", /* compatible */
		0xf12a0000, /* reg */
		{0x408e,  0x408f}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	 { 0 }
};