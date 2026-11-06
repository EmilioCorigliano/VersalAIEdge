#include "xipipsu.h"

XIpiPsu_Config XIpiPsu_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,versal-ipi-mailbox", /* compatible */
		0xff320000, /* reg */
		0x2, /* xlnx,ipi-bitmask */
		0x1, /* xlnx,ipi-buf-index */
		0x401b, /* interrupts */
		0xffff, /* interrupt-parent */
		0xa, /* xlnx,ipi-target-count */
		{
			{
				4, /* xlnx,ipi-bitmask */
				2 /* xlnx,ipi-buf-index */
			},
			{
				8, /* xlnx,ipi-bitmask */
				3 /* xlnx,ipi-buf-index */
			},
			{
				16, /* xlnx,ipi-bitmask */
				4 /* xlnx,ipi-buf-index */
			},
			{
				32, /* xlnx,ipi-bitmask */
				5 /* xlnx,ipi-buf-index */
			},
			{
				64, /* xlnx,ipi-bitmask */
				6 /* xlnx,ipi-buf-index */
			},
			{
				128, /* xlnx,ipi-bitmask */
				7 /* xlnx,ipi-buf-index */
			},
			{
				512, /* xlnx,ipi-bitmask */
				65535 /* xlnx,ipi-buf-index */
			},
			{
				2, /* xlnx,ipi-bitmask */
				1 /* xlnx,ipi-buf-index */
			},
			{
				256, /* xlnx,ipi-bitmask */
				65535 /* xlnx,ipi-buf-index */
			},
			{
				1, /* xlnx,ipi-bitmask */
				0 /* xlnx,ipi-buf-index */
			},
		}, /* child,required */
	},
	{
		"xlnx,versal-ipi-mailbox", /* compatible */
		0xff390000, /* reg */
		0x100, /* xlnx,ipi-bitmask */
		0xffff, /* xlnx,ipi-buf-index */
		0x401c, /* interrupts */
		0xffff, /* interrupt-parent */
		0xa, /* xlnx,ipi-target-count */
		{
			{
				4, /* xlnx,ipi-bitmask */
				2 /* xlnx,ipi-buf-index */
			},
			{
				8, /* xlnx,ipi-bitmask */
				3 /* xlnx,ipi-buf-index */
			},
			{
				16, /* xlnx,ipi-bitmask */
				4 /* xlnx,ipi-buf-index */
			},
			{
				32, /* xlnx,ipi-bitmask */
				5 /* xlnx,ipi-buf-index */
			},
			{
				64, /* xlnx,ipi-bitmask */
				6 /* xlnx,ipi-buf-index */
			},
			{
				128, /* xlnx,ipi-bitmask */
				7 /* xlnx,ipi-buf-index */
			},
			{
				512, /* xlnx,ipi-bitmask */
				65535 /* xlnx,ipi-buf-index */
			},
			{
				2, /* xlnx,ipi-bitmask */
				1 /* xlnx,ipi-buf-index */
			},
			{
				256, /* xlnx,ipi-bitmask */
				65535 /* xlnx,ipi-buf-index */
			},
			{
				1, /* xlnx,ipi-bitmask */
				0 /* xlnx,ipi-buf-index */
			},
		}, /* child,required */
	},
	 {
		 NULL
	}
};