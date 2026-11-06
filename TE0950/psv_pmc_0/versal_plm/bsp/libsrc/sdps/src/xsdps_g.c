#include "xsdps.h"

XSdPs_Config XSdPs_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,versal-8.9a", /* compatible */
		0xf1040000, /* reg */
		0xbebc200, /* xlnx,sdio-clk-freq-hz */
		0x0, /* xlnx,has-cd */
		0x0, /* xlnx,has-wp */
		0x8, /* xlnx,bus-width */
		0x1, /* xlnx,mio-bank */
		0x0, /* xlnx,has-emio */
		0x1, /* xlnx,slot-type */
		0x0, /* xlnx,is-cache-coherent */
		0x3b, /* clocks */
		0x2c, /* xlnx,clk-50-sdr-itap-dly */
		0x5, /* xlnx,clk-50-sdr-otap-dly */
		0x1e, /* xlnx,clk-50-ddr-itap-dly */
		0x5, /* xlnx,clk-50-ddr-otap-dly */
		0x0, /* xlnx,clk-100-sdr-otap-dly */
		0x2, /* xlnx,clk-200-sdr-otap-dly */
		0x0 /* xlnx,clk-200-ddr-otap-dly */
	},
	{
		"xlnx,versal-8.9a", /* compatible */
		0xf1050000, /* reg */
		0xbebc200, /* xlnx,sdio-clk-freq-hz */
		0x1, /* xlnx,has-cd */
		0x0, /* xlnx,has-wp */
		0x4, /* xlnx,bus-width */
		0x1, /* xlnx,mio-bank */
		0x0, /* xlnx,has-emio */
		0x2, /* xlnx,slot-type */
		0x0, /* xlnx,is-cache-coherent */
		0x3c, /* clocks */
		0x2c, /* xlnx,clk-50-sdr-itap-dly */
		0x4, /* xlnx,clk-50-sdr-otap-dly */
		0x0, /* xlnx,clk-50-ddr-itap-dly */
		0x0, /* xlnx,clk-50-ddr-otap-dly */
		0x0, /* xlnx,clk-100-sdr-otap-dly */
		0x0, /* xlnx,clk-200-sdr-otap-dly */
		0x0 /* xlnx,clk-200-ddr-otap-dly */
	},
	 {
		 NULL
	}
};