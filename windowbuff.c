#include "windowbuff.h"

void addWindow(struct WindowBuff *wb, Window window) {
  // Create a new WindowEle for given Window
  struct WindowEle *tmp = calloc(1, sizeof(struct WindowEle));
  tmp->window = window;
  tmp->next   = (wb->head != NULL) ? wb->head : tmp;

  // Remap lasts WindowBuff's next pointer to future head
  if (wb->head != NULL) {
    struct WindowEle *we = wb->head;
    for (; we->next != wb->head; we = we->next);
    we->next = tmp;
  }

  // Enque at new head
  wb->head = tmp;
}

void remWindow(struct WindowBuff *wb, Window window) {
  if (!chkWindowExists(wb, window)) {
    return;
  }

  if (wb->head == wb->head->next) {
    // There is only one window, must be ours
    free(wb->head);
    wb->head = NULL;
  } else {
    // Search for entry which succsessor points to our window
    struct WindowEle *we = wb->head, *del;
    for (; we->next->window != window; we = we->next);
    del = we->next;
    we->next = del->next;

    if (wb->head == del) {
      wb->head = we;
    }

    free(del);
  }
}

bool chkWindowExists(struct WindowBuff *wb, Window window) {
  if (wb->head == NULL) {
    return false;
  }

  if (wb->head == wb->head->next) {
    return wb->head->window == window;
  }

  for (struct WindowEle *we = wb->head; we->next != wb->head; we = we->next) {
    if (we->window == window) {
      return true;
    }
  }
  return false;
}
