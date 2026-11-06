#include "xtrngpsv.h"

XTrngpsv_Config XTrngpsv_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,psv-pmc-trng-1.0", /* compatible */
		0xf1230000 /* reg */
	},
	 {
		 NULL
	}
};