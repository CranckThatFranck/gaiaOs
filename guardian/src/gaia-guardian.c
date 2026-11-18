/*
 * ==============================================================================
 * Gaia OS 1.0 (Westmere Edition) - gaia-guardian.c
 * Daemon Nativo Ultraleve em C (~1 MB RSS RAM) para Telemetria, Prevenção
 * Anti-Congelamento (PSI / zRAM / Thermal) e Orquestração Efêmera do Gemini 3.8 Flash.
 * ==============================================================================
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>

#define LOG_DIR "/var/log/gaia-guardian"
#define ACTIONS_LOG "/var/log/gaia-guardian/actions.log"
#define STATUS_JSON "/var/log/gaia-guardian/status.json"
#define STATUS_TMP "/var/log/gaia-guardian/status.json.tmp"
#define PID_FILE "/run/gaia-guardian.pid"
#define ADVISOR_BIN "/usr/local/bin/gaia-gemini-advisor"

static volatile sig_atomic_t keep_running = 1;
static volatile sig_atomic_t trigger_gemini_now = 0;
static volatile sig_atomic_t trigger_reclaim_now = 0;

typedef struct {
    long mem_total_kb;
    long mem_avail_kb;
    long mem_free_kb;
    long buffers_kb;
    long cached_kb;
    long swap_total_kb;
    long swap_free_kb;
    double psi_some_avg10;
    double psi_full_avg10;
    double cpu_temp_c;
    char mglru_state[32];
    long daemon_rss_kb;
} sys_metrics_t;

static void handle_sigterm(int sig) {
    (void)sig;
    keep_running = 0;
}

static void handle_sigusr1(int sig) {
    (void)sig;
    trigger_gemini_now = 1;
}

static void handle_sigusr2(int sig) {
    (void)sig;
    trigger_reclaim_now = 1;
}

static void log_action(const char *level, const char *msg) {
    FILE *fp = fopen(ACTIONS_LOG, "a");
    if (!fp) return;
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &tm_info);
    fprintf(fp, "[%s] [%s] %s\n", tbuf, level, msg);
    fclose(fp);
}

static void read_metrics(sys_metrics_t *m) {
    memset(m, 0, sizeof(*m));
    strcpy(m->mglru_state, "unknown");

    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            long val = 0;
            if (sscanf(line, "MemTotal: %ld kB", &val) == 1) m->mem_total_kb = val;
            else if (sscanf(line, "MemAvailable: %ld kB", &val) == 1) m->mem_avail_kb = val;
            else if (sscanf(line, "MemFree: %ld kB", &val) == 1) m->mem_free_kb = val;
            else if (sscanf(line, "Buffers: %ld kB", &val) == 1) m->buffers_kb = val;
            else if (sscanf(line, "Cached: %ld kB", &val) == 1) m->cached_kb = val;
            else if (sscanf(line, "SwapTotal: %ld kB", &val) == 1) m->swap_total_kb = val;
            else if (sscanf(line, "SwapFree: %ld kB", &val) == 1) m->swap_free_kb = val;
        }
        fclose(fp);
    }

    fp = fopen("/proc/pressure/memory", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            double avg10 = 0.0;
            if (strncmp(line, "some ", 5) == 0) {
                if (sscanf(line, "some avg10=%lf", &avg10) == 1) m->psi_some_avg10 = avg10;
            } else if (strncmp(line, "full ", 5) == 0) {
                if (sscanf(line, "full avg10=%lf", &avg10) == 1) m->psi_full_avg10 = avg10;
            }
        }
        fclose(fp);
    }

    /* Read max thermal zone & hwmon (coretemp) temperature */
    double max_temp = 0.0;
    for (int i = 0; i < 6; i++) {
        char path[128];
        snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", i);
        fp = fopen(path, "r");
        if (fp) {
            long millic = 0;
            if (fscanf(fp, "%ld", &millic) == 1) {
                double c = millic / 1000.0;
                if (c > max_temp) max_temp = c;
            }
            fclose(fp);
        }
        for (int j = 1; j <= 8; j++) {
            snprintf(path, sizeof(path), "/sys/class/hwmon/hwmon%d/temp%d_input", i, j);
            fp = fopen(path, "r");
            if (fp) {
                long millic = 0;
                if (fscanf(fp, "%ld", &millic) == 1) {
                    double c = millic / 1000.0;
                    if (c > max_temp) max_temp = c;
                }
                fclose(fp);
            }
        }
    }
    m->cpu_temp_c = max_temp;

    fp = fopen("/sys/kernel/mm/lru_gen/enabled", "r");
    if (fp) {
        if (fgets(m->mglru_state, sizeof(m->mglru_state), fp)) {
            m->mglru_state[strcspn(m->mglru_state, "\r\n")] = 0;
        }
        fclose(fp);
    }

    fp = fopen("/proc/self/status", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            long rss = 0;
            if (sscanf(line, "VmRSS: %ld kB", &rss) == 1) {
                m->daemon_rss_kb = rss;
                break;
            }
        }
        fclose(fp);
    }
}

