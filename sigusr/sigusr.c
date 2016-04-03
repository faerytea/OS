#define ONE_AND_EXIT

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#ifdef ONE_AND_EXIT
#include <stdlib.h>

int secs = 10;
#endif

void handler(int signum, siginfo_t * siginfo, void * useless) {
	char s = 1;
	switch(signum) {
		case SIGUSR1: --s;
		case SIGUSR2: ++s;
	}
	printf("SIGUSR%d from %d\n", s, siginfo->si_pid);
	#ifdef ONE_AND_EXIT
	exit(0);
	#endif
	#ifdef STRICT_10_SEC
	const struct sigaction sa;
	sa.sa_handler = SIGIGN;
	sigaction(SIGUSR1, sa, NULL);
	sigaction(SIGUSR2, sa, NULL);
	#endif
}

int main() {
	struct sigaction sa;
	sa.sa_sigaction = &handler;
	sa.sa_flags = SA_SIGINFO;
	sigaddset(&sa.sa_mask, SIGUSR1);
	sigaddset(&sa.sa_mask, SIGUSR2);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	secs = sleep(10);
	#ifdef STRICT_10_SEC
	sleep(secs);
	#endif
	if (secs == 0) printf("No signals were caught\n");
	return 0;
}
