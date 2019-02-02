#include <stdio.h>

#include "../include/utils.h"
#include "../include/memory.h"


struct meminfo *sysmon_get_meminfo(void) {
    static struct meminfo meminfo;
    FILE *fp;
    char buf[256];
    if ((fp = fopen("/proc/meminfo", "r")) == NULL) {
        perror("fopen");
        return NULL;
    }
    //   5          4         3        2       1           0
    // { mem_total, mem_free, buffers, cached, swap_total, swap_free }
    unsigned found = 0;
    while (freadline(fp, buf, 256) == 1) {
        if (sscanf(buf, "MemTotal : %lu kB", &meminfo.mem_total) == 1) {
            found |= 32;
        } else if (sscanf(buf, "MemFree : %lu kB", &meminfo.mem_free) == 1) {
            found |= 16;
        } else if (sscanf(buf, "Buffers : %lu kB", &meminfo.buffers) == 1) {
            found |= 8;
        } else if (sscanf(buf, "Cached : %lu kB", &meminfo.cached) == 1) {
            found |= 4;
        } else if (sscanf(buf, "SwapTotal : %lu kB", &meminfo.swap_total) == 1) {
            found |= 2;
        } else if (sscanf(buf, "SwapFree : %lu kB", &meminfo.swap_free) == 1) {
            found |= 1;
        }
        if (found == 63) {
            break;
        }
    }
    fclose(fp);
    if (found != 63) {
        return NULL;
    }
    return &meminfo;
}


/******* Test *******/

#if SYSMON_MEMORY_TEST
int main(const int argc, const char **argv) {

    printf("Testing %s():\n", "sysmon_get_meminfo");
    struct meminfo *meminfo = sysmon_get_meminfo();
    if (meminfo != NULL) {
        printf(SYSMON_TEST_SUCCESS ": MemTotal: %lu\n", meminfo->mem_total);
        printf(SYSMON_TEST_ESCAPE "MemFree: %lu\n", meminfo->mem_free);
        printf(SYSMON_TEST_ESCAPE "Buffers: %lu\n", meminfo->buffers);
        printf(SYSMON_TEST_ESCAPE "Cached: %lu\n", meminfo->cached);
        printf(SYSMON_TEST_ESCAPE "SwapTotal: %lu\n", meminfo->swap_total);
        printf(SYSMON_TEST_ESCAPE "SwaptFree: %lu\n", meminfo->swap_free);
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", meminfo);
    }

    return 0;
}
#endif