static void write_status_json(const sys_metrics_t *m) {
    FILE *fp = fopen(STATUS_TMP, "w");
    if (!fp) return;
    long used_kb = m->mem_total_kb - m->mem_avail_kb;
    double used_pct = m->mem_total_kb > 0 ? (used_kb * 100.0) / m->mem_total_kb : 0.0;
    long swap_used_kb = m->swap_total_kb - m->swap_free_kb;
    time_t now = time(NULL);

    fprintf(fp,
        "{\n"
        "  \"timestamp\": %ld,\n"
        "  \"daemon_pid\": %d,\n"
        "  \"daemon_rss_kb\": %ld,\n"
        "  \"mem_total_mb\": %ld,\n"
        "  \"mem_avail_mb\": %ld,\n"
        "  \"mem_used_mb\": %ld,\n"
        "  \"mem_used_pct\": %.1f,\n"
        "  \"buffers_cached_mb\": %ld,\n"
        "  \"zram_total_mb\": %ld,\n"
        "  \"zram_used_mb\": %ld,\n"
        "  \"psi_some_avg10\": %.2f,\n"
        "  \"psi_full_avg10\": %.2f,\n"
        "  \"cpu_temp_c\": %.1f,\n"
        "  \"mglru_enabled\": \"%s\"\n"
        "}\n",
        (long)now,
        (int)getpid(),
        m->daemon_rss_kb,
        m->mem_total_kb / 1024,
        m->mem_avail_kb / 1024,
        used_kb / 1024,
        used_pct,
        (m->buffers_kb + m->cached_kb) / 1024,
        m->swap_total_kb / 1024,
        swap_used_kb / 1024,
        m->psi_some_avg10,
        m->psi_full_avg10,
        m->cpu_temp_c,
        m->mglru_state
    );
    fclose(fp);
    chmod(STATUS_TMP, 0644);
    rename(STATUS_TMP, STATUS_JSON);
}

static void perform_gentle_memory_reclaim(const char *reason, int aggressive) {
    sys_metrics_t before, after;
    read_metrics(&before);
    sync();

    FILE *fp = fopen("/proc/sys/vm/drop_caches", "w");
    if (fp) {
        fputs("1\n", fp);
        fclose(fp);
    }

    /* Compact memory only on genuine emergency or manual trigger to avoid
     * Westmere CPU stalls and excessive ext4 buddy/page-cache churn. */
    if (aggressive) {
        fp = fopen("/proc/sys/vm/compact_memory", "w");
        if (fp) {
            fputs("1\n", fp);
            fclose(fp);
        }
    }

    usleep(200000);
    read_metrics(&after);
    long freed_mb = (after.mem_avail_kb - before.mem_avail_kb) / 1024;
    if (freed_mb < 0) freed_mb = 0;

    char msg[256];
    snprintf(msg, sizeof(msg),
        "Otimizacao de RAM executada (%s): MemAvailable %ld MB -> %ld MB (+%ld MB liberados, Daemon RSS: %ld KB)",
        reason, before.mem_avail_kb / 1024, after.mem_avail_kb / 1024, freed_mb, after.daemon_rss_kb);
    log_action("OPTIMIZE", msg);
}

/* Emergency anti-freeze: terminate only the single heaviest Chrome renderer tab if RAM < 180MB */
static void emergency_protect_desktop(void) {
    DIR *d = opendir("/proc");
    if (!d) return;

    pid_t target_pid = 0;
    long max_rss_kb = 0;
    struct dirent *de;

    while ((de = readdir(d)) != NULL) {
        if (!isdigit((unsigned char)de->d_name[0])) continue;
        pid_t pid = atoi(de->d_name);
        if (pid <= 300) continue;

        char cmd_path[128], cmdline[512];
        snprintf(cmd_path, sizeof(cmd_path), "/proc/%d/cmdline", pid);
        FILE *fp = fopen(cmd_path, "r");
        if (!fp) continue;
        size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
        fclose(fp);
        if (n == 0) continue;
        for (size_t i = 0; i < n; i++) {
            if (cmdline[i] == '\0') cmdline[i] = ' ';
        }
        cmdline[n] = '\0';

        /* Only target isolated background renderer processes if critically out of RAM */
        if (strstr(cmdline, "--type=renderer") != NULL) {
            char stat_path[128];
            snprintf(stat_path, sizeof(stat_path), "/proc/%d/status", pid);
            fp = fopen(stat_path, "r");
            if (fp) {
                char line[256];
                while (fgets(line, sizeof(line), fp)) {
                    long rss = 0;
                    if (sscanf(line, "VmRSS: %ld kB", &rss) == 1) {
                        if (rss > max_rss_kb) {
                            max_rss_kb = rss;
                            target_pid = pid;
                        }
                        break;
                    }
                }
                fclose(fp);
            }
        }
    }
    closedir(d);

    if (target_pid > 0 && max_rss_kb > 150000) {
        kill(target_pid, SIGTERM);
        char msg[256];
        snprintf(msg, sizeof(msg),
            "Protecao Anti-Congelamento KDE: Encerrado renderer isolado PID %d (%ld MB RSS) para evitar travamento do sistema.",
            (int)target_pid, max_rss_kb / 1024);
        log_action("GUARDIAN-CRITICAL", msg);
    }
}

