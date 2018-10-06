#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "windowbuff.h"

// Calculates the length of an array of struct elements
#define LENGTH(x) (sizeof(x) / sizeof(*x))

// Macro to "use" unused parameters against -Wunused-parameter
#define UNUSED(x) (void)(x)

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

void closeCurWindow();
void destroyNotifyHandler(XEvent*);
void keyPressHandler(XEvent*);
void loop();
void mapRequestHandler(XEvent*);
void quit();
void refocusCurWindow();
void setup();
void sendDeleteEvent(Window);
void sigchld(int);
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
static struct WindowBuff *winBuff;

// Stop immediately and drop an error message
void die(const char *msg) {
  printf("meowmeow is kill: %s\n\n", msg);
  exit(EXIT_FAILURE);
}

// Quits the WM and perhaps resets some things in the future
void quit() {
  running = false;

  while (winBuff->head != NULL) {
    Window tmpWindow = winBuff->head->window;

    sendDeleteEvent(tmpWindow);
    remWindow(winBuff, tmpWindow);
  }

  free(winBuff);
}

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

// Send a delete event to the given window.
void sendDeleteEvent(Window window) {
  XEvent xevent;
  xevent.type = ClientMessage;
  xevent.xclient.window = window;
  xevent.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", true);
  xevent.xclient.format = 32;
  xevent.xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", true);
  xevent.xclient.data.l[1] = CurrentTime;

  XSendEvent(dpy, window, false, NoEventMask, &xevent);
}

// Closes current window by asking it to close itself
void closeCurWindow() {
  if (winBuff->head == NULL ||
      !chkWindowExists(winBuff, winBuff->head->window)) {
    return;
  }

  Window window = winBuff->head->window;

  sendDeleteEvent(window);
  remWindow(winBuff, window);
  refocusCurWindow();
}

// Set the focus to the current window
void refocusCurWindow() {
  if (winBuff->head == NULL) {
    return;
  }

  XRaiseWindow(dpy, winBuff->head->window);
  XSetInputFocus(dpy, winBuff->head->window, RevertToParent, CurrentTime);
}

// Toggle to the next window
void tabNextWindow() {
  if (winBuff->head == NULL) {
    // no windows = no work
    return;
  }

  // Set pointer to next window.
  winBuff->head = winBuff->head->next;
  refocusCurWindow();
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

  addWindow(winBuff, xmaprequest.window);
  refocusCurWindow();
}

// Handle the DestroyNotify event for unmapping windows
void destroyNotifyHandler(XEvent *ev) {
  XDestroyWindowEvent xdestroywindow = ev->xdestroywindow;

  if (!chkWindowExists(winBuff, xdestroywindow.window)) {
    return;
  }

  remWindow(winBuff, xdestroywindow.window);
  refocusCurWindow();
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

// Stolen from dwm ^^,
void sigchld(int unused) {
  UNUSED(unused);

  if (signal(SIGCHLD, sigchld) == SIG_ERR) {
    die("can't install SIGCHLD handler");
  }
  while (0 < waitpid(-1, NULL, WNOHANG));
}

// Initial setup based on `config.h`
void setup() {
  // Select default screen and read resolution
  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);

  disWidth = DisplayWidth(dpy, screen);
  disHeight = DisplayHeight(dpy, screen);

  sigchld(0);

  // Register evHandler for:
  // - KeyPress: XGrabKey function
  // - MapRequest: SubstructureRedirectMask
  // - DestroyNotify: (StructureNotifyMask,) SubstructureNotifyMask

  for (uint32_t i = 0; i < LENGTH(bindings); i++) {
    XGrabKey(dpy,
        XKeysymToKeycode(dpy, bindings[i].keysym), bindings[i].modifier,
        DefaultRootWindow(dpy), true, GrabModeAsync, GrabModeAsync);
  }

  XSelectInput(dpy, root, SubstructureRedirectMask|SubstructureNotifyMask);

  running = true;

  winBuff = calloc(1, sizeof(struct WindowBuff));
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
