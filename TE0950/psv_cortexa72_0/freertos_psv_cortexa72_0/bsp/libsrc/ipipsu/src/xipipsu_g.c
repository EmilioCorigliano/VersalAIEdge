#include "xipipsu.h"

XIpiPsu_Config XIpiPsu_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,versal-ipi-mailbox", /* compatible */
		0xff330000, /* reg */
		0x4, /* xlnx,ipi-bitmask */
		0x2, /* xlnx,ipi-buf-index */
		0x401e, /* interrupts */
		0xf9000000, /* interrupt-parent */
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
		0xff340000, /* reg */
		0x8, /* xlnx,ipi-bitmask */
		0x3, /* xlnx,ipi-buf-index */
		0x401f, /* interrupts */
		0xf9000000, /* interrupt-parent */
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
		0xff350000, /* reg */
		0x10, /* xlnx,ipi-bitmask */
		0x4, /* xlnx,ipi-buf-index */
		0x4020, /* interrupts */
		0xf9000000, /* interrupt-parent */
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
		0xff360000, /* reg */
		0x20, /* xlnx,ipi-bitmask */
		0x5, /* xlnx,ipi-buf-index */
		0x4021, /* interrupts */
		0xf9000000, /* interrupt-parent */
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
		0xff370000, /* reg */
		0x40, /* xlnx,ipi-bitmask */
		0x6, /* xlnx,ipi-buf-index */
		0x4022, /* interrupts */
		0xf9000000, /* interrupt-parent */
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
		0xff380000, /* reg */
		0x80, /* xlnx,ipi-bitmask */
		0x7, /* xlnx,ipi-buf-index */
		0x4023, /* interrupts */
		0xf9000000, /* interrupt-parent */
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
		0xff3a0000, /* reg */
		0x200, /* xlnx,ipi-bitmask */
		0xffff, /* xlnx,ipi-buf-index */
		0x4024, /* interrupts */
		0xf9000000, /* interrupt-parent */
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
	 { 0 }
};