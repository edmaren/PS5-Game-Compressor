/*
 * Game Compressor - standalone payload entrypoint.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "gc_app_installer.h"
#include "gc_api.h"
#include "gc_diag.h"
#include "gc_power_guard.h"
#include "websrv.h"

#ifndef GAME_COMPRESSOR_PORT
#define GAME_COMPRESSOR_PORT 5910
#endif

#define LISTEN_RETRY_COUNT 5
#define LISTEN_RETRY_SECONDS 1

static void
detect_lan_ip(char *out, size_t out_size) {
  struct ifaddrs *ifaddr = NULL;

  snprintf(out, out_size, "<PS5_IP>");
  if(getifaddrs(&ifaddr) != 0) return;

  for(struct ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
    if(!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
    if(ifa->ifa_flags & IFF_LOOPBACK) continue;

    struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
    const char *ip = inet_ntop(AF_INET, &sa->sin_addr, out, out_size);
    if(ip && strncmp(out, "169.254.", 8) != 0) {
      freeifaddrs(ifaddr);
      return;
    }
  }

  freeifaddrs(ifaddr);
}

static void
on_ready(unsigned short port, void *arg) {
  (void)arg;
  char ip[64];
  detect_lan_ip(ip, sizeof(ip));
  gc_checkpoint("web server ready");
  gc_log("web ui ready http://%s:%u/", ip, (unsigned)port);
  gc_launcher_start(ip);
}

static int
send_all_local(int fd, const char *data, size_t size) {
  size_t off = 0;
  while(off < size) {
    ssize_t n = send(fd, data + off, size - off, 0);
    if(n < 0) {
      if(errno == EINTR) continue;
      return -1;
    }
    if(n == 0) return -1;
    off += (size_t)n;
  }
  return 0;
}

static int
request_previous_instance_shutdown(unsigned short port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0) return -1;

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    close(fd);
    return -1;
  }

  const char request[] =
      "GET /api/control/shutdown HTTP/1.1\r\n"
      "Host: 127.0.0.1\r\n"
      "Connection: close\r\n"
      "\r\n";
  int rc = send_all_local(fd, request, sizeof(request) - 1);
  char buf[256];
  while(rc == 0) {
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if(n < 0 && errno == EINTR) continue;
    if(n <= 0) break;
  }
  close(fd);
  return rc;
}

int
main(int argc, char **argv) {
  gc_diag_init();
  gc_diag_install_signal_handlers();
  gc_checkpoint("process start");
  gc_log("PID=%ld argc=%d", (long)getpid(), argc);
  for(int i = 0; i < argc; i++) {
    gc_log("argv[%d]=%s", i, argv && argv[i] ? argv[i] : "(null)");
  }

  if(request_previous_instance_shutdown((unsigned short)GAME_COMPRESSOR_PORT) == 0) {
    gc_log("requested previous instance shutdown");
    sleep(LISTEN_RETRY_SECONDS);
  }

  gc_power_guard_start();
  gc_api_recover_on_startup();

  int rc = -1;
  for(int attempt = 1; attempt <= LISTEN_RETRY_COUNT; attempt++) {
    gc_checkpoint("web listen starting");
    rc = websrv_listen((unsigned short)GAME_COMPRESSOR_PORT, on_ready, NULL);
    gc_log("websrv_listen returned rc=%d attempt=%d", rc, attempt);
    if(rc == 0) return 0;
    sleep(LISTEN_RETRY_SECONDS);
  }
  return rc;
}
