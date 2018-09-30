#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

// Calculates the length of an array of struct elements
#define LENGTH(x) (sizeof(x) / sizeof(*x))

union KeyBindingArg {
  const char **args;
  const int32_t i;
};

struct KeyBinding {
  KeySym keysym;
  uint32_t modifier;
  void (*function)(const union KeyBindingArg);
  const union KeyBindingArg arg;
};

void loop();
void quit();
void setup();
void spawn(const union KeyBindingArg);

#include "config.h"

static bool running;
static Display *dpy;

// Spawns a new process for a KeyBindingArg (.args)
void spawn(const union KeyBindingArg arg) {
  if (fork() == 0) {
    // This code is kind of inspired by dwm :^)
		if (dpy)
			close(ConnectionNumber(dpy));

    // setsid() to keep the children alive, even if meowmeow has stopped
    setsid();
    execvp(arg.args[0], (char**)arg.args);

    exit(EXIT_SUCCESS);
  }
}

// Quits the WM and perhaps resets some things in the future
void quit() {
  running = false;
}

// Initial setup based on `config.h`
void setup() {
  // Registers each KeyBinding which will be inspected in loop()
  for (uint32_t i = 0; i < LENGTH(bindings); i++) {
    XGrabKey(dpy,
        XKeysymToKeycode(dpy, bindings[i].keysym), bindings[i].modifier,
        DefaultRootWindow(dpy), true, GrabModeAsync, GrabModeAsync);
  }

  running = true;
}

// The WM's loop where each keystroke or event will be handled
void loop() {
  while (running) {
    // Get the latest event (see setup())
    XEvent ev;
    XNextEvent(dpy, &ev);

    // We only care about key pressings and not the release
    if (ev.xkey.type != KeyPress) {
      continue;
    }

    // Execute the function defined for this KeySym in the KeyBinding
    KeySym ks = XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0);

    for (uint32_t i = 0; i < LENGTH(bindings); i++) {
      if (bindings[i].keysym == ks) {
        bindings[i].function(bindings[i].arg);
        break;
      }
    }
  }
}

int main(void) {
  if (!(dpy = XOpenDisplay(NULL))) {
    return EXIT_FAILURE;
  }

  setup();
  loop();

  XCloseDisplay(dpy);

  return EXIT_SUCCESS;
}
