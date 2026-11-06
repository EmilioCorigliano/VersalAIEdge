#include "xmm2s.h"

XMm2s_Config XMm2s_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,mm2s-1.0", /* compatible */
		0xa40d0000 /* reg */
	},
	 {
		 NULL
	}
};