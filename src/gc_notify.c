/*
 * Game Compressor - PS5 system notifications.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "gc_notify.h"

typedef struct notify_request {
  char reserved[45];
  char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t *, size_t, int);

static const char *
action_label(const char *action) {
  if(!strcmp(action ? action : "", "compress")) return "Compress";
  if(!strcmp(action ? action : "", "uncompress")) return "Uncompress";
  if(!strcmp(action ? action : "", "validate-repair")) {
    return "Validate and Repair";
  }
  if(!strcmp(action ? action : "", "move-to-usb")) return "Move to USB";
  if(!strcmp(action ? action : "", "move-to-internal")) {
    return "Move to Internal SSD";
  }
  return "Operation";
}

static void
gc_notifyf(const char *fmt, ...) {
  notify_request_t req;
  va_list ap;

  memset(&req, 0, sizeof(req));
  va_start(ap, fmt);
  vsnprintf(req.message, sizeof(req.message), fmt, ap);
  va_end(ap);
  if(req.message[0]) {
    (void)sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
  }
}

void
gc_notify_operation_done(const char *action, const char *game,
                         const char *status, const char *detail) {
  const char *name = (game && game[0]) ? game : "Game";
  const char *label = action_label(action);

  if(!strcmp(status ? status : "", "success")) {
    gc_notifyf("Game Compressor: %s finished for %s", label, name);
  } else if(!strcmp(status ? status : "", "cancelled")) {
    gc_notifyf("Game Compressor: %s cancelled for %s", label, name);
  } else {
    const char *reason = (detail && detail[0]) ? detail : "operation failed";
    gc_notifyf("Game Compressor: %s failed for %s: %s", label, name, reason);
  }
}

void
gc_notify_message(const char *title, const char *detail) {
  const char *label = (title && title[0]) ? title : "Notice";
  if(detail && detail[0]) {
    gc_notifyf("Game Compressor: %s - %s", label, detail);
  } else {
    gc_notifyf("Game Compressor: %s", label);
  }
}