/* Spawn transient Gemini 3.8 Flash advisor without blocking or keeping RAM resident */
static void spawn_ephemeral_gemini_advisor(const char *trigger_reason) {
    if (access(ADVISOR_BIN, X_OK) != 0) return;
    pid_t pid = fork();
    if (pid == 0) {
        execl(ADVISOR_BIN, ADVISOR_BIN, "--reason", trigger_reason, (char *)NULL);
        _exit(127);
    }
}

int main(void) {
    mkdir(LOG_DIR, 0755);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = handle_sigusr1;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_sigusr2;
    sigaction(SIGUSR2, &sa, NULL);

    /* Reap child processes automatically so ephemeral Gemini workers leave no zombies */
    signal(SIGCHLD, SIG_IGN);

    FILE *pf = fopen(PID_FILE, "w");
    if (pf) {
        fprintf(pf, "%d\n", (int)getpid());
        fclose(pf);
    }

    sys_metrics_t m;
    read_metrics(&m);
    write_status_json(&m);

    char start_msg[256];
    snprintf(start_msg, sizeof(start_msg),
        "Gaia Guardian C Daemon iniciado (PID %d, Consumo residente: %ld KB RSS, MGLRU: %s)",
        (int)getpid(), m.daemon_rss_kb, m.mglru_state);
    log_action("STARTUP", start_msg);

    time_t last_auto_reclaim = 0;
    time_t last_gemini_consult = 0;

    /* Run an initial asynchronous Gemini 3.8 Flash analysis on startup if insights log doesn't exist yet */
    if (access("/var/log/gaia-guardian/gemini-insights.log", F_OK) != 0) {
        spawn_ephemeral_gemini_advisor("Diagnostico inicial do Gaia Guardian AI");
        last_gemini_consult = time(NULL);
    }

    while (keep_running) {
        read_metrics(&m);
        write_status_json(&m);

        time_t now = time(NULL);
        long used_kb = m.mem_total_kb - m.mem_avail_kb;
        double used_pct = m.mem_total_kb > 0 ? (used_kb * 100.0) / m.mem_total_kb : 0.0;

        if (trigger_reclaim_now) {
            trigger_reclaim_now = 0;
            perform_gentle_memory_reclaim("Acionamento manual via interface/CLI", 1);
            read_metrics(&m);
            write_status_json(&m);
        }

        if (trigger_gemini_now) {
            trigger_gemini_now = 0;
            log_action("GEMINI-AI", "Analise sob demanda solicitada ao Gemini 3.8 Flash.");
            spawn_ephemeral_gemini_advisor("Solicitacao manual do usuario");
            last_gemini_consult = now;
        }

        /* Automatic proactive memory optimization ONLY under genuine pressure:
         * Respect MGLRU working-set protection when PSI is 0 and RAM available >= 512 MB. */
        int real_mem_pressure = (used_pct >= 90.0 && m.mem_avail_kb < 512 * 1024)
                                || (m.psi_some_avg10 >= 15.0)
                                || (m.psi_full_avg10 >= 5.0);
        if (real_mem_pressure && (now - last_auto_reclaim >= 180)) {
            perform_gentle_memory_reclaim("Pressao real de memoria (RAM>=90% / <512MB ou PSI stall)", 0);
            last_auto_reclaim = now;

            if (now - last_gemini_consult >= 600) {
                spawn_ephemeral_gemini_advisor("Alerta automatico de pressao de memoria/PSI");
                last_gemini_consult = now;
            }
        }

        /* Automatic thermal advisory if Westmere CPU exceeds 82C */
        if (m.cpu_temp_c >= 82.0 && (now - last_gemini_consult >= 600)) {
            char tmsg[160];
            snprintf(tmsg, sizeof(tmsg), "Temperatura elevada detectada no Core i7-640M (%.1f C). Acionando consultor Gemini 3.8 Flash.", m.cpu_temp_c);
            log_action("THERMAL-WARN", tmsg);
            spawn_ephemeral_gemini_advisor("Temperatura elevada da CPU (>82C)");
            last_gemini_consult = now;
        }

        /* Emergency desktop freeze protection if available RAM drops below 180 MB */
        if (m.mem_avail_kb > 0 && m.mem_avail_kb < 180 * 1024) {
            perform_gentle_memory_reclaim("Emergencia RAM < 180 MB", 1);
            read_metrics(&m);
            if (m.mem_avail_kb < 160 * 1024) {
                emergency_protect_desktop();
            }
        }

        sleep(5);
    }

    log_action("SHUTDOWN", "Gaia Guardian C Daemon encerrado normalmente.");
    unlink(PID_FILE);
    return 0;
}
