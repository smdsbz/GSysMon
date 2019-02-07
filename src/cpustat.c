#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../include/utils.h"
#include "../include/cpustat.h"


/******* Interface Implementation *******/

struct single_cpustat *sysmon_get_cpustat(int id) {     // {{{
    static const char statfmt[] = (     // {{{
        "%*s "      // cpu name (cpu | cpuN)
        "%lu "      // user
        "%lu "      // nice
        "%lu "      // system
        "%lu "      // idle
        "%lu "      // iowait
        "%lu "      // irq
        "%lu "      // softirq
        "%lu "      // steal
        /* "%lu "      // guest */
        /* "%lu "      // guest_nice */
    );      // }}}
    static struct single_cpustat stat;
    FILE *fp;
    char buf[256];
    char cpu_pref[16] = "cpu ";
    int retval;
    stat.id = id;
    // get line leading identifier
    if (id >= 0) {
        sprintf(cpu_pref, "cpu%d ", id);
    }
    if ((fp = fopen("/proc/stat", "r")) == NULL) {
        perror("fopen");
        return NULL;
    }
    // find and read
    int found = 0;
    while (freadline(fp, buf, 256) == 1) {
        if (!str_starts_with(buf, cpu_pref)) {
            continue;
        }
        retval = sscanf(buf, statfmt, &stat.user, &stat.nice, &stat.system,
                        &stat.idle, &stat.iowait, &stat.irq, &stat.softirq,
                        &stat.steal);
        if (retval == 8) {
            cpustat_set_total(&stat);
            found = 1;
        }
        break;
    }
    fclose(fp);
    if (!found) {
        return NULL;
    }
    return &stat;
}   // }}}

/**
 * get_ncpus() - get cpu count from ``/proc/stat``
 *
 * get_ncpus() returns the number of cpus, or 0 on error.
 *
 * get_ncpus() REQUIRES the ``/proc/stat`` to have the following structure when
 * the system has N + 1 cpus:
 *
 *      cpu xxxx xxxx xxxx ...
 *      cpu0 xxxx xxxx xxxx ...
 *      cpu1 xxxx xxxx xxxx ...
 *      ...
 *      cpuN xxxx xxxx xxxx ...
 *      (some other line that doesn't start with "cpu")
 *      ...
 *
 * otherwise the function fails and return value may be undefined.
 */
static size_t get_ncpus(void) {
    char buf[256];
    FILE *fp;
    size_t ncpus = 0;
    if ((fp = fopen("/proc/stat", "r")) == NULL) {
        return 0;
    }
    for (; (freadline(fp, buf, 256) == 1) && str_starts_with(buf, "cpu"); ++ncpus) ;
    fclose(fp);
    return ((ncpus == 0) ? 0 : ncpus - 1);      // discard the first total "cpu" line
}

// __cpustat_old: bakup of __cpustat_vol, used to record previous call results
// __cpustat_vol: the volatile one
// __cpustat_diff: diff result
static struct cpustat __cpustat_old = { .ncpus = 0, .each = NULL },
                      __cpustat_vol = { .ncpus = 0, .each = NULL },
                      __cpustat_diff = { .ncpus = 0, .each = NULL };

/**
 * sysmon_get_cpustat_all() gets latest cpu stats and stores it into __cpustat_vol.
 */
struct cpustat *sysmon_get_cpustat_all(void) {
    // prepare `ncpus` and `each`
    size_t ncpus_new = get_ncpus();
    if (__cpustat_vol.each != NULL) {
        if (__cpustat_vol.ncpus != ncpus_new) {
            free(__cpustat_vol.each);
            __cpustat_vol.each = NULL;
        }
    }
    __cpustat_vol.ncpus = ncpus_new;
    if (__cpustat_vol.each == NULL) {
        if ((__cpustat_vol.each = malloc(ncpus_new * sizeof(struct single_cpustat))) == NULL) {
            return NULL;
        }
    }
    // read and assign
    struct single_cpustat *stat = sysmon_get_cpustat(-1);
    if (stat == NULL) {
        return NULL;
    }
    single_cpustat_cpy(&__cpustat_vol.all, stat);
    for (int id = 0; id != __cpustat_vol.ncpus; ++id) {
        if ((stat = sysmon_get_cpustat(id)) == NULL) {
            return NULL;
        }
        single_cpustat_cpy(&__cpustat_vol.each[id], stat);
    }
    return &__cpustat_vol;
}

