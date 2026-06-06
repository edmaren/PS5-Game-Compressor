#ifndef GC_DIAG_H
#define GC_DIAG_H

#define GC_DIAG_SKIPPED (-2147483000)

void gc_diag_init(void);
void gc_diag_install_signal_handlers(void);
void gc_checkpoint(const char *checkpoint);
void gc_log(const char *fmt, ...);
const char *gc_diag_checkpoint(void);

#endif
