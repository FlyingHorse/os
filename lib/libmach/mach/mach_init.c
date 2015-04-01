#include <sys/cdefs.h>
#include <sys/types.h>

#include <mach/mach.h>
#include <mach/boolean.h>
#include <mach/mach_traps.h>
#include <mach/mach_init.h>
#include <mach/mach_types.h>
#include <mach/mach_port.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <machine/mach/ndr_def.h>
static void mach_init(void) __attribute__((constructor));
mach_port_t mach_reply_port(void);
mach_port_t task_self_trap(void);

extern mach_port_t _task_reply_port;

extern mach_port_t bootstrap_port;
mach_port_t mach_task_self_ = MACH_PORT_NULL;
 __attribute__((visibility("hidden"))) mach_port_t _task_reply_port;


static void
mach_init(void)
{
	pid_t pid;
	char *root_flag;
	int root_bootstrap;
	kern_return_t kr;
	static bool mach_inited = false;
	if (!mach_inited) {
		root_flag = getenv("ROOT_BOOTSTRAP");
		root_bootstrap = (root_flag != NULL) && (strcmp(root_flag, "T") == 0);

		pid = getpid();
		mach_task_self_ = task_self_trap();
        _task_reply_port = mach_reply_port();
		if (pid != 1 && root_bootstrap == false) {
			kr = task_get_special_port(mach_task_self_, TASK_BOOTSTRAP_PORT, &bootstrap_port);
			if (kr != KERN_SUCCESS) {
				fprintf(stderr, "failed to bootstrap\n");
				abort();
			}
		}
		mach_inited = true;
	}
}
