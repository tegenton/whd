#include <fcntl.h>                    // for open, O_CLOEXEC, O_CREAT, O_RDWR
#include <stdio.h>                    // for perror, printf, fprintf
#include <stdlib.h>                   // for free, malloc, exit
#include <sys/stat.h>                 // for S_IRUSR, S_IWUSR
#include <sys/file.h>                 // for flock
#include <unistd.h>                   // for daemon, fork
#include <wayland-client-core.h>      // for wl_display
#include "ext-action-binder-v1.h"     // for ext_action_binder

#include "wayland.h"
#include "config.h"

int
main(int argc, char **argv) {
	int pidfd, len;
	char pidstr[20];
	struct wl_display *display = NULL;
	struct ext_action_binder_v1 *binder = NULL;

	if ((pidfd = open(pidfile, O_CREAT | O_CLOEXEC | O_RDWR, S_IRUSR | S_IWUSR)) < 0) {
		perror("Could not create pidfile");
		goto cleanup;
	}

	if (flock(pidfd, LOCK_EX | LOCK_NB) < 0) {
		perror("Could not lock pidfile");
		goto cleanup;
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

	while (wl_display_dispatch(display) > -1);
	perror("Could not process wayland messages");

cleanup:
	if (binder)
		ext_action_binder_v1_destroy(binder);
	if (display)
		wl_display_disconnect(display);
	return EXIT_FAILURE;
}
