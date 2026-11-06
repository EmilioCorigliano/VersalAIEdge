#include "xsysmonpsv.h"

XSysMonPsv_Config XSysMonPsv_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,versal-sysmon", /* compatible */
		0xf1270000, /* reg */
		0xffff, /* interrupts */
		0xffff, /* interrupt-parent */
		{
				0, /* reg */
				1, /* reg */
		}, /* child,required */
	},
	 { 0 }
};

const char *XSysMonPsv_Supply_Arr[] = {
	"VAUX_CH0",
	"VAUX_CH1",
};
