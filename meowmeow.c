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

struct WindowList {
  Window window;
  struct WindowList *next;
};

void addWindow(Window);
void destroyNotifyHandler(XEvent*);
void keyPressHandler(XEvent*);
void loop();
void mapRequestHandler(XEvent*);
void quit();
void refocusCurrentWindow();
void remWindow(Window);
void setup();
void spawn(const union KeyBindingArg);
void tabNextWindow();

#include "config.h"

// Array of size LASTEvent to map events to their handler functions
static void (*evHandler[LASTEvent])(XEvent *ev) = {
  [KeyPress] = keyPressHandler,
  [MapRequest] = mapRequestHandler,
  [DestroyNotify] = destroyNotifyHandler,
};

static bool running;
static Display *dpy;
static int32_t screen;
static Window root;
static int32_t disWidth;
static int32_t disHeight;
static struct WindowList *curWindow;

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

// Set the focus to the current window
void refocusCurrentWindow() {
  if (curWindow == NULL) {
    return;
  }

  XSetInputFocus(dpy, curWindow->window, RevertToParent, CurrentTime);
  XRaiseWindow(dpy, curWindow->window);
}

// Register a new window (map it)
void addWindow(Window window) {
  struct WindowList *tmpWindow = calloc(1, sizeof(struct WindowList));
  tmpWindow->window = window;
  tmpWindow->next   = (curWindow == NULL) ? tmpWindow : curWindow;

  // Remap next pointer of our ring buffer
  if (curWindow != NULL) {
    // Last entry iff next points to curWindow
    struct WindowList *wl = curWindow;
    for (; wl->next != curWindow; wl = wl->next);
    wl->next = tmpWindow;
  }

  curWindow = tmpWindow;

  refocusCurrentWindow();
}

// Unregister a window (unmap)
void remWindow(Window window) {
  if (curWindow == curWindow->next) {
    // There is only one window, must be ours
    free(curWindow);
    curWindow = NULL;
  } else {
    // Search for entry which succsessor points to our window
    struct WindowList *wl = curWindow, *del;
    for (; wl->next->window != window; wl = wl->next);
    del = wl->next;
    wl->next  = del->next;

    if (curWindow == del) {
      curWindow = wl;
    }

    free(del);
  }

  refocusCurrentWindow();
}

// Toggle to the next window
void tabNextWindow() {
  if (curWindow == NULL) {
    // no windows = no work
    return;
  }

  // Set pointer to next window.
  curWindow = curWindow->next;

  refocusCurrentWindow();
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

  addWindow(xmaprequest.window);
}

// Handle the DestroyNotify event for unmapping windows
void destroyNotifyHandler(XEvent *ev) {
  XDestroyWindowEvent xdestroywindow = ev->xdestroywindow;

  remWindow(xdestroywindow.window);
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
  // This is kind of buggy, time to RTFM; TODO
	XSelectInput(dpy, root,
      EnterWindowMask|FocusChangeMask|PropertyChangeMask| \
      StructureNotifyMask|SubstructureRedirectMask);

  running = true;

  curWindow = NULL;
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
