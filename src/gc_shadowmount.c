/*
 * Game Compressor - ShadowMountPlus compatibility hints.
 */

#include "gc_shadowmount.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define SHADOWMOUNT_DIR "/data/shadowmount"
#define SHADOWMOUNT_CONFIG SHADOWMOUNT_DIR "/config.ini"
#define SHADOWMOUNT_CONFIG_TMP SHADOWMOUNT_DIR "/config.ini.game-compressor.tmp"
#define SHADOWMOUNT_AUTOTUNE SHADOWMOUNT_DIR "/autotune.ini"
#define SHADOWMOUNT_AUTOTUNE_TMP SHADOWMOUNT_DIR "/autotune.ini.game-compressor.tmp"
#define SHADOWMOUNT_MANUAL_LIST SHADOWMOUNT_DIR "/manual.lst"
#define SHADOWMOUNT_PFSC_SECTOR 65536U

static void
set_err(char *err, size_t err_size, const char *message) {
  if(err && err_size && !err[0]) {
    snprintf(err, err_size, "%s", message ? message : "shadowmount error");
  }
}

static void
set_errno_err(char *err, size_t err_size, const char *context) {
  if(err && err_size && !err[0]) {
    snprintf(err, err_size, "%s: %s", context ? context : "shadowmount",
             strerror(errno));
  }
}

static int
mkdir_if_needed(const char *path) {
  if(mkdir(path, 0777) == 0) {
    chmod(path, 0777);
    return 0;
  }
  if(errno == EEXIST) return 0;
  return -1;
}

static const char *
base_name(const char *path) {
  const char *slash = strrchr(path ? path : "", '/');
  return slash ? slash + 1 : path;
}

static int
normalize_image_name(const char *path_or_name, char *out, size_t out_size) {
  const char *name = base_name(path_or_name);
  size_t len = name ? strlen(name) : 0;
  if(!out || out_size == 0 || len == 0 || len >= out_size) return -1;
  for(size_t i = 0; i < len; i++) {
    unsigned char ch = (unsigned char)name[i];
    if(ch < 0x20 || ch >= 0x7f || ch == ':' || ch == '/' ||
       ch == '\\') {
      return -1;
    }
  }
  memcpy(out, name, len + 1);
  return 0;
}

static char *
trim_left(char *s) {
  while(s && *s && isspace((unsigned char)*s)) s++;
  return s;
}

static void
trim_right(char *s) {
  size_t len = s ? strlen(s) : 0;
  while(len > 0 && isspace((unsigned char)s[len - 1])) {
    s[--len] = 0;
  }
}

static int
line_matches_image_sector(const char *line, const char *image_name) {
  char tmp[512];
  int n = snprintf(tmp, sizeof(tmp), "%s", line ? line : "");
  if(n < 0 || (size_t)n >= sizeof(tmp)) return 0;

  char *p = trim_left(tmp);
  if(!*p || *p == '#' || *p == ';') return 0;
  char *eq = strchr(p, '=');
  if(!eq) return 0;
  *eq = 0;
  trim_right(p);
  if(strcasecmp(p, "image_sector")) return 0;

  char *value = trim_left(eq + 1);
  char *colon = strrchr(value, ':');
  if(!colon) return 0;
  *colon = 0;
  trim_right(value);
  value = trim_left(value);
  return !strcasecmp(value, image_name);
}

static int
line_matches_image_mode(const char *line, const char *image_name) {
  char tmp[512];
  int n = snprintf(tmp, sizeof(tmp), "%s", line ? line : "");
  if(n < 0 || (size_t)n >= sizeof(tmp)) return 0;

  char *p = trim_left(tmp);
  if(!*p || *p == '#' || *p == ';') return 0;
  char *eq = strchr(p, '=');
  if(!eq) return 0;
  *eq = 0;
  trim_right(p);
  if(strcasecmp(p, "image_ro") && strcasecmp(p, "image_rw")) return 0;

  char *value = trim_left(eq + 1);
  trim_right(value);
  return !strcasecmp(value, image_name);
}