int cpustat_init(void) {
    memset(&__cpustat_old, 0, sizeof(struct cpustat));
    memset(&__cpustat_vol, 0, sizeof(struct cpustat));
    memset(&__cpustat_diff, 0, sizeof(struct cpustat));
    return 0;
}

void cpustat_cleanup(void) {
    if (__cpustat_old.each != NULL) {
        free(__cpustat_old.each);
    }
    memset(&__cpustat_old, 0, sizeof(struct cpustat));
    if (__cpustat_vol.each != NULL) {
        free(__cpustat_vol.each);
    }
    memset(&__cpustat_vol, 0, sizeof(struct cpustat));
    if (__cpustat_diff.each != NULL) {
        free(__cpustat_diff.each);
    }
    memset(&__cpustat_diff, 0, sizeof(struct cpustat));
    return;
}

struct cpustat *cpustatcpy(struct cpustat *dst, const struct cpustat *src) {
    // sync `ncpus` and make sure `each` field has the same size
    if (dst->ncpus != src->ncpus) {
        if (dst->each != NULL) {
            free(dst->each);
            dst->each = NULL;
        }
        dst->ncpus = src->ncpus;
    }
    if (dst->each == NULL) {
        if ((dst->each = malloc(src->ncpus * sizeof(struct single_cpustat))) == NULL) {
            return NULL;
        }
    }
    memcpy(&dst->all, &src->all, sizeof(struct single_cpustat));
    memcpy(dst->each, src->each, src->ncpus * sizeof(struct single_cpustat));
    return dst;
}

void cpustat_del(struct cpustat *cpustat) {
    if (cpustat->each != NULL) {
        free(cpustat->each);
    }
    free(cpustat);
    return;
}

/**
 * single_cpustat/cpustat_sub_() - subtract @old from @new inplace
 * @new
 * @old
 *
 * @new and @old must be valid variables, i.e. assigned at least once.
 *
 * @new must be later stats gathered after @old, otherwise values are undefined!
 *
 * cpustat_sub_() returns 0 on success. If failed, e.g. `ncpus` don't match,
 * returns -1, but diffs of `all` is still calculated, although I don't think
 * it is anything useful unless you have hot-swappable cpus.
 */
static void single_cpustat_sub_(struct single_cpustat *new,
                                const struct single_cpustat *old) {
    new->total -= old->total;
    new->user -= old->user;
    new->nice -= old->nice;
    new->system -= old->system;
    new->idle -= old->idle;
    new->iowait -= old->iowait;
    new->irq -= old->irq;
    new->softirq -= old->softirq;
    new->steal -= old->steal;
    return;
}

static int cpustat_sub_(struct cpustat *new, const struct cpustat *old) {
    single_cpustat_sub_(&new->all, &old->all);
    if (new->ncpus != old->ncpus) {
        return -1;
    }
    for (int id = 0; id != new->ncpus; ++id) {
        single_cpustat_sub_(&new->each[id], &old->each[id]);
    }
    return 0;
}

struct cpustat *sysmon_get_cpustat_diff(void) {
    // check if previous stat exists, if not, it's not diff-able
    if (__cpustat_old.each == NULL) {
        // fill __cpustat_old with data
        if (sysmon_get_cpustat_all() == NULL) {
            return NULL;
        }
        cpustatcpy(&__cpustat_old, &__cpustat_vol);
        return NULL;
    }
    // get new stats to __cpustat_vol
    if (sysmon_get_cpustat_all() == NULL) {
        return NULL;
    }
    // calculate diff to __cpustat_diff
    cpustatcpy(&__cpustat_diff, &__cpustat_vol);
    cpustat_sub_(&__cpustat_diff, &__cpustat_old);
    // overwrite old stats with new
    cpustatcpy(&__cpustat_old, &__cpustat_vol);
    return &__cpustat_diff;
}


/******* Calculation Helpers *******/

static inline const struct single_cpustat *get_single_from_cpustat(
        const struct cpustat *in, int cpuid) {
    return (
        (cpuid < 0) ?
            &in->all
        : (cpuid >= in->ncpus) ?
            NULL
        : &in->each[cpuid]
    );
}

