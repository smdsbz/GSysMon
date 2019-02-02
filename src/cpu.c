#include <stdio.h>
#include <string.h>

#include "../include/utils.h"
#include "../include/cpu.h"


/******* Single Processor *******/

struct cpuinfo *sysmon_get_cpuinfo(int processor) {     // {{{
    static struct cpuinfo cpuinfo;
    FILE *fp;
    char buf[1024];
    memset(&cpuinfo, 0, sizeof(struct cpuinfo));
    if ((fp = fopen("/proc/cpuinfo", "r")) == NULL) {
        perror("fopen");
        return NULL;
    }
    int curr_proc = -1;
    // indicates whether corresponding field has been filled:
    // { proc, model, mhz, physid }
    unsigned found = 0;
    while (freadline(fp, buf, 1024) >= 0) {
        if (sscanf(buf, "processor : %d", &curr_proc) == 1) {
            if (curr_proc == processor) {
                cpuinfo.processor = curr_proc;
                found |= 8;
            }
            continue;
        }
        if (curr_proc < processor) {
            continue;
        } else if (curr_proc > processor) {
            break;
        }
        if (sscanf(buf, "model name : %255c", cpuinfo.model_name) == 1) {
            found |= 4;
        } else if (sscanf(buf, "cpu MHz : %lf", &cpuinfo.cpu_mhz) == 1) {
            found |= 2;
        } else if (sscanf(buf, "core id : %u", &cpuinfo.core_id) == 1) {
            found |= 1;
        }
        if (found == 15) {
            break;
        }
    }
    fclose(fp);
    // checks if every field has been filled
    if (found != 15) {
        return NULL;
    }
    return &cpuinfo;
}   // }}}

/******* Multiple Processors *******/

static int _get_processor_count(void) {
    int proc_cnt = 0;
    FILE *fp;
    char buf[1024];
    if ((fp = fopen("/proc/cpuinfo", "r")) == NULL) {
        perror("fopen");
        return -1;
    }
    while (freadline(fp, buf, 1024) >= 0) {
        if (str_starts_with(buf, "processor")) {
            ++proc_cnt;
        }
    }
    fclose(fp);
    return proc_cnt;
}

/******* struct cpusinfo Helpers *******/

static struct cpusinfo __cpusinfo;

int cpusinfo_init(void) {
    int retval;
    memset(&__cpusinfo, 0, sizeof(struct cpusinfo));
    retval = _get_processor_count();
    if (retval < 0) {
        return retval;
    }
    __cpusinfo.processor_count = retval;
    __cpusinfo.cpuinfos =
        malloc(__cpusinfo.processor_count * sizeof(struct cpuinfo));
    if (__cpusinfo.cpuinfos == NULL) {
        return -1;
    }
    memset(__cpusinfo.cpuinfos, 0,
            __cpusinfo.processor_count * sizeof(struct cpuinfo));
    return 0;
}

void cpusinfo_del(void) {
    if (__cpusinfo.cpuinfos != NULL) {
        free(__cpusinfo.cpuinfos);
    }
    memset(&__cpusinfo, 0, sizeof(struct cpusinfo));
    return;
}

/******* Module Main Interfaces *******/

struct cpusinfo *sysmon_get_cpusinfo(void) {
    if (__cpusinfo.cpuinfos == NULL) {
        return NULL;
    }
    for (int idx = 0; idx != __cpusinfo.processor_count; ++idx) {
        struct cpuinfo *cpuinfo = sysmon_get_cpuinfo(idx);
        if (cpuinfo != NULL) {
            cpuinfocpy(&__cpusinfo.cpuinfos[idx], cpuinfo);
        } else {
            memset(&__cpusinfo.cpuinfos[idx], 0, sizeof(struct cpuinfo));
            return NULL;
        }
    }
    return &__cpusinfo;
}


/******* Test *******/

#if SYSMON_CPU_TEST
int main(const int argc, const char **argv) {

    /* printf("Testing %s():\n", "sysmon_get_cpuinfo"); */
    /* struct cpuinfo *cpuinfo = sysmon_get_cpuinfo(3); */
    /* if (cpuinfo) { */
    /*     printf(SYSMON_TEST_SUCCESS ": processor: %u\n", cpuinfo->processor); */
    /*     if (strlen(cpuinfo->model_name)) { */
    /*         printf(SYSMON_TEST_SUCCESS ": model name: %s\n", cpuinfo->model_name); */
    /*     } else { */
    /*         puts(SYSMON_TEST_FAIL ": model name"); */
    /*     } */
    /*     if (cpuinfo->cpu_mhz != 0.0) { */
    /*         printf(SYSMON_TEST_SUCCESS ": cpu MHz: %.2lf\n", cpuinfo->cpu_mhz); */
    /*     } else { */
    /*         puts(SYSMON_TEST_FAIL ": cpu MHz"); */
    /*     } */
    /*     printf(SYSMON_TEST_SUCCESS ": core id: %u\n", cpuinfo->core_id); */
    /* } else { */
    /*     printf(SYSMON_TEST_FAIL ": got %p\n", cpuinfo); */
    /* } */

    /* printf("Testing %s():\n", "_get_processor_count"); */
    /* int proc_cnt = _get_processor_count(); */
    /* if (proc_cnt != -1) { */
    /*     printf(SYSMON_TEST_SUCCESS ": processor count (logical): %d\n", proc_cnt); */
    /* } else { */
    /*     printf(SYSMON_TEST_FAIL ": got %d\n", proc_cnt); */
    /* } */

    printf("Testing the %s module:\n", "cpu");
    int retinit = sysmon_cpu_load();
    if (retinit == 0) {
        printf(SYSMON_TEST_SUCCESS ": %s\n", "initialization");
    } else {
        printf(SYSMON_TEST_FAIL ": initialization got %d\n", retinit);
        return 0;
    }
    struct cpusinfo *info = sysmon_get_cpusinfo();
    if (info != NULL) {
        for (unsigned idx = 0, range = info->processor_count;
                idx != range; ++idx) {
            struct cpuinfo *cpuinfo = &info->cpuinfos[idx];
            printf(SYSMON_TEST_SUCCESS ": processor: %u\n", cpuinfo->processor);
            if (strlen(cpuinfo->model_name)) {
                printf(SYSMON_TEST_ESCAPE "model name: %s\n",
                        cpuinfo->model_name);
            } else {
                puts(SYSMON_TEST_FAIL ": model name");
            }
            if (cpuinfo->cpu_mhz != 0.0) {
                printf(SYSMON_TEST_ESCAPE "cpu MHz: %.2lf\n",
                        cpuinfo->cpu_mhz);
            } else {
                puts(SYSMON_TEST_FAIL ": cpu MHz");
            }
            printf(SYSMON_TEST_ESCAPE "core id: %u\n", cpuinfo->core_id);
        }
    } else {
        printf(SYSMON_TEST_FAIL ": %s\n", "sysmon_get_cpusinfo()");
    }
    sysmon_cpu_unload();

    return 0;
}
#endif

