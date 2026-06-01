#include <fcntl.h>                    // for open, O_CLOEXEC, O_CREAT, O_RDWR
#include <stdint.h>                   // for uint32_t
#include <stdio.h>                    // for perror, printf, fprintf
#include <stdlib.h>                   // for free, malloc, exit
#include <string.h>                   // for strcmp
#include <sys/stat.h>                 // for S_IRUSR, S_IWUSR
#include <sys/file.h>                 // for flock
#include <unistd.h>                   // for daemon, fork
#include <wayland-client-core.h>      // for wl_display
#include <wayland-client-protocol.h>  // for wl_registry_listener
#include <wayland-util.h>             // for wl_interface
#include "ext-action-binder-v1.h"     // for ext_action_binding_v1_listener

struct ext_action_binder_v1;
struct ext_action_binding_v1;
struct wl_display;
struct wl_registry;

#include "config.h"

void on_global_add(void *data, struct wl_registry *registry, unsigned int name, const char *iface, unsigned int ver);
void on_global_remove(void *data, struct wl_registry *registry, unsigned int name);

void on_bind(void *data, struct ext_action_binding_v1 *ext_action_binding_v1, const char *trigger);
void on_reject(void *data, struct ext_action_binding_v1 *ext_action_binding_v1);
void on_trigger(void *data, struct ext_action_binding_v1 *ext_action_binding_v1, uint32_t time, enum ext_action_binding_v1_trigger_type type);

struct ext_action_binder_v1* get_binder(struct wl_display *display);

void
on_global_add(void *data, struct wl_registry *registry, unsigned int name, const char *iface, unsigned int ver) {
	struct ext_action_binder **binder = (struct ext_action_binder**) data;

	if (!strcmp(iface, ext_action_binder_v1_interface.name))
		*binder = wl_registry_bind(registry, name, &ext_action_binder_v1_interface, ver);
}

void
on_global_remove(void *data, struct wl_registry *registry, unsigned int name) {
}

void
on_bind(void *data, struct ext_action_binding_v1 *ext_action_binding_v1, const char *trigger) {
}

void
on_reject(void *data, struct ext_action_binding_v1 *ext_action_binding_v1) {
	char **cmd = data;

	fprintf(stderr, "binding for");
	while (*cmd)
		fprintf(stderr, "%s ", *(cmd++));
	fprintf(stderr, "was rejected\n");
}

void
on_trigger(void *data, struct ext_action_binding_v1 *ext_action_binding_v1, uint32_t time, enum ext_action_binding_v1_trigger_type type) {
	char **cmd = data;
	if (!fork()) {
		execv(cmd[0], cmd);
	}
}

struct ext_action_binder_v1*
get_binder(struct wl_display *display) {
	struct wl_registry *registry = NULL;
	struct wl_registry_listener *registry_listener = NULL;
	struct ext_action_binder_v1 *binder = NULL;

	if (!(registry = wl_display_get_registry(display))) {
		//perror("Could not get registry");
		goto cleanup;
	}

	if (!(registry_listener = malloc(sizeof(struct wl_registry_listener)))) {
		//perror("Could not allocate registry listener");
		goto cleanup;
	}

	registry_listener->global = &on_global_add;
	registry_listener->global_remove = &on_global_remove;

	if (wl_registry_add_listener(registry, registry_listener, &binder) < 0) {
		//perror("Could not install listener");
		goto cleanup;
	}

	if (wl_display_roundtrip(display) < 0) {
		//perror("Could not process pending requests");
		goto cleanup;
	}

	if (!binder) {
		//perror("Could not bind action binder");
		goto cleanup;
	}

cleanup:
	free(registry_listener);
	wl_registry_destroy(registry);
	return binder;
}

int
create_binding(struct ext_action_binder_v1 *binder, keybind key) {
	struct ext_action_binding_v1 *binding = NULL;
	struct ext_action_binding_v1_listener *action_listener = NULL;

	if (!(binding = ext_action_binder_v1_create_binding(binder))) {
		perror("Could not create binding");
		goto cleanup;
	}

	ext_action_binding_v1_set_keyboard_hint(binding, key.sym);
	ext_action_binding_v1_set_name(binding, key.category, key.name);
	ext_action_binding_v1_set_description(binding, key.description);

	if (!(action_listener = malloc(sizeof(struct ext_action_binding_v1_listener)))) {
		perror("Could not allocate action listener");
		goto cleanup;
	}

	action_listener->bound = &on_bind;
	action_listener->rejected = &on_reject;
	action_listener->triggered = &on_trigger;

	if (ext_action_binding_v1_add_listener(binding, action_listener, key.command) < 0) {
		perror("Could not add listener to binding");
		goto cleanup;
	}

	return 0;

cleanup:
	free(action_listener);
	ext_action_binding_v1_destroy(binding);
	return -1;
}

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
