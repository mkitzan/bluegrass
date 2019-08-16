#define _GNU_SOURCE
#define BUFFER_SIZE 256

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <signal.h>
#include <fcntl.h>

void sigio_hook(int signal, siginfo_t *info, void *context) {
	printf("SIGIO trigger: %d\nSI_SIGIO code: %d\nFD attached:   %d\n", 
	signal, info->si_code, info->si_fd);
}

int main() {
	char buffer[BUFFER_SIZE];
	int flags;
	struct sigaction action;

	action.sa_handler = NULL;
	action.sa_sigaction = sigio_hook;
	action.sa_flags = SA_SIGINFO;
	action.sa_restorer = NULL;
	sigemptyset(&action.sa_mask);

	printf("Installing signal action to SIGIO\n");

	if(sigaction(SIGIO, &action, NULL) == -1) {
		err(STDERR_FILENO, "Error installing action to signal handler");
	}

	printf("Installing flags to STDIN file descriptor\n");

	flags = fcntl(STDIN_FILENO, F_GETFL);
	if(fcntl(STDIN_FILENO, F_SETFL, flags | O_ASYNC) == -1) {
		err(STDERR_FILENO, "Error installing O_ASYNC flag");
	}

	if(fcntl(STDIN_FILENO, F_SETOWN, getpid()) == -1) {
		err(STDERR_FILENO, "Error claiming ownership of STDIN signals");
	}

	if(fcntl(STDIN_FILENO, F_SETSIG, SIGIO) == -1) {
		err(STDERR_FILENO, "Error configuring SA_SIGINFO information");
	}

	printf("Enter test string\n");

	if(read(STDIN_FILENO, buffer, BUFFER_SIZE) == -1) {
		err(STDERR_FILENO, "Error reading STDIN");
	}

	return 0;
}
