#define CMD(c) (const char*[]){"/bin/sh", "-c", c, NULL}

static const char *pidfile = "/var/run/whd.pid";

static const keybind keys[] = {
	{"Print", "screenshot", "selection", "Copy a selection of the screen", CMD("slurp | grim -g - - | wclip")}
};
