#ifndef GC_SHADOWMOUNT_H
#define GC_SHADOWMOUNT_H

#include <stddef.h>

#include "pfs_compress.h"

int gc_shadowmount_write_pfsc_hints(const char *outer_path,
                                    const char *nested_name,
                                    int nested_type,
                                    char *err,
                                    size_t err_size);

int gc_shadowmount_request_scan(char *err, size_t err_size);

#endif
