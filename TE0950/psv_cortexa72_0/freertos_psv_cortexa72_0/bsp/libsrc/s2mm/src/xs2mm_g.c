#include "xs2mm.h"

XS2mm_Config XS2mm_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,s2mm-1.0", /* compatible */
		0xa40e0000 /* reg */
	},
	{
		"xlnx,s2mm-1.0", /* compatible */
		0xa40f0000 /* reg */
	},
	 {
		 NULL
	}
};