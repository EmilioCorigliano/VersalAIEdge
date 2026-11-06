#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#define _AIE_LOG_LOCAL_TEST_
#include "klog.h"
#include "klogr.h"
int main() {
	char log[256];
	void *address = (void *)log;
	int run_round = 0;
	for(int i = 0; i< 1024; i++) {
		wlog(0,"2******mmstart 1 round = %d \n", ++run_round);
	}
	wlog_reset();
	wlog(0,"0----This is a test %d", 2);
	wlog(0,"0----This is a test %f", 3.14);

	char * s = rlog(0,0,0);
	printf("%s\n",s);

	return 0;
}