double cpustat_get_user_percentage(const struct cpustat *diff, int cpuid) {
    const struct single_cpustat *single = get_single_from_cpustat(diff, cpuid);
    if (single == NULL) {
        return -1.0;
    }
    return (double)100.0 * (double)(single->user + single->nice)
            / (double)single->total;
}

double cpustat_get_kernel_percentage(const struct cpustat *diff, int cpuid) {
    const struct single_cpustat *single = get_single_from_cpustat(diff, cpuid);
    if (single == NULL) {
        return -1.0;
    }
    return (double)100.0 * (double)(single->system + single->irq + single->softirq)
            / (double)single->total;
}

double cpustat_get_idle_percentage(const struct cpustat *diff, int cpuid) {
    const struct single_cpustat *single = get_single_from_cpustat(diff, cpuid);
    if (single == NULL) {
        return -1.0;
    }
    return (double)100.0 * (double)(single->idle + single->iowait)
            / (double)single->total;
}

double cpustat_get_usage_percentage(const struct cpustat *diff, int cpuid) {
    const struct single_cpustat *single = get_single_from_cpustat(diff, cpuid);
    if (single == NULL) {
        return -1.0;
    }
    return (double)100.0 * (double)(single->total - single->idle - single->iowait)
            / (double)single->total;
}


/******* Test *******/

#if SYSMON_CPUSTAT_TEST
#include <time.h>

static inline void print_single_cpustat(const struct single_cpustat *const stat) {
    if (stat != NULL) {
        printf(SYSMON_TEST_SUCCESS ": id: %d\n", stat->id);
        printf(SYSMON_TEST_ESCAPE "total: %lu\n", stat->total);
        printf(SYSMON_TEST_ESCAPE "user: %lu\n", stat->user);
        printf(SYSMON_TEST_ESCAPE "nice: %lu\n", stat->nice);
        printf(SYSMON_TEST_ESCAPE "system: %lu\n", stat->system);
        printf(SYSMON_TEST_ESCAPE "idle: %lu\n", stat->idle);
        printf(SYSMON_TEST_ESCAPE "iowait: %lu\n", stat->iowait);
        printf(SYSMON_TEST_ESCAPE "irq: %lu\n", stat->irq);
        printf(SYSMON_TEST_ESCAPE "softirq: %lu\n", stat->softirq);
        printf(SYSMON_TEST_ESCAPE "steal: %lu\n", stat->steal);
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", stat);
    }
    return;
}

int main(const int argc, const char **argv) {
    sysmon_cpustat_load();

    printf("Testing %s():\n", "get_ncpus");
    size_t ncpus = get_ncpus();
    if (ncpus != 0) {
        printf(SYSMON_TEST_SUCCESS ": ncpus: %lu\n", ncpus);
    } else {
        printf(SYSMON_TEST_FAIL ": got %lu\n", ncpus);
    }

    /* printf("Testing %s():\n", "sysmon_get_cpustat"); */
    /* struct single_cpustat *stat = sysmon_get_cpustat(2); */
    /* print_single_cpustat(stat); */

    printf("Testing %s():\n", "sysmon_get_cpustat_all");
    struct cpustat *stat_all = sysmon_get_cpustat_all();
    if (stat_all != NULL) {
        struct single_cpustat *stat = &stat_all->all;
        print_single_cpustat(stat);
        for (int id = 0; id != stat_all->ncpus; ++id) {
            stat = &stat_all->each[id];
            print_single_cpustat(stat);
        }
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", stat_all);
    }

    printf("Testing %s():\n", "sysmon_get_cpustat_diff");
    sysmon_get_cpustat_diff();
    printf(SYSMON_TEST_ESCAPE "sleeping for %.2lf seconds ...\n", 0.5);
    usleep(500000);
    struct cpustat *diff = sysmon_get_cpustat_diff();
    if (diff != NULL) {
        struct single_cpustat *stat = &diff->all;
        print_single_cpustat(stat);
        printf(SYSMON_TEST_ESCAPE "usage: %.2lf\n", cpustat_get_usage_percentage(diff, -1));
        for (int id = 0; id != diff->ncpus; ++id) {
            stat = &diff->each[id];
            print_single_cpustat(stat);
            printf(SYSMON_TEST_ESCAPE "usage: %.2lf\n", cpustat_get_usage_percentage(diff, id));
        }
    } else {
        printf(SYSMON_TEST_FAIL ": got %p\n", diff);
    }

    sysmon_cpustat_unload();
    return 0;
}
#endif
