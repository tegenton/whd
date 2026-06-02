#include <errno.h>                 // for errno
#include <fcntl.h>                 // for open
#include <signal.h>                // for kill
#include <stdio.h>                 // for perror, printf, fprintf
#include <stdlib.h>                // for atol, free, malloc, exit
#include <sys/stat.h>              // for S_IRUSR, S_IWUSR
#include <sys/types.h>             // for pid_t
#include <unistd.h>                // for daemon, fork
#include <wayland-client-core.h>   // for wl_display
#include "ext-action-binder-v1.h"  // for ext_action_binder

#include "wayland.h"
#include "config.h"

int kill_daemon();
int restart_daemon();
int start_daemon();

int
kill_daemon() {
	int len, pidfd;
	char pidstr[25];
	pid_t pid;

	if ((pidfd = open(pidfile, O_RDONLY)) < 0) {
		perror("Could not open pidfile");
		goto cleanup;
	}

	if ((len = read(pidfd, pidstr, 25)) < 0) {
		perror("Could not read pid from file");
		goto cleanup;
	}

	if (!(pid = atol(pidstr))) {
		perror("pid file has no pid");
		goto cleanup;
	}

	if (kill(pid, SIGTERM) && errno != ESRCH) {
		perror("Failed to send signal");
		goto cleanup;
	}

	if (unlink(pidfile)) {
		perror("Failed to remove pidfile");
		goto cleanup;
	}

	close(pidfd);
	return EXIT_SUCCESS;
cleanup:
	close(pidfd);
	return EXIT_FAILURE;
}

int
restart_daemon() {
	if (kill_daemon())
		return EXIT_FAILURE;
	return start_daemon();
}

int start_daemon() {
	int len, pidfd;
	char pidstr[25];
	struct wl_display *display = NULL;
	struct ext_action_binder_v1 *binder = NULL;

	if ((pidfd = open(pidfile, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR)) < 0) {
		perror("Could not create pidfile");
		return EXIT_FAILURE;
	}

	if (!(display = wl_display_connect(NULL))) {
		perror("Could not connect to display");
		goto cleanup;
	}

	if (!(binder = get_binder(display))) {
		perror("Could not bind action binder");
		goto cleanup;
	}

	for (int i = 0; i < sizeof(keys) / sizeof(keybind); i++) {
		if (create_binding(binder, keys[i])) {
			perror("Could not create binding");
			goto cleanup;
		}
	}

	ext_action_binder_v1_commit(binder);

	if (wl_display_roundtrip(display) < 0) {
		perror("Could not commit bindings");
		goto cleanup;
	}

	if (daemon(0, 0)) {
		perror("Could not daemonize");
		goto cleanup;
	}

	len = snprintf(pidstr, 20, "%d", getpid());

	if (write(pidfd, pidstr, len) < len) {
		perror("Could not write pid to file");
		goto cleanup;
	}

	close(pidfd);

	while (wl_display_dispatch(display) > -1);
	perror("Could not process wayland messages");

cleanup:
	if (binder)
		ext_action_binder_v1_destroy(binder);
	if (display)
		wl_display_disconnect(display);
	return EXIT_FAILURE;
}

int
main(int argc, char **argv) {
	int opt;
	int (*mode)(void) = &start_daemon;

	while ((opt = getopt(argc, argv, "krs")) != -1) {
		switch (opt) {
		case 'k':
			mode = &kill_daemon;
			break;
		case 'r':
			mode = &restart_daemon;
			break;
		case 's':
			mode = &start_daemon;
			break;
		default:
			fprintf(stderr, "Usage: %s [-k|-r|-s]\n", argv[0]);
			return EXIT_FAILURE;
		}
	}
	return mode();
}
