#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define IOWAIT_CHECK_INTVL 5  /* Seconds */
#define IOWAIT_THRESH 30      /* Percent */

static int iowait_check_intvl = IOWAIT_CHECK_INTVL;
static int iowait_thresh = IOWAIT_THRESH;

#define IOTOP_OUTPUT_FILE     "/tmp/iowait_checkd_iotop.log"
#define IOTOP_OUTPUT_FILE_BKP "/tmp/iowait_checkd_iotop.log.1"
#define IOTOP_OUTPUT_FILE_SIZE_MAX (100 * 1000 * 1000)  /* Bytes */

static char *iotop_output_file = NULL;
static char *iotop_output_file_bkp = NULL;
static int iotop_output_file_size_max = IOTOP_OUTPUT_FILE_SIZE_MAX;

#define STAT_FILE "/proc/stat"
#define IOTOP_COMMAND "iotop -b -n 2 -t -o -qqq"
#define CPU_ARY_SIZE 8
#define BUF_SIZE 1024

static char *param_file = NULL;

void print_cpu_info(unsigned long *cpu_info, int size)
{
    const char *entry[] = {
        "user", "nice", "system", "idle", "iowait",
        "irq", "softirq", "steal", "guest", "guest nice"
    };
    int i;

    // cpu[0]: user
    // cpu[1]: nice
    // cpu[2]: system
    // cpu[3]: idle
    // cpu[4]: iowait
    // cpu[5]: irq
    // cpu[6]: softirq
    // cpu[7]: steal
    // cpu[8]: guest
    // cpu[9]: guest nice
    for (i = 0; i < size; i++) {
        printf("%12s: %lu\n", entry[i], cpu_info[i]);
    }
    printf("\n");
}

static void ioc_info(const char *message, ...)
{
    va_list ap;

    va_start(ap, message);
    vsyslog(LOG_INFO, message, ap);
    va_end(ap);
}