static int
upsert_image_mode_hint(const char *path_or_name, int read_only,
                       char *err, size_t err_size) {
  char image_name[256];
  char line[384];
  FILE *in = NULL;
  FILE *out = NULL;
  int found = 0;
  int rc = -1;

  if(normalize_image_name(path_or_name, image_name, sizeof(image_name)) != 0) {
    set_err(err, err_size, "bad ShadowMount image mode hint name");
    return -1;
  }
  if(mkdir_if_needed(SHADOWMOUNT_DIR) != 0) {
    set_errno_err(err, err_size, "create /data/shadowmount");
    return -1;
  }

  int n = snprintf(line, sizeof(line), "image_%s=%s\n",
                   read_only ? "ro" : "rw", image_name);
  if(n < 0 || (size_t)n >= sizeof(line)) {
    set_err(err, err_size, "ShadowMount image mode hint line too long");
    return -1;
  }

  in = fopen(SHADOWMOUNT_CONFIG, "r");
  out = fopen(SHADOWMOUNT_CONFIG_TMP, "w");
  if(!out) {
    if(in) fclose(in);
    set_errno_err(err, err_size, "open ShadowMount config temp");
    return -1;
  }

  if(in) {
    char existing[512];
    while(fgets(existing, sizeof(existing), in)) {
      if(line_matches_image_mode(existing, image_name)) {
        if(!found && fputs(line, out) == EOF) {
          set_errno_err(err, err_size, "write ShadowMount image mode hint");
          goto done;
        }
        found = 1;
        continue;
      }
      if(fputs(existing, out) == EOF) {
        set_errno_err(err, err_size, "write ShadowMount config");
        goto done;
      }
    }
    if(ferror(in)) {
      set_errno_err(err, err_size, "read ShadowMount config");
      goto done;
    }
  }

  if(!found && fputs(line, out) == EOF) {
    set_errno_err(err, err_size, "append ShadowMount image mode hint");
    goto done;
  }
  if(fflush(out) != 0 || fsync(fileno(out)) != 0) {
    set_errno_err(err, err_size, "flush ShadowMount config");
    goto done;
  }
  if(fclose(out) != 0) {
    out = NULL;
    set_errno_err(err, err_size, "close ShadowMount config temp");
    goto done;
  }
  out = NULL;
  if(rename(SHADOWMOUNT_CONFIG_TMP, SHADOWMOUNT_CONFIG) != 0) {
    set_errno_err(err, err_size, "replace ShadowMount config");
    goto done;
  }
  chmod(SHADOWMOUNT_CONFIG, 0666);
  rc = 0;

done:
  if(in) fclose(in);
  if(out) fclose(out);
  if(rc != 0) unlink(SHADOWMOUNT_CONFIG_TMP);
  return rc;
}

static int
upsert_sector_hint(const char *path_or_name, unsigned sector,
                   char *err, size_t err_size) {
  char image_name[256];
  char line[384];
  FILE *in = NULL;
  FILE *out = NULL;
  int found = 0;
  int rc = -1;

  if(normalize_image_name(path_or_name, image_name, sizeof(image_name)) != 0) {
    set_err(err, err_size, "bad ShadowMount image hint name");
    return -1;
  }
  if(sector != 4096U && sector != 8192U && sector != 16384U &&
     sector != 32768U && sector != 65536U) {
    set_err(err, err_size, "bad ShadowMount sector hint");
    return -1;
  }
  if(mkdir_if_needed(SHADOWMOUNT_DIR) != 0) {
    set_errno_err(err, err_size, "create /data/shadowmount");
    return -1;
  }

  int n = snprintf(line, sizeof(line), "image_sector=%s:%u\n",
                   image_name, sector);
  if(n < 0 || (size_t)n >= sizeof(line)) {
    set_err(err, err_size, "ShadowMount hint line too long");
    return -1;
  }

  in = fopen(SHADOWMOUNT_AUTOTUNE, "r");
  out = fopen(SHADOWMOUNT_AUTOTUNE_TMP, "w");
  if(!out) {
    if(in) fclose(in);
    set_errno_err(err, err_size, "open ShadowMount autotune temp");
    return -1;
  }

  if(in) {
    char existing[512];
    while(fgets(existing, sizeof(existing), in)) {
      if(line_matches_image_sector(existing, image_name)) {
        if(!found && fputs(line, out) == EOF) {
          set_errno_err(err, err_size, "write ShadowMount hint");
          goto done;
        }
        found = 1;
        continue;
      }
      if(fputs(existing, out) == EOF) {
        set_errno_err(err, err_size, "write ShadowMount autotune");
        goto done;
      }
    }
    if(ferror(in)) {
      set_errno_err(err, err_size, "read ShadowMount autotune");
      goto done;
    }
  }

  if(!found && fputs(line, out) == EOF) {
    set_errno_err(err, err_size, "append ShadowMount hint");
    goto done;
  }
  if(fflush(out) != 0 || fsync(fileno(out)) != 0) {
    set_errno_err(err, err_size, "flush ShadowMount autotune");
    goto done;
  }
  if(fclose(out) != 0) {
    out = NULL;
    set_errno_err(err, err_size, "close ShadowMount autotune temp");
    goto done;
  }
  out = NULL;
  if(rename(SHADOWMOUNT_AUTOTUNE_TMP, SHADOWMOUNT_AUTOTUNE) != 0) {
    set_errno_err(err, err_size, "replace ShadowMount autotune");
    goto done;
  }
  chmod(SHADOWMOUNT_AUTOTUNE, 0666);
  rc = 0;

done:
  if(in) fclose(in);
  if(out) fclose(out);
  if(rc != 0) unlink(SHADOWMOUNT_AUTOTUNE_TMP);
  return rc;
}

