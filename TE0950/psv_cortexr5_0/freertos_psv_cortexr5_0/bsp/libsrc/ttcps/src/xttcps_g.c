#include "xttcps.h"

XTtcPs_Config XTtcPs_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"cdns,ttc", /* compatible */
		0xff0e0000, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x4025}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff0e0004, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x4026}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff0e0008, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x4027}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff0f0000, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x4028}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff0f0004, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x4029}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff0f0008, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x402a}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff100000, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x402b}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff100004, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x402c}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff100008, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x402d}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff110000, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x402e}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff110004, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x402f}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	{
		"cdns,ttc", /* compatible */
		0xff110008, /* reg */
		0x5f5e100, /* xlnx,clock-freq */
		{0x4030}, /* interrupts */
		0xf9000000 /* interrupt-parent */
	},
	 { 0 }
};