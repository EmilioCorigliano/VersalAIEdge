#include "xscugic.h"

XScuGic_Config XScuGic_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"arm,gic-v3", /* compatible */
		0xf9000000,
		0xf9080000, /* reg */
		{{0U}} /* Handler-table */
	},
	{
		"arm,gic-v3-its", /* compatible */
		0xf9020000,
		0x0, /* reg */
		{{0U}} /* Handler-table */
	},
	 { 0 }
};