static int
remove_sector_hint(const char *path_or_name, char *err, size_t err_size) {
  char image_name[256];
  FILE *in = NULL;
  FILE *out = NULL;
  int removed = 0;
  int rc = -1;

  if(normalize_image_name(path_or_name, image_name, sizeof(image_name)) != 0) {
    set_err(err, err_size, "bad ShadowMount image hint name");
    return -1;
  }
  if(mkdir_if_needed(SHADOWMOUNT_DIR) != 0) {
    set_errno_err(err, err_size, "create /data/shadowmount");
    return -1;
  }

  in = fopen(SHADOWMOUNT_AUTOTUNE, "r");
  if(!in) {
    if(errno == ENOENT) return 0;
    set_errno_err(err, err_size, "open ShadowMount autotune");
    return -1;
  }
  out = fopen(SHADOWMOUNT_AUTOTUNE_TMP, "w");
  if(!out) {
    fclose(in);
    set_errno_err(err, err_size, "open ShadowMount autotune temp");
    return -1;
  }

  char existing[512];
  while(fgets(existing, sizeof(existing), in)) {
    if(line_matches_image_sector(existing, image_name)) {
      removed = 1;
      continue;
    }
    if(fputs(existing, out) == EOF) {
      set_errno_err(err, err_size, "write ShadowMount autotune");
      goto done;
    }
  }
  if(ferror(in)) {
    set_errno_err(err, err_size, "read ShadowMount autotune");
    goto done;
  }
  if(fflush(out) != 0 || fsync(fileno(out)) != 0) {
    set_errno_err(err, err_size, "flush ShadowMount autotune");
    goto done;
  }
  if(fclose(out) != 0) {
    out = NULL;
    set_errno_err(err, err_size, "close ShadowMount autotune temp");
    goto done;
  }
  out = NULL;
  if(rename(SHADOWMOUNT_AUTOTUNE_TMP, SHADOWMOUNT_AUTOTUNE) != 0) {
    set_errno_err(err, err_size, "replace ShadowMount autotune");
    goto done;
  }
  chmod(SHADOWMOUNT_AUTOTUNE, 0666);
  if(removed) {
    sync();
  }
  rc = 0;

done:
  if(in) fclose(in);
  if(out) fclose(out);
  if(rc != 0) unlink(SHADOWMOUNT_AUTOTUNE_TMP);
  return rc;
}

int
gc_shadowmount_write_pfsc_hints(const char *outer_path,
                                const char *nested_name,
                                int nested_type,
                                char *err,
                                size_t err_size) {
  if(err && err_size) err[0] = 0;
  if(upsert_sector_hint(outer_path, SHADOWMOUNT_PFSC_SECTOR,
                        err, err_size) != 0) {
    return -1;
  }
  if(nested_type == PFS_NESTED_EXFAT && nested_name && nested_name[0]) {
    if(upsert_image_mode_hint(nested_name, 1, err, err_size) != 0) {
      return -1;
    }
    if(remove_sector_hint(nested_name, err, err_size) != 0) {
      return -1;
    }
  }
  return 0;
}

int
gc_shadowmount_request_scan(char *err, size_t err_size) {
  int fd;
  struct timeval tv[2];
  if(err && err_size) err[0] = 0;
  if(mkdir_if_needed(SHADOWMOUNT_DIR) != 0) {
    set_errno_err(err, err_size, "create /data/shadowmount");
    return -1;
  }
  fd = open(SHADOWMOUNT_MANUAL_LIST, O_WRONLY | O_CREAT | O_APPEND, 0666);
  if(fd < 0) {
    set_errno_err(err, err_size, "open ShadowMount manual.lst");
    return -1;
  }
  close(fd);
  if(gettimeofday(&tv[0], NULL) != 0) {
    set_errno_err(err, err_size, "time ShadowMount scan request");
    return -1;
  }
  tv[1] = tv[0];
  if(utimes(SHADOWMOUNT_MANUAL_LIST, tv) != 0) {
    set_errno_err(err, err_size, "touch ShadowMount manual.lst");
    return -1;
  }
  chmod(SHADOWMOUNT_MANUAL_LIST, 0666);
  return 0;
}
