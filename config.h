#pragma once

static const char *spawnDmenu[] = {"dmenu_run", NULL};
static const char *spawnSt[]    = {"st", NULL};

static struct KeyBinding bindings[] = {
  { XK_q,      Mod1Mask, quit,          {NULL} },
  { XK_Tab,    Mod1Mask, tabNextWindow, {NULL} },
  { XK_p,      Mod1Mask, spawn,         {.args = spawnDmenu} },
  { XK_Return, Mod1Mask, spawn,         {.args = spawnSt} },
};
