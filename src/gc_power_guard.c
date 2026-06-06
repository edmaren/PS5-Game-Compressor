/*
 * Game Compressor - keep PS5 auto power-down timer reset during operations.
 */

#include "gc_power_guard.h"

#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

#include "gc_diag.h"
#include "transfer_internal.h"

#define POWER_GUARD_TICK_SECONDS 30
#define POWER_GUARD_IDLE_SLEEP_SECONDS 2
#define POWER_GUARD_ACTIVE_SLEEP_SECONDS 1
#define POWER_GUARD_FAIL_LOG_SECONDS 60

extern int sceShellCoreUtilResetAutoPowerDownTimer(void);

static pthread_once_t g_power_guard_once = PTHREAD_ONCE_INIT;

static void *
power_guard_thread_main(void *arg) {
  (void)arg;

  int was_busy = 0;
  int success_logged = 0;
  time_t next_tick = 0;
  time_t last_fail_log = 0;

  for(;;) {
    int busy = atomic_load(&g_job.busy) != 0;
    time_t now = time(NULL);

    if(!busy) {
      if(was_busy) {
        gc_log("power guard idle");
      }
      was_busy = 0;
      success_logged = 0;
      next_tick = 0;
      sleep(POWER_GUARD_IDLE_SLEEP_SECONDS);
      continue;
    }

    if(!was_busy) {
      gc_log("power guard active");
      next_tick = 0;
      last_fail_log = 0;
    }

    if(next_tick == 0 || now >= next_tick) {
      int rc = sceShellCoreUtilResetAutoPowerDownTimer();
      if(rc < 0) {
        if(last_fail_log == 0 ||
           now - last_fail_log >= POWER_GUARD_FAIL_LOG_SECONDS) {
          gc_log("power guard reset auto power-down failed rc=0x%08X",
                 (unsigned)rc);
          last_fail_log = now;
        }
      } else if(!success_logged) {
        gc_log("power guard reset auto power-down timer rc=0x%08X",
               (unsigned)rc);
        success_logged = 1;
      }
      next_tick = now + POWER_GUARD_TICK_SECONDS;
    }

    was_busy = 1;
    sleep(POWER_GUARD_ACTIVE_SLEEP_SECONDS);
  }

  return NULL;
}

static void
power_guard_start_once(void) {
  pthread_t thread;
  int rc = pthread_create(&thread, NULL, power_guard_thread_main, NULL);
  if(rc != 0) {
    gc_log("power guard pthread_create failed rc=%d", rc);
    return;
  }
  pthread_detach(thread);
  gc_log("power guard started");
}

void
gc_power_guard_start(void) {
  pthread_once(&g_power_guard_once, power_guard_start_once);
}
