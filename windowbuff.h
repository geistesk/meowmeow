#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include <X11/Xlib.h>

struct WindowEle {
  Window window;
  struct WindowEle *next;
};

struct WindowBuff {
  struct WindowEle *head;
};

void addWindow(struct WindowBuff *wb, Window window);
void remWindow(struct WindowBuff *wb, Window window);
bool chkWindowExists(struct WindowBuff *wb, Window window);
