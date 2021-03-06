/*
 * RUN: clang %cflags -emit-llvm -S %s -o %t.ll
 * RUN: soaap -o %t.soaap.ll %t.ll > %t.out
 * RUN: FileCheck %s -input-file %t.out
 *
 * CHECK: Running Soaap Pass
 */
#include "soaap.h"

#include <fcntl.h>
#include <unistd.h>

/*
 * An extremely simple model of Capsicum's cap_enter():
 * disallow all system calls except for a whitelist.
 *
 * We can't statically express all of the Capsicum behaviour
 * (e.g., namespace subsetting), but we can express some things.
 *
 * TODO: tag the cap_enter() declaration with these syscalls so that we
 *       don't have to manually annotate every call site.
 */
#define SOAAP_CAPSICUM_CAPABILITY_MODE \
	__soaap_sandboxed_region_start("cap_sandbox"); \
	__soaap_limit_syscalls(read, write, openat, exit);

int	cap_enter(void);


int main(int argc, char** argv)
{
	int passwd, root;

	/*
	 * This is ok: we still have ambient authority.
	 *
	 * CHECK-NOT: +++ Line 39 of file
	 */
	passwd = open("/etc/passwd", O_RDONLY);

	/*
	 * Enter capability mode: from now on, only system calls in
	 * cap_enter()'s annotation above are allowed.
	 *
	 * TODO: allow this annotation to be attached to cap_enter()'s
	 *       declaration, then eventually incorporate this domain
	 *       knowledge into SOAAP itself (as well as seccomp-bpf, etc.).
	 *
	 * CHECK-NOT: +++ Line 51 of file
	 */
	cap_enter();
	SOAAP_CAPSICUM_CAPABILITY_MODE

	/*
	 * This is not ok: cap_enter() disallows "open".
	 *
	 * CHECK: performs system call "open" but it is not allowed to
	 * CHECK: +++ Line 60 of file
	 */
	root = open("/", O_RDONLY);

	return 0;
}
