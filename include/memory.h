#ifndef _SYSMON_MEMORY_H
#define _SYSMON_MEMORY_H

#include <string.h>

struct meminfo {
    size_t      mem_total;
    size_t      mem_free;
    size_t      buffers;
    size_t      cached;
    size_t      swap_total;
    size_t      swap_free;
};
static inline void meminfocpy(struct meminfo *dst, const struct meminfo *src) {
    memcpy(dst, src, sizeof(struct meminfo));
    return;
}

/**
 * sysmon_get_meminfo() - get memory infos from ``/proc/meminfo``
 *
 * sysmon_get_meminfo() returns a statically allocated meminfo struct on success,
 * or NULL if failed.
 */
struct meminfo *sysmon_get_meminfo(void);

#endif
