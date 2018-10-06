#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>

struct WindowEle {
  Window window;
  struct WindowEle *next;
};

struct WindowBuff {
  struct WindowEle *head;
};

void addWindow(struct WindowBuff*, Window);
void remWindow(struct WindowBuff*, Window);
bool chkWindowExists(struct WindowBuff*, Window);
