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

void keyPressHandler(XEvent*);
void loop();
void mapRequestHandler(XEvent*);
void quit();
void setup();
void spawn(const union KeyBindingArg);

#include "config.h"

// Array of size LASTEvent to map events to their handler functions
static void (*evHandler[LASTEvent])(XEvent *ev) = {
  [KeyPress] = keyPressHandler,
  [MapRequest] = mapRequestHandler,
};

static bool running;
static Display *dpy;
static int32_t screen;
static Window root;
static int32_t disWidth;
static int32_t disHeight;

// Spawns a new process for a KeyBindingArg (.args)
void spawn(const union KeyBindingArg arg) {
  if (fork() == 0) {
    // This code is kind of inspired by dwm :^)
    if (dpy) {
      close(ConnectionNumber(dpy));
    }

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
  // Select default screen and read resolution
  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);

  disWidth = DisplayWidth(dpy, screen);
  disHeight = DisplayHeight(dpy, screen);

  // Registers each KeyBinding which will be inspected in loop()
  for (uint32_t i = 0; i < LENGTH(bindings); i++) {
    XGrabKey(dpy,
        XKeysymToKeycode(dpy, bindings[i].keysym), bindings[i].modifier,
        DefaultRootWindow(dpy), true, GrabModeAsync, GrabModeAsync);
  }

  // Register for MapRequest and DestroyNotify
  XSelectInput(dpy, root, SubstructureNotifyMask | SubstructureRedirectMask);

  running = true;
}

// Handles the KeyPress events
void keyPressHandler(XEvent *ev) {
  // Execute the function defined for this KeySym in the KeyBinding
  XKeyEvent xkey = ev->xkey;
  KeySym ks = XkbKeycodeToKeysym(dpy, xkey.keycode, 0, 0);

  for (uint32_t i = 0; i < LENGTH(bindings); i++) {
    if (bindings[i].keysym == ks && bindings[i].modifier == xkey.state) {
      bindings[i].function(bindings[i].arg);
      break;
    }
  }
}

// Handle the MapRequest events for new windows
void mapRequestHandler(XEvent *ev) {
  XMapRequestEvent xmaprequest = ev->xmaprequest;

  XMapWindow(dpy, xmaprequest.window);
  XMoveResizeWindow(dpy, xmaprequest.window, 0, 0, disWidth, disHeight);
}

// The WM's loop where each keystroke or event will be handled
void loop() {
  while (running) {
    // Get the latest event (see setup())..
    XEvent ev;
    XNextEvent(dpy, &ev);

    // ..and call its handler
    if (evHandler[ev.type]) {
      evHandler[ev.type](&ev);
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
