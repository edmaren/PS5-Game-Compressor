#ifndef GC_APP_INSTALLER_H
#define GC_APP_INSTALLER_H

#define GAME_COMPRESSOR_LAUNCHER_TITLE_ID "PSGC50001"

int gc_launcher_start(const char *ip);
int gc_install_app_if_needed(void);

#endif
