#include <stddef.h> // for NULL

#define CMD(c) (const char*[]){"/bin/sh", "-c", c, NULL}

typedef struct {
	const char *sym;
	const char *category;
	const char *name;
	const char *description;
	void* command;
} keybind;

static const keybind keys[] = {
	{"Print", "screenshot", "selection", "Copy a selection of the screen", CMD("slurp | grim -g - - | wclip")}
};
