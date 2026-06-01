#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include "ext-action-binder-v1.h"

#include "config.h"

struct ext_action_binder_v1 *binder;

void
on_global_add(void *data, struct wl_registry *registry, unsigned int name, const char *iface, unsigned int ver) {
	if (!strcmp(iface, ext_action_binder_v1_interface.name))
		binder = wl_registry_bind(registry, name, &ext_action_binder_v1_interface, ver);
}

void
on_global_remove(void *data, struct wl_registry *registry, unsigned int name) {
}

void
on_bind(void *data, struct ext_action_binding_v1 *ext_action_binding_v1, const char *trigger) {
	printf("bound %s\n", trigger);
}

void
on_reject(void *data, struct ext_action_binding_v1 *ext_action_binding_v1) {
	fprintf(stderr, "Binding was rejected!\n");
	exit(EXIT_FAILURE);
}

void
on_trigger(void *data, struct ext_action_binding_v1 *ext_action_binding_v1, uint32_t time, uint32_t type) {
	printf("triggered\n");
}

int
main(int argc, char **argv) {
	struct wl_display *display = NULL;
	struct wl_registry *registry = NULL;
	struct wl_registry_listener *registry_listener = NULL;
	struct ext_action_binding_v1 *binding = NULL;
	struct ext_action_binding_v1_listener *action_listener = NULL;
	int ret = EXIT_FAILURE;

	if (!(display = wl_display_connect(NULL))) {
		perror("Could not connect to display");
		goto cleanup;
	}

	if (!(registry = wl_display_get_registry(display))) {
		perror("Could not get registry");
		goto cleanup;
	}

	if (!(registry_listener = malloc(sizeof(struct wl_registry_listener)))) {
		perror("Could not allocate registry listener");
		goto cleanup;
	}

	registry_listener->global = &on_global_add;
	registry_listener->global_remove = &on_global_remove;

	if (wl_registry_add_listener(registry, registry_listener, NULL) < 0) {
		perror("Could not install listener");
		goto cleanup;
	}

	if (wl_display_roundtrip(display) < 0) {
		perror("Could not process pending requests");
		goto cleanup;
	}

	if (!binder) {
		perror("Could not bind action binder");
		goto cleanup;
	}

	if (!(binding = ext_action_binder_v1_create_binding(binder))) {
		perror("Could not create binding");
		goto cleanup;
	}

	ext_action_binding_v1_set_keyboard_hint(binding, "ctrl+a");
	ext_action_binding_v1_set_name(binding, "cat", "name");
	ext_action_binding_v1_set_description(binding, "sample text");

	if (!(action_listener = malloc(sizeof(struct ext_action_binding_v1_listener)))) {
		perror("Could not allocate action listener");
		goto cleanup;
	}

	action_listener->bound = &on_bind;
	action_listener->rejected = &on_reject;
	action_listener->triggered = &on_trigger;

	if (ext_action_binding_v1_add_listener(binding, action_listener, NULL) < 0) {
		perror("Could not add listener to binding");
		goto cleanup;
	}
	ext_action_binder_v1_commit(binder);

	while (wl_display_roundtrip(display) > 0);
	perror("display roundtrip");

cleanup:
	free(action_listener);
	ext_action_binding_v1_destroy(binding);
	ext_action_binder_v1_destroy(binder);
	free(registry_listener);
	wl_registry_destroy(registry);
	wl_display_disconnect(display);
	return ret;
}