static void ioc_abort(const char *message, ...)
{
    va_list ap;

    va_start(ap, message);
    vsyslog(LOG_ERR, message, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

static void copy_file(const char *src_file, const char *dst_file)
{
    FILE *fp_src, *fp_dst;
    char buf[BUF_SIZE + 1];
    int size;

    if ((fp_src = fopen(src_file, "r")) == NULL) {
        ioc_abort("fopen error for %s", src_file);
    }
    if ((fp_dst = fopen(dst_file, "w")) == NULL) {
        ioc_abort("fopen error for %s", dst_file);
    }
    while ((size = fread(buf, sizeof(char), sizeof(buf), fp_src)) > 0) {
        if (fwrite(buf, sizeof(char), size, fp_dst) != size) {
            ioc_abort("fwrite error for %s", dst_file);
        }
    }
    fclose(fp_dst);
    fclose(fp_src);
}

static void output_process_info()
{
    FILE *fp, *fp_p;
    char buf[BUF_SIZE + 1];
    int size;

    if ((fp = fopen(iotop_output_file, "a")) == NULL) {
        ioc_abort("fopen error for %s", iotop_output_file);
    }
    if ((fp_p = popen(IOTOP_COMMAND, "r")) == NULL) {
        ioc_abort("popen error for %s", IOTOP_COMMAND);
    }
    if ((size = fread(buf, sizeof(char), sizeof(buf), fp_p)) > 0) {
        time_t t;
        struct tm *tm_now;

        t = time(NULL);
        tm_now = localtime(&t);

        fprintf(fp, "[%4d/%02d/%02d %02d:%02d:%02d]\n",
                tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
                tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
        if (fwrite(buf, sizeof(char), size, fp) != size) {
            ioc_abort("fwrite error for %s", iotop_output_file);
        }
        while ((size = fread(buf, sizeof(char), sizeof(buf), fp_p)) > 0) {
            if (fwrite(buf, sizeof(char), size, fp) != size) {
                ioc_abort("fwrite error for %s", iotop_output_file);
            }
        }
    }
    if (ftell(fp) > iotop_output_file_size_max) {
        copy_file(iotop_output_file, iotop_output_file_bkp);
        if (truncate(iotop_output_file, 0) < 0) {
            ioc_abort("truncate error for %s", iotop_output_file);
        }
    }
    pclose(fp_p);
    fclose(fp);
}

static void check_iowait()
{
    FILE *fp;
    unsigned long cpu_old[CPU_ARY_SIZE];

    if ((fp = fopen(STAT_FILE, "r")) == NULL) {
        ioc_abort("fopen error for %s", STAT_FILE);
    }
    if (fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &cpu_old[0], &cpu_old[1], &cpu_old[2], &cpu_old[3],
               &cpu_old[4], &cpu_old[5], &cpu_old[6], &cpu_old[7]) != 8) {
        ioc_abort("fscanf error for %s", STAT_FILE);
    }

#ifdef IOC_DEBUG
    print_cpu_info(cpu_old, CPU_ARY_SIZE);
#endif

    sleep(iowait_check_intvl);

    while (1) {
        unsigned long cpu_now[CPU_ARY_SIZE], cpu_diff[CPU_ARY_SIZE];
        unsigned long cpu_diff_sum = 0;
        int i;

        if ((fp = freopen(STAT_FILE, "r", fp)) == NULL) {
            ioc_abort("freopen error for %s", STAT_FILE);
        }
        if (fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &cpu_now[0], &cpu_now[1], &cpu_now[2], &cpu_now[3],
                   &cpu_now[4], &cpu_now[5], &cpu_now[6], &cpu_now[7]) != 8) {
            ioc_abort("fscanf error for %s", STAT_FILE);
        }

#ifdef IOC_DEBUG
        print_cpu_info(cpu_now, CPU_ARY_SIZE);
#endif

        for (i = 0; i < CPU_ARY_SIZE; i++) {
            cpu_diff[i] = cpu_now[i] - cpu_old[i];
            cpu_diff_sum += cpu_diff[i];
        }
        if ((cpu_diff[4] * 100 / cpu_diff_sum) > iowait_thresh) {
            output_process_info();
        }

        memcpy(cpu_old, cpu_now, sizeof(long) * CPU_ARY_SIZE);
        sleep(iowait_check_intvl);
    }

    fclose(fp);
}

static void input_param()
{
    FILE *fp;
    char buf[BUF_SIZE + 1];
    char name[BUF_SIZE], dummy[BUF_SIZE], value[BUF_SIZE];

    if ((fp = fopen(param_file, "r")) == NULL) {
        ioc_abort("fopen error for %s", param_file);
    }
    while (fgets(buf, BUF_SIZE, fp) != NULL) {
        if ((buf[0] == '#') || (strcmp(buf, "\n") == 0)) {
            continue;
        } else {
            if (sscanf(buf, "%s %s %s\n", name, dummy, value) != 3) {
                ioc_abort("fscanf error for %s", param_file);
            }
            if (strcmp(name, "iowait_check_intvl") == 0) {
                iowait_check_intvl = atoi(value);
            } else if (strcmp(name, "iowait_thresh") == 0) {
                iowait_thresh = atoi(value);
            } else if (strcmp(name, "iotop_output_file") == 0) {
                free(iotop_output_file);
                if ((iotop_output_file = strdup(value)) == NULL) {
                    ioc_abort("strdup error for %s", value);
                }
            } else if (strcmp(name, "iotop_output_file_bkp") == 0) {
                free(iotop_output_file_bkp);
                if ((iotop_output_file_bkp = strdup(value)) == NULL) {
                    ioc_abort("strdup error for %s", value);
                }
            } else if (strcmp(name, "iotop_output_file_size_max") == 0) {
                iotop_output_file_size_max = atoi(value);
            }
        }
    }
    fclose(fp);
}

static void sighup_handler(int sig)
{
    int iowait_check_intvl_old = iowait_check_intvl;
    int iowait_thresh_old = iowait_thresh;
    int iotop_output_file_size_max_old = iotop_output_file_size_max;

    input_param();

    ioc_info("iowait_check_intvl: %d -> %d", iowait_check_intvl_old, iowait_check_intvl);
    ioc_info("iowait_thresh: %d -> %d", iowait_thresh_old, iowait_thresh);
    ioc_info("iotop_output_file: %s", iotop_output_file);
    ioc_info("iotop_output_file_bkp: %s", iotop_output_file_bkp);
    ioc_info("iotop_output_file_size_max: %d -> %d", iotop_output_file_size_max_old, iotop_output_file_size_max);
}

static void init(int argc, char *argv[])
{
#ifdef IOC_DEBUG
    printf("********** debug mode **********\n");
#else
    if (daemon(0, 0) != 0) {
        ioc_abort("daemon error");
    }
#endif

    if ((iotop_output_file = strdup(IOTOP_OUTPUT_FILE)) == NULL) {
        ioc_abort("strdup error for %s", IOTOP_OUTPUT_FILE);
    }
    if ((iotop_output_file_bkp = strdup(IOTOP_OUTPUT_FILE_BKP)) == NULL) {
        ioc_abort("strdup error for %s", IOTOP_OUTPUT_FILE_BKP);
    }

    if (argc == 2) {
        if ((param_file = strdup(argv[1])) == NULL) {
            ioc_abort("strdup error for %s", argv[1]);
        }
        if (signal(SIGHUP, sighup_handler) == SIG_ERR) {
            ioc_abort("signal error for SIGHUP");
        }
        input_param();
    }
}

int main(int argc, char *argv[])
{
    init(argc, argv);
    check_iowait();
    return (0);
}
