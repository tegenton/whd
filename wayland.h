struct ext_action_binder_v1;
struct wl_display;

typedef struct {
	const char *sym;
	const char *category;
	const char *name;
	const char *description;
	void* command;
} keybind;

struct ext_action_binder_v1* get_binder(struct wl_display *display);
int create_binding(struct ext_action_binder_v1 *binder, keybind);
