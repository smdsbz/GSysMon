#include <stdio.h>
#include <string.h>
#if SYSMON_SYSTEM_TEST
#include <stdlib.h>
#endif

#include "../include/utils.h"
#include "../include/cpu.h"


struct key_value {
    char   *key;
    char   *value;
};

static struct key_value *parse_line(const char *in) {
    static char key[256];
    static char val[1024];
    static struct key_value kv;
    // "%nc" in scanf-family is not null-terminated, thus cleaning is needed!
    memset(val, 0, 1024);
    int match_count = sscanf(in, "%255[^\t] : %1023c", key, val);
    if (match_count == 0) {
        return NULL;
    }
    kv.key = key;
    if (match_count == 1) {
        kv.value = NULL;
    } else {
        kv.value = val;
    }
    return &kv;
}

struct cpuinfo *sysmon_get_cpuinfo(void) {
    static struct cpuinfo cpuinfo;
    FILE *fp;
    char buf[1024];
    struct key_value *kv;
    // basic initializations, in case fopen() fails, at least we have a fallback
    // state
    memset(&cpuinfo, 0, sizeof(struct cpuinfo));
    if ((fp = fopen("/proc/cpuinfo", "r")) == NULL) {
        perror("fopen");
        return NULL;
    }
    while (freadline(fp, buf, 1024) > 0) {
        kv = parse_line(buf);
        if (strequ(kv->key, "model name")) {
            if (kv->value != NULL) {
                strcpy(cpuinfo.model_name, kv->value);
            } else {
                cpuinfo.model_name[0] = '\0';
            }
        } else if (strequ(kv->key, "cpu MHz")) {
            if (kv->value != NULL) {
                sscanf(kv->value, "%lf", &cpuinfo.cpu_mhz);
            } else {
                cpuinfo.cpu_mhz = 0.0;
            }
        }
    }
    fclose(fp);
    return &cpuinfo;
}


/******* Test *******/

#if SYSMON_CPU_TEST
int main(const int argc, const char **argv) {

    printf("Testing %s():\n", "sysmon_get_cpuinfo");
    struct cpuinfo *cpuinfo = sysmon_get_cpuinfo();
    if (cpuinfo) {
        if (strlen(cpuinfo->model_name)) {
            printf(SYSMON_TEST_SUCCESS ": model name: %s\n", cpuinfo->model_name);
        } else {
            puts(SYSMON_TEST_FAIL ": model name");
        }
        if (cpuinfo->cpu_mhz != 0.0) {
            printf(SYSMON_TEST_SUCCESS ": cpu MHz: %.2lf\n", cpuinfo->cpu_mhz);
        } else {
            puts(SYSMON_TEST_FAIL ": cpu MHz");
        }
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", cpuinfo);
    }

}
#endif

